#pragma once

#include "cprocessing.h"
int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);
int IsAreaClicked_Exit(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y);
int IsCircleClicked(float circle_center_x, float circle_center_y, float diameter, float click_x, float click_y);
void DrawBackground(CP_Image img);
//To be used in win/lose screens, to draw a background image
void DrawImageFill(CP_Image img, float cx, float cy, float w, float h);

//Jayden
int LevelClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y, int i);
