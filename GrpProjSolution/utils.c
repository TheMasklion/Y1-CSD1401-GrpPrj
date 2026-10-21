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

