#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>
#include "Config.h"
#include "Controller.h"
#include "main.h"
#include "Map.h"

void move(int& x, int& y, int angle) {
    float rad = angle * 6.2831f / around;
    int xTest = x + int(MOVE_SPD * cos(rad));
    int yTest = y + int(MOVE_SPD * sin(rad));

    //check for wall collision
    int safetyX = ((aroundq < angle) && (angle < around3q)) ? +1 : -1;
    int safetyY = (angle < aroundh) ? -1 : +1;
    int adjXMap = ((aroundq < angle) && (angle < around3q)) ? -1 : 0;
    int adjYMap = (angle > aroundh) ? -1 : 0;

    int xWall, yWall;
    int wallID = Cast(angle, xWall, yWall);
    if (sq(x - xTest) + sq(y - yTest) >= sq(x - xWall) + sq(y - yWall))
    { //inside wall
        if (wallID % 2 == 0) //vertical wall ||
        {
            x = xWall + safetyX;
            y = yTest;                                      //               __
            if (Map[y / sqSize][x / sqSize] != 0) //it's a corner |
                y = (yTest / sqSize - adjYMap) * sqSize + safetyY;
        }
        else //horizontal wall ==
        {
            x = xTest;
            y = yWall + safetyY;                            //               __
            if (Map[y / sqSize][x / sqSize] != 0) //it's a corner |
                x = (xTest / sqSize - adjXMap) * sqSize + safetyX;
        }
    }
    else //free cell
        x = xTest, y = yTest;
}

void rotate(int& angle, int dir, int around) {
    angle = (angle + dir * ROTATE_SPD + around) % around;
}

int maxJumpHeight = int(1.1 * sqSizeh); //jump this high
float fFPS = 30; //approximate FPS
static int acceleratedMotion[200];

void initController() {
    float fDist = 0, fSpeed = 0, G = 4000;
    for (int i = 0; i < 200; i++) {
        acceleratedMotion[i] = (int)fDist;

        fSpeed += G / fFPS; //G was empirically chosen as we don't have a proper world scale here
        fDist += fSpeed / fFPS;
    }
}

int loopController(int& x, int& y, int& angle, int around) {
    int sign = 1;
#ifdef INVERT_COORDINATE_SYSTEM
    sign = -1;
#endif

    int did = 0;

#ifdef USE_MULTIPLE_KEYS_SIMULTANEOUSLY
    {
        unsigned char ch = 255;
#else
    if (_kbhit()) { //check user's input
        unsigned char ch = toupper(_getch());
#endif
        if ((ch == 27) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) //ASCII code for the Esc key
            exit(0); //end the game
        if ((ch == 'R') || (GetAsyncKeyState('R') & 0x8000)) { //reset viewer's position
            xC = xInit;
            yC = yInit;
            angleC = angleInit;
            did = 1;
        }
        if ((ch == 'M') || (GetAsyncKeyState('M') & 0x8000)) { //show/hide map
            showMap = 1 - showMap;
            did = 1;
        }
        if ((ch == 'W') || (GetAsyncKeyState('W') & 0x8000)) { //pedal forward
            move(x, y, angle);
            did = 1;
        }
        if ((ch == 'S') || (GetAsyncKeyState('S') & 0x8000)) { //pedal backward
            move(x, y, (angle + around / 2) % around);
            did = 1;
        }
        if ((ch == 'A') || (GetAsyncKeyState('A') & 0x8000)) { //strafe left
            move(x, y, (angle + sign * around / 4 + around) % around);
            did = 1;
        }
        if ((ch == 'D') || (GetAsyncKeyState('D') & 0x8000)) { //strafe right
            move(x, y, (angle - sign * around / 4 + around) % around);
            did = 1;
        }

        //jump
        static int verticalAdvance = 0;
        static int height_j, z;
        int refreshVertMove = 1;
        if (((ch == 'E') || (GetAsyncKeyState('E') & 0x8000)) && (verticalAdvance == 0)) {
            verticalAdvance = 1;
            //search for the acceleratedMotion entry so that we'll decelerate to zero speed at max jump height
            for (height_j = 0; height_j < 200; height_j++)
                if (acceleratedMotion[height_j] > maxJumpHeight)
                    break;
            z = max(0, maxJumpHeight - acceleratedMotion[height_j]);
            did = 1;
        }
        else
        if (verticalAdvance > 0) {
            if (height_j > 0) {
                height_j--;
                z = maxJumpHeight - acceleratedMotion[height_j];
            }
            else {
                verticalAdvance = -1;
                z = maxJumpHeight;
            }
            did = 1;
        }
        else
        if (verticalAdvance < 0) {
            if (z > 0) {
                height_j++;
                z = max(0, maxJumpHeight - acceleratedMotion[height_j]);
            }
            else
                verticalAdvance = 0;
            did = 1;
        }
        else
            refreshVertMove = 0;

        if (refreshVertMove) {
            elevation = 100 * z / sqSizeh; //as percentage
        }

        //crunch
        if ((ch == 'C') || (GetAsyncKeyState('C') & 0x8000)) {
            //elevation -= 5;
            did = 1;
        }
#ifdef USE_MULTIPLE_KEYS_SIMULTANEOUSLY
        {
#else
        if (ch == 224) { //it's a key that generates two bytes when being pressed, the first one being 224
            ch = _getch();
#endif
            if ((ch == 75) || (GetAsyncKeyState(VK_LEFT) & 0x8000)) { //the left arrow key => do turn left
                rotate(angle, +1 * sign, around);
                did = 1;
            }
            if ((ch == 77) || (GetAsyncKeyState(VK_RIGHT) & 0x8000)) { //the right arrow key => do turn right
                rotate(angle, -1 * sign, around);
                did = 1;
            }
        }
    } //if (_kbhit())

    return did;
}
