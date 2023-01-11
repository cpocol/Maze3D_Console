#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "Config.h"
#include "Controller.h"
#include "main.h"
#include "Map.h"


const int mapSizeHeight = mapHeight * sqSize, mapSizeWidth = mapWidth * sqSize;
const int mapSizeWidth_fp = (mapSizeWidth << fp), mapSizeHeight_fp = (mapSizeHeight << fp);
char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen
char Texture[sqSize*sqSize];
int H[screenW], WallID[screenW], TextureColumn[screenW];

int Tan_fp[around]; //fp bits fixed point
int cTan_fp[around];

//initial viewer Current position and orientation
int xC = xInit;
int yC = yInit;
int angleC = angleInit;

int showMap = 0;

float X2Rad(int X) {
    return X * 3.1415f / aroundh;
}

bool init() {
    int i, j;
    //precalculate
    for (int a = 0; a < around; a++) {
        float angf = X2Rad(a);

        //tangent (theoretical range is [-inf..+inf], in practice (-128..+128) is fine)
        float temp = tanf(angf) * (1 << fp);
        Tan_fp[a] = (int)temp;
        if (temp > (128 << fp) - 1)
            Tan_fp[a] = (128 << fp) - 1;
        if (temp < (-128 << fp) + 1)
            Tan_fp[a] = (-128 << fp) + 1;

        //cotangent
        temp = 1 / tanf(angf) * (1 << fp);
        cTan_fp[a] = (int)temp;
        if (temp > 128 * (1 << fp) - 1)
            cTan_fp[a] = 128 * (1 << fp) - 1;
        if (temp < -128 * (1 << fp) + 1)
            cTan_fp[a] = -128 * (1 << fp) + 1;
    }

    //load texture
    FILE* pF = fopen("diamond.txt", "r");
    if (!pF) {
        printf("Texture file can't be open\n");
        return false;
    }

    for (i = 0; i < sqSize; i++) {
        char str[sqSize + 10];
        memset(str, -1, sizeof(str));
        fgets(str, sqSize + 5, pF);
        //check input
        for (j = 0; j < 128; j++)
            if (str[j] == 0 || str[j] == 10) {
                printf("Texture error on line %d; line ends at char %d\n", i, j);
                return false;
            }
        if (!(str[128 + (i < sqSize - 1)] == 0)) {
            printf("Texture error on line %d; line too long\n", i);
            return false;
        }

        memcpy(Texture + i*sqSize, str, sqSize);
    }

    return true;
}

void renderMap() {
    for (int y = 0; y < mapHeight; y++)
        for (int x = 0; x < mapWidth; x++)
            if (Map[y][x] == 0)
                screen[y][x] = ' ';
            else
                screen[y][x] = 'W';
    int xPos = xC / sqSize, yPos = yC / sqSize;
    
    screen[yPos][xPos] = 'P';

    if ((0 < xPos) && (xPos < mapWidth - 1) && (0 < yPos) && (yPos < mapHeight - 1))
    {
        char dx, dy, dir;
        if ((angleC < 1 * around / 16) || (angleC >= 15 * around / 16))
            dx = +1, dy = 0, dir = '-';
        if ((1 * around / 16 <= angleC) && (angleC < 3 * around / 16))
            dx = +1, dy = +1, dir = '\\';
        if ((3 * around / 16 <= angleC) && (angleC < 5 * around / 16))
            dx = +0, dy = +1, dir = '|';
        if ((5 * around / 16 <= angleC) && (angleC < 7 * around / 16))
            dx = -1, dy = +1, dir = '/';
        if ((7 * around / 16 <= angleC) && (angleC < 9 * around / 16))
            dx = -1, dy = 0, dir = '-';
        if ((9 * around / 16 <= angleC) && (angleC < 11 * around / 16))
            dx = -1, dy = -1, dir = '\\';
        if ((11 * around / 16 <= angleC) && (angleC < 13 * around / 16))
            dx = +0, dy = -1, dir = '|';
        if ((13 * around / 16 <= angleC) && (angleC < 15 * around / 16))
            dx = +1, dy = -1, dir = '/';
        int xDirPos = xPos + dx;
        int yDirPos = yPos + dy;
        screen[yDirPos][xDirPos] = dir;
    }
}

