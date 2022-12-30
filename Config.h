#pragma once

#define CONTINUOUS_RENDERING //comment this out and new rendering will be done only when pressing a key (avoid flickering)
#define INVERT_COORDINATE_SYSTEM //

#define MOVE_SPD 20
#define ROTATE_SPD 5

const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqSide = 128; //must be the side of Texture

//initial viewer current position and orientation
//int xC = int(2.5f * sqSide);
//int yC = int(mapHeight - 2.5f) * sqSide; //flip vertically
//int angleC = 1400;
