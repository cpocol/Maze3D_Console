#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "Controller.h"
#include "Map.h"

#define sq(x) ((x)*(x))


const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4; //FOV = 60 degs (6 FOVs = 360 degrees)
char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen

const int sqSide = 128; //must be the side of Texture
const int mapSizeHeight = mapHeight * sqSide, mapSizeWidth = mapWidth * sqSide;

char Texture[sqSide*sqSide];

const int TanFixPoint = 7;
int Tan_fp[around]; //TanFixPoint bits fixed point
int cTan_fp[around];

//initial viewer current position and orientation
int xC = int(2.5f * sqSide);
int yC = int(mapHeight - 2.5f) * sqSide; //flip vertically
int angleC = 1400;

float X2Rad(int X) {
    return X * 3.1415f / aroundh;
}

bool init() {
    int i, j;
    //precalculate
    for (int a = 0; a < around; a++) {
        float angf = X2Rad(a);

        //tangent (theoretical range is [-inf..+inf], in practice (-128..+128) is fine)
        float temp = tanf(angf) * (1 << TanFixPoint);
        if (temp > (128 << TanFixPoint) - 1)
            Tan_fp[a] = (128 << TanFixPoint) - 1;
        else
        if (temp < (-128 << TanFixPoint) + 1)
            Tan_fp[a] = (-128 << TanFixPoint) + 1;
        else
            Tan_fp[a] = (int)temp;

        //cotangent
        temp = 1 / tanf(angf) * (1 << TanFixPoint);
        if (temp > 128 * (1 << TanFixPoint) - 1)
            cTan_fp[a] = 128 * (1 << TanFixPoint) - 1;
        else
        if (temp < -128 * (1 << TanFixPoint) + 1)
            cTan_fp[a] = -128 * (1 << TanFixPoint) + 1;
        else
            cTan_fp[a] = (int)temp;
    }

    //vertically mirror the map (it's more natural to edit it this way)
    for (i = 0; i < mapHeight/2; i++)
        for (j = 0; j < mapWidth; j++) {
            int aux = Map[i][j];
            Map[i][j] = Map[mapHeight - 1 - i][j];
            Map[mapHeight - 1 - i][j] = aux;
        }

    //load texture
    FILE* pF = fopen("diamond.txt", "r");
    if (!pF) {
        printf("Texture file can't be open\n");
        return false;
    }

    for (i = 0; i < sqSide; i++) {
        char str[sqSide + 10];
        memset(str, -1, sizeof(str));
        fgets(str, sqSide + 5, pF);
        //check input
        for (j = 0; j < 128; j++)
            if (str[j] == 0 || str[j] == 10) {
                printf("Texture error on line %d; line ends at char %d\n", i, j);
                return false;
            }
        if (!(str[128 + (i < sqSide - 1)] == 0)) {
            printf("Texture error on line %d; line too long\n", i);
            return false;
        }

        memcpy(Texture + i*sqSide, str, sqSide);
    }

    return true;
}

//returns wall ID (as map position)
int CastX(int ang, int* xS, int* yS) { //   hit vertical walls ||
    if ((ang == aroundq) || (ang == 3 * aroundq / 4))
        return -1; //CastY() will hit a wall correctly

    //prepare as for 1st or 4th quadrant
    int x = (xC / sqSide) * sqSide + sqSide,   dx = sqSide,   adjXMap = 0;
    int dy = ((sqSide * Tan_fp[ang]) >> TanFixPoint);
    //2nd or 3rd quadrant
    if ((aroundq < ang) && (ang < 3 * aroundq)) {
        x -= sqSide;
        adjXMap = -1;
        dx = -dx;
        dy = -dy;
    }
    int y = yC + (((x - xC) * Tan_fp[ang]) >> TanFixPoint);

    while ((x > 0) && (x < mapSizeWidth) && (y > 0) && (y < mapSizeHeight) &&
           (Map[y / sqSide][x / sqSide + adjXMap] == 0)) {
        x += dx;
        y += dy;
    }

    *xS = x;
    *yS = y;
    return (y / sqSide) * mapWidth + (x / sqSide);
}

