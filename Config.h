#pragma once

#define INVERT_COORDINATE_SYSTEM //we need this because the map's CS is left handed while the ray casting works right handed
#define USE_MULTIPLE_KEYS_SIMULTANEOUSLY
#define ACCESS_STD_OUTPUT_DIRECTLY //avoids flickering

#define MOVE_SPD (10 * sqRes / 100)
#define ROTATE_SPD 4
#define VERTICAL_SPD (sqRes/25)

//bits of fptype >= (number of bits for sqRes) + fp + (1 bit for sign) + X
//where X = max((number of bits for mapSize), (number of bits for the integral part of tan/ctan))

const int fp = 14; //fixed point position
typedef __int32 fptype;

const int screenW = 142, screenH = 40, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4, around3q = 3 * aroundq; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqRes = (1 << 10), sqResh = sqRes / 2;
const fptype sqRes_fp = (fptype)sqRes << fp;
const int safety_dist = 3; //to wall

const int texRes = (1 << 7);

//viewer Current position, orientation and elevation_perc
extern int xC, yC, angleC;
const int xInit = int(2.5 * sqRes), yInit = int(2.5 * sqRes), angleInit = 10;
extern int elevation_perc; //percentage

extern int showMap;
