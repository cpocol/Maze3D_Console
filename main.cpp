#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "Config.h"
#include "Controller.h"
#include "main.h"
#include "Map.h"


const int mapSizeHeight = mapHeight * sqRes, mapSizeWidth = mapWidth * sqRes;
const fptype mapSizeWidth_fp = (((fptype)mapSizeWidth) << fp), mapSizeHeight_fp = (((fptype)mapSizeHeight) << fp);
char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen
char Texture[texRes * texRes];
int H[screenW], WallID[screenW], TextureColumn[screenW];

fptype Tan_fp[around]; //fp bits fixed point
fptype CTan_fp[around];

//viewer Current position, orientation and elevation_perc
int xC = xInit;
int yC = yInit;
int angleC = angleInit;
int elevation_perc = 0; //as percentage from wall half height

int showMap = 1;

HANDLE hStdOutput;

float X2Rad(int X) {
    return X * 3.1415f / aroundh;
}

bool init() {
    SetConsoleTitleW(L"Maze 3D in console");

    int i, j;
    //precalculate
    for (int a = 0; a < around; a++) {
        float angf = X2Rad(a);

        //tangent (theoretical range is [-inf..+inf], in practice (-128..+128) is fine)
        fptype maxTan = (((fptype)128) << fp) - 1;
        float temp = tanf(angf) * (((fptype)1) << fp);
        Tan_fp[a] = (fptype)temp;
        if (temp > maxTan)
            Tan_fp[a] = maxTan;
        if (temp < -maxTan)
            Tan_fp[a] = -maxTan;

        //cotangent
        temp = 1 / tanf(angf) * (((fptype)1) << fp);
        CTan_fp[a] = (fptype)temp;
        if (temp > maxTan)
            CTan_fp[a] = maxTan;
        if (temp < -maxTan)
            CTan_fp[a] = -maxTan;
    }

    //load texture
    FILE* pF = fopen("diamond.txt", "r");
    if (!pF) {
        printf("Texture file can't be opened\n");
        return false;
    }

    for (i = 0; i < texRes; i++) {
        char str[texRes + 10];
        memset(str, -1, sizeof(str));
        fgets(str, texRes + 5, pF);
        //check input
        for (j = 0; j < texRes; j++)
            if (str[j] == 0 || str[j] == 10) {
                printf("Texture error on line %d; line ends at column %d\n", i, j);
                return false;
            }
        if (!(str[texRes + (i < texRes - 1)] == 0)) {
            printf("Texture error on line %d; line too long\n", i);
            return false;
        }

        memcpy(Texture + i*texRes, str, texRes);
    }

#ifdef ACCESS_STD_OUTPUT_DIRECTLY
    hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    //programmatically set screen buffer size and window size
    SetConsoleScreenBufferSize(hStdOutput, {(short)screenW, (short)screenH});
    SetConsoleActiveScreenBuffer(hStdOutput);
    SMALL_RECT rectWindow = {0, 0, (short)screenW - 1, (short)screenH - 1};
    SetConsoleWindowInfo(hStdOutput, TRUE, &rectWindow);

    COORD pos = {12, screenH - 1};
    SetConsoleCursorPosition(hStdOutput, pos);
#endif

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
    int xPos = xC / sqRes, yPos = yC / sqRes;

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
    sprintf(str, " e = %4d ", elevation_perc);
    strncpy((char*)screen + 3 * (screenW + 1) + mapWidth + 0, str, strlen(str));
}

//returns wall ID (as map position and cell face)
int CastX(int angle, fptype& xHit_fp, fptype& yHit_fp) { //   hit vertical walls ||
    if ((angle == aroundq) || (angle == around3q))
        return -1; //CastY() will hit a wall correctly

    //prepare as for 1st or 4th quadrant
    int x = (xC / sqRes) * sqRes + sqRes;
    int dx = sqRes,   adjXMap = 0;
    fptype dy_fp = sqRes * Tan_fp[angle];
    //2nd or 3rd quadrant
    if ((aroundq < angle) && (angle < around3q)) {
        x -= sqRes;
        adjXMap = -1;
        dx = -dx;
        dy_fp = -dy_fp;
    }
    yHit_fp = (((fptype)yC) << fp) + (x - xC) * Tan_fp[angle];

    while ((0 < x) && (x < mapSizeWidth) && (0 < yHit_fp) && (yHit_fp < mapSizeHeight_fp) &&
           (Map[(yHit_fp >> fp) / sqRes][x / sqRes + adjXMap] == 0)) {
        x += dx;
        yHit_fp += dy_fp;
    }

    xHit_fp = (fptype)x << fp;

    return int((yHit_fp / sqRes_fp) * mapWidth + (x / sqRes + adjXMap));
}