//returns wall ID (as map position)
int CastY(int ang, int* xS, int* yS) { //   hit horizontal walls ==
    if ((ang == 0) || (ang == aroundh))
        return -1; //CastX() will hit a wall correctly

    //prepare as for 1st or 2nd quadrant
    int y = (yC / sqSide) * sqSide + sqSide,   dy = sqSide,   adjYMap = 0;
    int dx = (sqSide * cTan_fp[ang]) >> TanFixPoint;
    if (ang > aroundh) { //3rd or 4th quadrants
        y -= sqSide;
        adjYMap = -1;
        dy = -dy;
        dx = -dx;
    }
    int x = xC + (((y - yC) * cTan_fp[ang]) >> TanFixPoint);

    while ((x > 0) && (x < mapSizeWidth) && (y > 0) && (y < mapSizeHeight) &&
           (Map[y / sqSide + adjYMap][x / sqSide] == 0)) {
        x += dx;
        y += dy;
    }

    *xS = x;
    *yS = y;
    return (y / sqSide) * mapWidth + (x / sqSide);
}

void RenderColumn(int col, int h, int textureColumn) {
    int Dh_fp = (sqSide << 10) / h; //1 row in screen space is this many rows in texture space; 10 bits fixed point
    int textureRow_fp = 0;
    int minY = screenHh - h / 2;
    if (minY < 0) {
        textureRow_fp = -minY * Dh_fp;
        minY = 0;
    }
    int maxY = min(screenHh + h / 2, screenH);

    for (int row = minY; row < maxY; row++) {
        char pixel = *(Texture + textureColumn + (textureRow_fp >> 10) * sqSide);
        //make sure the borders are always drawn - it improves the "contrast"
        if ((textureColumn == -1) //wall lateral margin
            || (row == (screenHh - h / 2)) || (row == (screenHh + h / 2) - 1)) //top or bottom
            pixel = '*';
        screen[row][col] = pixel;
        textureRow_fp += Dh_fp;
    }
}

void Render() {
    memset(screen, ' ', sizeof(screen));

    const int viewerToScreen_sq = sq(screenWh) * 3; //FOV = 60 degs => viewerToScreen = screenWh * sqrt(3)
    int prevWallID, prevWallHeight;
    for (int col = 0; col < screenW; col++) {
        int xX = 1000000, yX = 1000000, xY = 1000000, yY = 1000000, xHit, yHit, wallID, h;
        int ang = (screenWh - col + angleC + around) % around;
        int wallIDX = CastX(ang, &xX, &yX);
        int wallIDY = CastY(ang, &xY, &yY);
        if (abs(xC - xX) < abs(xC - xY)) { //choose the nearest hit point
            xHit = xX;
            yHit = yX;
            wallID = wallIDX;
        }
        else {
            xHit = xY;
            yHit = yY;
            wallID = wallIDY;
        }
        int textureColumn = (xHit + yHit) % sqSide;
        int dist_sq = sq(xC - xHit) + sq(yC - yHit);
        if (dist_sq == 0)
            h = 10000;
        else
            h = int(sqSide * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)dist_sq));

        RenderColumn(col, h, textureColumn);

        //make sure the borders are always drawn - it improves the "contrast"
        if ((col > 0) && (wallID != prevWallID)) //we just finished rendering a cube
            if (h < prevWallHeight) //prev cube was in foreground, curr in background
                RenderColumn(col - 1, prevWallHeight, -1); //go back render the margin of prev cube with '*'
            else //prev cube was in background, curr in foreground
                RenderColumn(col, h, -1); //re-render the margin of curr cube with '*'

        prevWallID = wallID;
        prevWallHeight = h;
    }

    for (int row = 0; row < screenH; row++)
        screen[row][screenW] = '\n';

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

    while (1) {
        Render();
        loopController(xC, yC, angleC, around);
    }

    return 0;
}
