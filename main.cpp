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
const fptype mapSizeWidth_fp = (((fptype)mapSizeWidth) << fp), mapSizeHeight_fp = (((fptype)mapSizeHeight) << fp);
char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen
char Texture[sqSize*sqSize];
int H[screenW], WallID[screenW], TextureColumn[screenW];

fptype Tan_fp[around]; //fp bits fixed point
fptype cTan_fp[around];

//initial viewer Current position, orientation and elevation
int xC = xInit;
int yC = yInit;
int angleC = angleInit;
int elevation = 0; //as percentage from wall half height

int showMap = 1;

float X2Rad(int X) {
    return X * 3.1415f / aroundh;
}

bool init() {
    int i, j;
    //precalculate
    for (int a = 0; a < around; a++) {
        float angf = X2Rad(a);

        //tangent (theoretical range is [-inf..+inf], in practice (-128..+128) is fine)
        float temp = tanf(angf) * (((fptype)1) << fp);
        Tan_fp[a] = (fptype)temp;
        if (temp > (((fptype)128) << fp) - 1)
            Tan_fp[a] = (((fptype)128) << fp) - 1;
        if (temp < (-((fptype)128) << fp) + 1)
            Tan_fp[a] = (-((fptype)128) << fp) + 1;

        //cotangent
        temp = 1 / tanf(angf) * (((fptype)1) << fp);
        cTan_fp[a] = (fptype)temp;
        if (temp > (((fptype)128) << fp) - 1)
            cTan_fp[a] = (((fptype)128) << fp) - 1;
        if (temp < (-((fptype)128) << fp) + 1)
            cTan_fp[a] = (-((fptype)128) << fp) + 1;
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

    initController();

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

    int dx[] = {+1, +1,  0, -1, -1, -1,  0, +1};
    int dy[] = { 0, +1, +1, +1,  0, -1, -1, -1};
    char dirs[] = R"(-\|/-\|/)";

    int ang = (angleC + around / 16) % around;
    int a = ang / (around / 8);
    int xDirPos = xPos + dx[a];
    int yDirPos = yPos + dy[a];
    if ((0 <= xDirPos) && (xDirPos < mapWidth) && (0 <= yDirPos) && (yDirPos < mapHeight))
        screen[yDirPos][xDirPos] = dirs[a];

    char str[100];
    sprintf(str, " x = %4d ", xC);
    strncpy((char*)screen + 0 * (screenW + 1) + mapWidth + 0, str, strlen(str));
    sprintf(str, " y = %4d ", yC);
    strncpy((char*)screen + 1 * (screenW + 1) + mapWidth + 0, str, strlen(str));
    sprintf(str, " a = %4d ", angleC);
    strncpy((char*)screen + 2 * (screenW + 1) + mapWidth + 0, str, strlen(str));
    sprintf(str, " e = %4d ", elevation);
    strncpy((char*)screen + 3 * (screenW + 1) + mapWidth + 0, str, strlen(str));
}

//returns wall ID (as map position)
int CastX(int angle, int& xHit, int& yHit) { //   hit vertical walls ||
    if ((angle == aroundq) || (angle == around3q))
        return -1; //CastY() will hit a wall correctly

    //prepare as for 1st or 4th quadrant
    xHit = (xC / sqSize) * sqSize + sqSize;
    int dx = sqSize,   adjXMap = 0;
    fptype dy_fp = sqSize * Tan_fp[angle];
    //2nd or 3rd quadrant
    if ((aroundq < angle) && (angle < around3q)) {
        xHit -= sqSize;
        adjXMap = -1;
        dx = -dx;
        dy_fp = -dy_fp;
    }
    fptype yHit_fp = (((fptype)yC) << fp) + (xHit - xC) * Tan_fp[angle];

    while ((0 < xHit) && (xHit < mapSizeWidth) && (0 < yHit_fp) && (yHit_fp < mapSizeHeight_fp) &&
           (Map[(yHit_fp >> fp) / sqSize][xHit / sqSize + adjXMap] == 0)) {
        xHit += dx;
        yHit_fp += dy_fp;
    }
    yHit = int(yHit_fp >> fp);

    return (yHit / sqSize) * mapWidth + (xHit / sqSize + adjXMap);
}

//returns wall ID (as map position)
int CastY(int angle, int& xHit, int& yHit) { //   hit horizontal walls ==
    if ((angle == 0) || (angle == aroundh))
        return -1; //CastX() will hit a wall correctly

    //prepare as for 1st or 2nd quadrant
    yHit = (yC / sqSize) * sqSize + sqSize;
    int dy = sqSize,   adjYMap = 0;
    fptype dx_fp = sqSize * cTan_fp[angle];
    if (angle > aroundh) { //3rd or 4th quadrants
        yHit -= sqSize;
        adjYMap = -1;
        dy = -dy;
        dx_fp = -dx_fp;
    }
    fptype xHit_fp = (((fptype)xC) << fp) + (yHit - yC) * cTan_fp[angle];

    while ((0 < xHit_fp) && (xHit_fp < mapSizeWidth_fp) && (0 < yHit) && (yHit < mapSizeHeight) &&
           (Map[yHit / sqSize + adjYMap][(xHit_fp >> fp) / sqSize] == 0)) {
        xHit_fp += dx_fp;
        yHit += dy;
    }
    xHit = int(xHit_fp >> fp);

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
    //int minRow = screenHh - h / 2;
    int minRow = ((100 - elevation) * (2 * screenHh - h) / 2 + elevation * screenHh) / 100;
    int maxRow = min(minRow + h, screenH);

    int minRowOrig = minRow;
    if (minRow < 0) { //clip
        textureRow_fp = -(minRow * Dh_fp);
        minRow = 0;
    }

    if (minRowOrig >= 0) { //top border visible
	    if (minRow < screenH)
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
   int startRow = minRow;
   char str[100];
   _itoa(WallID[col], str, 10);
   //_itoa(h, str, 10);
   for (int row = startRow; row < startRow + (int)strlen(str); row++)   screen[row][col] = str[row - startRow];
}

void Render() {
    memset(screen, ' ', sizeof(screen));

    ///pass 1: do the ray casting and store results
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

    ///pass 2: analyze and improve

	//sometimes, the ray hits a cube exactly in its corner, thus there is an ambiguity about which face was hit
	//fix it by a custom analyzis of local wall ids and heights
    for (int col = 1; col < screenW - 1; col++) {
		if ((WallID[col] / 2 == WallID[col + 1] / 2) && (WallID[col] / 2 != WallID[col - 1] / 2)
			&& (abs(H[col - 1] - H[col + 1]) <= 1))
			WallID[col] = WallID[col + 1];

		if ((WallID[col] / 2 == WallID[col - 1] / 2) && (WallID[col] / 2 != WallID[col + 1] / 2)
			&& (abs(H[col - 1] - H[col + 1]) <= 1))
			WallID[col] = WallID[col - 1];
	}

	//decide border
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

    ///pass 3: do rendering
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
            //screen[row][col] = 'O';
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
    //Sleep(19); //tune this one to get less flickering on a specific PC
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
