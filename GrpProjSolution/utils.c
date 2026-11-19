//---------------------------------------------------------
// file:	utils.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	utility functions for solo proj 2, for onclick functions rectangle and circles. Computing the area of the shapes and whether
//			the clicked area, is inside the shape.
//
//---------------------------------------------------------
#include <math.h>
#include <stdio.h>
#include "cprocessing.h"
#include "utils.h"


//rect parameters = float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y

int IsAreaClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y)
{
	//return 1 if yes, else return 0
	if (CP_Input_MouseClicked())
	{
		// Check if the click is within the area
		if (click_x >= area_center_x - area_width / 2 && click_x <= area_center_x + area_width / 2 && // <-----X-----> just checking that it is within the x axis from point x
				click_y >= area_center_y - area_height / 2 && click_y <= area_center_y + area_height / 2) //just checking that it is within the y axis, from point y
			{
				//printf("Clicked Rectangle\n", click_x, click_y);
				return 1;
			}
	}
	return 0;
}

int IsCircleClicked(float circle_center_x, float circle_center_y, float diameter, float click_x, float click_y)
{
	//return 1 if yes, else return 0
	if (CP_Input_MouseClicked())
	{
		float radius = diameter / 2;
		float dx = click_x - circle_center_x;
		float dy = click_y - circle_center_y;
		float distance_squared = dx * dx + dy * dy;
		if (distance_squared <= radius * radius)
		{
			//printf("X:%f, y:%f\n", click_x, click_y);
			return 1;
		}
	}
	return 0;
}

//Jayden Tan utils ---------------------------------------------------------------------------------------------------------------------------------------
//return the value of i if true means the button got clicked and return that button number
int LevelClicked(float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y, int i)
{
	//return i
	if (CP_Input_MouseClicked())
	{
		// Check if the click is within the area
		if (click_x >= area_center_x - area_width / 2 && click_x <= area_center_x + area_width / 2 && // <-----X-----> just checking that it is within the x axis from point x
			click_y >= area_center_y - area_height / 2 && click_y <= area_center_y + area_height / 2) //just checking that it is within the y axis, from point y
		{
			//START FROM CASE 1
			return i+1;
		}
	}
	return 0;
}


// Draws img over the whole window without stretching: scaled to cover the window,
// with the overflow cropped evenly from the sides or from the top and bottom.
void DrawBackground(CP_Image img)
{
	float win_w = (float)CP_System_GetWindowWidth();
	float win_h = (float)CP_System_GetWindowHeight();
	float img_w = (float)CP_Image_GetWidth(img);
	float img_h = (float)CP_Image_GetHeight(img);
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = img_w;
	float v1 = img_h;

	if (img_w / img_h > win_w / win_h)		// image is wider than the window: crop the sides
	{
		float keep_w = img_h * win_w / win_h;
		u0 = (img_w - keep_w) / 2.0f;
		u1 = u0 + keep_w;
	}
	else									// image is taller than the window: crop top and bottom
	{
		float keep_h = img_w * win_h / win_w;
		v0 = (img_h - keep_h) / 2.0f;
		v1 = v0 + keep_h;
	}
	CP_Image_DrawSubImage(img, win_w / 2.0f, win_h / 2.0f, win_w, win_h, u0, v0, u1, v1, 255);
}

void DrawImageFill(CP_Image img, float cx, float cy, float w, float h)
{
	float img_w = (float)CP_Image_GetWidth(img);
	float img_h = (float)CP_Image_GetHeight(img);
	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = img_w;
	float v1 = img_h;

	if (img_w / img_h > w / h)			// image is wider than the rectangle: crop the sides
	{
		float keep_w = img_h * w / h;
		u0 = (img_w - keep_w) / 2.0f;
		u1 = u0 + keep_w;
	}
	else								// image is taller than the rectangle: crop top and bottom
	{
		float keep_h = img_w * h / w;
		v0 = (img_h - keep_h) / 2.0f;
		v1 = v0 + keep_h;
	}
	CP_Image_DrawSubImage(img, cx, cy, w, h, u0, v0, u1, v1, 255);
}