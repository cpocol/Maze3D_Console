#pragma once

#define INVERT_COORDINATE_SYSTEM //we need this because the map's CS is left handed while the ray casting works right handed
#define USE_MULTIPLE_KEYS_SIMULTANEOUSLY

#define MOVE_SPD 20
#define ROTATE_SPD 5
#define VERTICAL_SPD 5

//(number of bits for sqSize (= texture size)) + fp + (1 bit for sign) + X <= 32
//where X = max((number of bits for mapSize), (number of bits for the integral part of tan/ctan))

const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4, around3q = 3 * aroundq; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqSize = 128, sqSizeh = sqSize / 2; //must be the size of Texture

const int fp = 17; //fixed point position

//viewer Current position, orientation and elevation
extern int xC, yC, angleC;
const int xInit = int(2.5 * sqSize), yInit = int(2.5 * sqSize), angleInit = 10;
extern int elevation; //percentage

extern int showMap;
