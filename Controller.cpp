#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>
#include "Config.h"
#include "Controller.h"
#include "main.h"

void doPedal(int& x, int& y, int angle) {
    float rad = angle * 6.2831f / around;
	int xTest = x + int(MOVE_SPD * cos(rad));
	int yTest = y + int(MOVE_SPD * sin(rad));

	//check for wall collision
	int xWall, yWall;
	int wallID = Cast(angle, xWall, yWall);
	if (sq(x - xTest) + sq(y - yTest) > sq(x - xWall) + sq(y - yWall))
	{
		if (wallID % 2 == 0) //vertical wall
		{
			x = xWall;
			y = yTest;
		}
		else //horizontal wall
		{
			x = xTest;
			y = yWall;
		}
	}
	else
		x = xTest, y = yTest;
}

void doRotate(int& angle, int dir, int around) {
	angle = (angle + dir * ROTATE_SPD + around) % around;
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
        unsigned char ch = _getch();
#endif
        if ((ch == 27) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) //ASCII code for the Esc key
            exit(0); //end the game
        if ((tolower(ch) == 'w') || (GetAsyncKeyState('W') & 0x8000)) { //the up arrow key => pedal forward
			doPedal(x, y, angle);
			did = 1;
		}
        if ((tolower(ch) == 's') || (GetAsyncKeyState('S') & 0x8000)) { //the down arrow key => pedal backward
			doPedal(x, y, (angle + around / 2) % around);
			did = 1;
		}
		if ((tolower(ch) == 'a') || (GetAsyncKeyState('A') & 0x8000)) { //strafe left
			doPedal(x, y, (angle + sign * around / 4) % around);
			did = 1;
		}
        if ((tolower(ch) == 'd') || (GetAsyncKeyState('D') & 0x8000)) { //strafe right
			doPedal(x, y, (angle - sign * around / 4 + around) % around);
			did = 1;
		}
#ifdef USE_MULTIPLE_KEYS_SIMULTANEOUSLY
		{
#else
        if (ch == 224) { //it's a key that generates two bytes when being pressed, the first one being 224
            ch = _getch();
#endif
            if ((ch == 75) || (GetAsyncKeyState(VK_LEFT) & 0x8000)) { //the left arrow key => do turn left
				doRotate(angle, +1 * sign, around);
				did = 1;
			}
            if ((ch == 77) || (GetAsyncKeyState(VK_RIGHT) & 0x8000)) { //the right arrow key => do turn right
				doRotate(angle, -1 * sign, around);
				did = 1;
			}
        }
    } //if (_kbhit())

	return did;
}