//returns wall ID (as map position and cell face)
int CastY(int angle, fptype& xHit_fp, fptype& yHit_fp) { //   hit horizontal walls ==
    if ((angle == 0) || (angle == aroundh))
        return -1; //CastX() will hit a wall correctly

    //prepare as for 1st or 2nd quadrant
    int y = (yC / sqRes) * sqRes + sqRes;
    int dy = sqRes,   adjYMap = 0;
    fptype dx_fp = sqRes * CTan_fp[angle];
    if (angle > aroundh) { //3rd or 4th quadrants
        y -= sqRes;
        adjYMap = -1;
        dy = -dy;
        dx_fp = -dx_fp;
    }
    xHit_fp = (((fptype)xC) << fp) + (y - yC) * CTan_fp[angle];

    while ((0 < xHit_fp) && (xHit_fp < mapSizeWidth_fp) && (0 < y) && (y < mapSizeHeight) &&
           (Map[y / sqRes + adjYMap][(xHit_fp >> fp) / sqRes] == 0)) {
        xHit_fp += dx_fp;
        y += dy;
    }

    yHit_fp = (fptype)y << fp;

    return int((y / sqRes + adjYMap) * mapWidth + (xHit_fp / sqRes_fp));
}

//returns wall ID (as map position and cell face)
int Cast(int angle, int& xHit, int& yHit) {
    fptype xX_fp = 1000000000, yX_fp = xX_fp, xY_fp = xX_fp, yY_fp = xX_fp;
    int wallIDX = CastX(angle, xX_fp, yX_fp);
    int wallIDY = CastY(angle, xY_fp, yY_fp);
    //choose the nearest hit point
    if (llabs(((fptype)xC << fp) - xX_fp) < llabs(((fptype)xC << fp) - xY_fp)) { //vertical wall ||
        xHit = int(xX_fp >> fp);
        yHit = int(yX_fp >> fp);
        return 2 * wallIDX + 0;
    }
    else { //horizontal wall ==
        xHit = int(xY_fp >> fp);
        yHit = int(yY_fp >> fp);
        return 2 * wallIDY + 1;
    }
}

void PrintHorizontalText(int col, int row, char text[]) {
   for (int c = col; c < col + (int)strlen(text); c++)
       if (0 <= row && row < screenH && 0 <= c && c < screenW)
           screen[row][c] = text[c - col];
}

void PrintVerticalText(int col, int row, char text[]) {
   for (int r = row; r < row + (int)strlen(text); r++)
       if (0 <= r && r < screenH && 0 <= col && col < screenW)
           screen[r][col] = text[r - row];
}

void RenderColumn(int col, int h, int textureColumn) {
//makes sure the borders are always drawn - it improves the "contrast" if rendered in console
    int Dh_fp = (texRes << 22) / h; //1 row in screen space is this many rows in texture space; use fixed point
    int textureRow_fp = 0;
    int minRow = ((100 - elevation_perc) * (screenH - h) / 2 + elevation_perc * screenHh) / 100; //no elev: minRow = screenHh - h / 2
    int minRowOrig = minRow;
    int maxRow = minRow + h;
    int maxRowOrig = maxRow;

    if (minRow < 0) { //clip
        textureRow_fp = -(minRow * Dh_fp);
        minRow = 0;
    }

    if (maxRow > screenH) //clip
        maxRow = screenH;

    if (minRowOrig >= 0) { //top border visible
        if (minRow < screenH)
            screen[minRow][col] = '*';
        textureRow_fp += Dh_fp;
        minRow++;
    }

    char pixel = '*';
    for (int row = minRow; row < maxRow; row++) {
        if (textureColumn != -1) //not lateral border
            pixel = *(Texture + (textureRow_fp >> 22) * texRes + textureColumn);
        screen[row][col] = pixel;
        textureRow_fp += Dh_fp;
    }

    if (maxRowOrig < screenH) //bottom border visible
        screen[maxRowOrig][col] = '*';

    //display debugging info
    char str[100];
    _itoa(WallID[col], str, 10);
    _itoa(minRowOrig, str, 10);
    //PrintVerticalText(col, minRow, str);
}