//returns wall ID (as map position)
int CastX(int angle, int& xHit, int& yHit) { //   hit vertical walls ||
    if ((angle == aroundq) || (angle == around3q))
        return -1; //CastY() will hit a wall correctly

    //prepare as for 1st or 4th quadrant
    xHit = (xC / sqSize) * sqSize + sqSize;
    int dx = sqSize,   adjXMap = 0;
    int dy_fp = sqSize * Tan_fp[angle];
    //2nd or 3rd quadrant
    if ((aroundq < angle) && (angle < around3q)) {
        xHit -= sqSize;
        adjXMap = -1;
        dx = -dx;
        dy_fp = -dy_fp;
    }
    int yHit_fp = (yC << fp) + (xHit - xC) * Tan_fp[angle];

    while ((0 < xHit) && (xHit < mapSizeWidth) && (0 < yHit_fp) && (yHit_fp < mapSizeHeight_fp) &&
           (Map[(yHit_fp >> fp) / sqSize][xHit / sqSize + adjXMap] == 0)) {
        xHit += dx;
        yHit_fp += dy_fp;
    }
    yHit = (yHit_fp >> fp);

    return (yHit / sqSize) * mapWidth + (xHit / sqSize + adjXMap);
}

//returns wall ID (as map position)
int CastY(int angle, int& xHit, int& yHit) { //   hit horizontal walls ==
    if ((angle == 0) || (angle == aroundh))
        return -1; //CastX() will hit a wall correctly

    //prepare as for 1st or 2nd quadrant
    yHit = (yC / sqSize) * sqSize + sqSize;
    int dy = sqSize,   adjYMap = 0;
    int dx_fp = sqSize * cTan_fp[angle];
    if (angle > aroundh) { //3rd or 4th quadrants
        yHit -= sqSize;
        adjYMap = -1;
        dy = -dy;
        dx_fp = -dx_fp;
    }
    int xHit_fp = (xC << fp) + (yHit - yC) * cTan_fp[angle];

    while ((0 < xHit_fp) && (xHit_fp < mapSizeWidth_fp) && (0 < yHit) && (yHit < mapSizeHeight) &&
           (Map[yHit / sqSize + adjYMap][(xHit_fp >> fp) / sqSize] == 0)) {
        xHit_fp += dx_fp;
        yHit += dy;
    }
    xHit = (xHit_fp >> fp);

    return (yHit / sqSize + adjYMap) * mapWidth + (xHit / sqSize);
}

//returns wall ID (as map position)
int Cast(int angle, int& xHit, int& yHit) {
    int xX = 1000000, yX = 1000000, xY = 1000000, yY = 1000000;
    int wallIDX = CastX(angle, xX, yX);
    int wallIDY = CastY(angle, xY, yY);
    //choose the nearest hit point
    if (abs(xC - xX) < abs(xC - xY)) { //vertical wall ||
        xHit = xX;
        yHit = yX;
        return 2 * wallIDX + 0;
    }
    else { //horizontal wall ==
        xHit = xY;
        yHit = yY;
        return 2 * wallIDY + 1;
    }
}

void RenderColumn(int col, int h, int textureColumn) {
//makes sure the borders are always drawn - it improves the "contrast" if rendered in console

    int Dh_fp = (sqSize << 20) / h; //1 row in screen space is this many rows in texture space; use fixed point
    int textureRow_fp = 0;
    int minRow = screenHh - h / 2;
    int maxRow = min(minRow + h, screenH);
    if (minRow < 0) {
        textureRow_fp = -(minRow * Dh_fp);
        minRow = 0;
    }

    if (screenHh - h / 2 >= 0) { //top border visible
        screen[minRow][col] = '*';
        textureRow_fp += Dh_fp;
        minRow++;
    }

    char pixel = '*';
    for (int row = minRow; row < maxRow; row++) {
        if (textureColumn != -1) //not lateral border
            pixel = *(Texture + (textureRow_fp >> 20) * sqSize + textureColumn);
        screen[row][col] = pixel;
        textureRow_fp += Dh_fp;
    }

    if (maxRow < screenH) //bottom border visible
        screen[maxRow][col] = '*';

    //display debugging info
//   char str[100];
//   _itoa(H[col], str, 10);
//   int startRow = minRow_p / 2;
//   for (int row = startRow; row < startRow + (int)strlen(str); row++)
//       screen[row][col] = str[row - startRow];
}

