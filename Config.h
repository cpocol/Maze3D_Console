#pragma once

//#define CONTINUOUS_RENDERING //comment this out and new rendering will be done only when pressing a key (avoid flickering)
#define INVERT_COORDINATE_SYSTEM //we need this because the map's CS is left handed while the ray casting works right handed

#define MOVE_SPD 20
#define ROTATE_SPD 5

const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqSide = 128; //must be the side of Texture

//viewer current position and orientation
extern int xC, yC, angleC;