void Render() {
    auto time_ms = (float)clock() / CLOCKS_PER_SEC * 1000;
    memset(screen, ' ', sizeof(screen));

    ///pass 1: do the ray casting and store results
    const int viewerToScreen_sq = sq(screenWh) * 3; //FOV = 60 degs => viewerToScreen = screenWh * sqrt(3)
    for (int col = 0; col < screenW; col++) {
        int ang = (screenWh - col + angleC + around) % around;
        int xHit, yHit;
        WallID[col] = Cast(ang, xHit, yHit);

        TextureColumn[col] = ((xHit + yHit) % sqRes) * texRes / sqRes;

        if ((WallID[col] % 2 == 1) && (ang < aroundh) ||
            (WallID[col] % 2 == 0) && ((aroundq < ang) && (ang < around3q)))
            TextureColumn[col] = texRes - TextureColumn[col]; //mirror texture the right way

        int dist_sq = sq(xC - xHit) + sq(yC - yHit) + 1; //+1 avoids division by zero
        H[col] = int(sqRes * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)dist_sq) + 0.5);
    }

    ///pass 2: analyze and improve

    //decide border
    int drawnPrevBorder = 0;
    for (int col = 0; col < screenW; col++) {
        //!when using INVERT_COORDINATE_SYSTEM, next is prev and prev is next, they will be reverted later on
        //tip: while developing, disable INVERT_COORDINATE_SYSTEM

        if (drawnPrevBorder)
            drawnPrevBorder = 0; //*s just drawn for the prev column
        else
            if ((col < screenW - 1) && (WallID[col] != WallID[col + 1]) && (H[col] >= H[col + 1]) || //next in background
                (col > 0) && (WallID[col] != WallID[col - 1]) && (H[col] >= H[col - 1])) { //prev in background
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

    //measure rendering time
    char str[screenW + 2];
    sprintf(str, "%.f ms ", (float)clock() / CLOCKS_PER_SEC * 1000 - time_ms);
    PrintHorizontalText(screenW - 10, 0, str);

    //make it look more like a commnand prompt window
    sprintf(str, "%-*s", screenW, R"(C:\Users\cp>)");
    PrintHorizontalText(0, screenH - 1, str);

    //flush the screen matrix onto the real screen
#ifdef ACCESS_STD_OUTPUT_DIRECTLY
    DWORD dwBytesWritten;
    for (int row = 0; row < screenH; row++) {
        wchar_t scr[screenW];
        for (int col = 0; col < screenW; col++) //convert to wchar_t
            scr[col] = screen[row][col];
#ifdef CODEBLOCKS
        WriteConsoleOutputCharacter(hConsole, (const char*)screen[row], screenW, {0, (short)row}, &dwBytesWritten);
#else
        WriteConsoleOutputCharacter(hStdOutput, (wchar_t*)scr, screenW, {0, (short)row}, &dwBytesWritten);
#endif
    }

    Sleep(20); //it's far too fast :)
#else
    screen[screenH - 1][screenW - 1] = 0; //avoid scrolling one row up when the screen is full
    system("cls"); //clear the (real) screen
    printf("%s", (char*)screen);
#endif
}

int main()
{
    if (!init()) {
        _getch();
        return 0;
    }

    while (1) {
        int cmd = loopController(xC, yC, angleC, around);
        if (cmd > 0)
           Render();
        if (cmd < 0)
           break;
    }

    CloseHandle(hStdOutput);

    return 0;
}