void Render() {
    memset(screen, ' ', sizeof(screen));

    //pass 1: do the ray casting and store results
    const int viewerToScreen_sq = sq(screenWh) * 3; //FOV = 60 degs => viewerToScreen = screenWh * sqrt(3)
    for (int col = 0; col < screenW; col++) {
        int ang = (screenWh - col + angleC + around) % around;
        int xHit, yHit;
        WallID[col] = Cast(ang, xHit, yHit);

        TextureColumn[col] = (xHit + yHit) % sqSize;
        int dist_sq = sq(xC - xHit) + sq(yC - yHit);
        if (dist_sq == 0)
            H[col] = 10000;
        else
            H[col] = int(sqSize * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)dist_sq) + 0.5);
    }

    //pass 2: analyze and improve
    int drawnPrevBorder = 0;
    for (int col = 0; col < screenW; col++) {
        //!when using INVERT_COORDINATE_SYSTEM, next is prev and prev is next, they will be reverted later on
        //tip: while developing, disable INVERT_COORDINATE_SYSTEM

        if (drawnPrevBorder)
            drawnPrevBorder = 0; //*s just drawn for the prev column
        else
            if ((col < screenW - 1) && (WallID[col] != WallID[col + 1]) && (H[col] >= H[col + 1]) || //next in background
                (col > 0) && (WallID[col] != WallID[col - 1]) && (H[col] >= H[col - 1])) //prev in background
            {
                TextureColumn[col] = -1; //render the last border of curr cube with '*'
                drawnPrevBorder = 1;
            }
    }

    //pass 3: do rendering
    for (int col = 0; col < screenW; col++)
        RenderColumn(col, H[col], TextureColumn[col]);

    for (int row = 0; row < screenH; row++) {
        screen[row][screenW] = '\n';
#ifdef INVERT_COORDINATE_SYSTEM
        for (int col = 0; col < screenWh; col++) {
            char aux = screen[row][col];
            screen[row][col] = screen[row][screenW - 1 - col];
            screen[row][screenW - 1 - col] = aux;
        }
#endif
    }

    ///improve output
    //use a big round character ('O')
    for (int row = 0; row < screenH; row++)
        for (int col = 0; col < screenW; col++) {
            if ((screen[row][col] == 0) || (screen[row][col] == 10) || (screen[row][col] == 13)
                || (screen[row][col] == ' ') || (screen[row][col] == '*'))
                continue;
            screen[row][col] = 'O';
        }

    //do anti-aliasing - use a small round character
    //when having at least one blank neighbour and at least one 'O' neighbour
    for (int row = 0; row < screenH; row++)
        for (int col = 0; col < screenW; col++)
            if (screen[row][col] == 'O') {
                int blanksCnt = 0, bigOsCnt = 0;
                for (int drow = -1; drow <= 1; drow++)
                    for (int dcol = -1; dcol <= 1; dcol++) {
                        int col1 = col + dcol;
                        int row1 = row + drow;
                        if ((col1 >= 0) && (col1 < screenW) && (row1 >= 0) && (row1 < screenH)) {
                            blanksCnt += (screen[row1][col1] == ' ');
                            bigOsCnt  += (screen[row1][col1] == 'O');
                        }
                    }
                if (blanksCnt && bigOsCnt)
                    screen[row][col] = 'o';
            }

    if (showMap)
        renderMap();

    //flush the screen matrix onto the real screen
    system("cls"); //clear the (real) screen
    screen[screenH - 1][screenW - 1] = 0; //avoid scrolling one row up when the screen is full
    printf("%s", (char*)screen);
    Sleep(19); //tune this one to get less flickering on a specific PC
}

int main()
{
    if (!init())
        return 0;

    Render();
    while (1) {
        if (loopController(xC, yC, angleC, around))
           Render();
    }

    return 0;
}
