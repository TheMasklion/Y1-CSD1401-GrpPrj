//---------------------------------------------------------
// file:	PigCandy.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Reusable candy and pig code. The candy is a circle that falls under gravity and can be reset to its start position.
//---------------------------------------------------------

#include "PigCandy.h"

FallingCandy candy;		// follows the rope end while attached, moves by its own gravity once cut
CP_Vector candy_start;	// where the candy is placed on init
int candy_reached_pig;	// 1 once the candy has touched the pig
int candy_lost;			// 1 once the candy has left the bottom or sides of the screen
PigState pig_state;		// which frame of Pig.png is drawn
CP_Image candy_img;
CP_Image pig_img;

// Resets the candy to its start position, clears the candy_reached_pig flag, and sets the candy's fall speed to 0.
void Candy_Reset(void)
{
	candy.x = candy_start.x;
	candy.y = candy_start.y;
	candy.vy = 0.0f;
	candy_reached_pig = 0;
	candy_lost = 0;
	pig_state = PIG_NORMAL;
}

// The candy has fully left the bottom or either side of the screen.
int Candy_IsOffScreen(void)
{
	float screen_w = (float)CP_System_GetWindowWidth();
	float screen_h = (float)CP_System_GetWindowHeight();

	return (candy.y - candy.r > screen_h) ||
		(candy.x + candy.r < 0.0f) ||
		(candy.x - candy.r > screen_w);
}

// Gravity: speed builds up each frame until it hits the limit, then the candy moves by that speed.
void Candy_Update(float dt)
{
	candy.vy += GRAVITY_Y * dt;
	if (candy.vy > MAX_FALL_SPEED)
		candy.vy = MAX_FALL_SPEED;
	candy.y += candy.vy * dt;
}