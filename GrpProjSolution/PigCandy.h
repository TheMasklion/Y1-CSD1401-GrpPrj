//---------------------------------------------------------
// file:	PigCandy.h
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Reusable candy and pig code. The candy is a circle that falls under gravity and can be reset to its start position.
//---------------------------------------------------------

#pragma once

#include "cprocessing.h"

#define GRAVITY_Y			980.0f	// When the group's shared gravity vector is ready, pass into RopeGame_Init instead.
#define MAX_FALL_SPEED	1200.0f			// the free candy never falls faster than this, pixels / second
#define PIG_FRAMES		2				// frames stacked top to bottom in Pig.png

// The candy has its own fall speed because it is not held by the rope once cut.
typedef struct
{
	float x;
	float y;
	float r;
	float vy;		// fall speed, pixels / second, +y is down
} FallingCandy;

typedef enum
{
	PIG_NORMAL,		// frame 0, top half of Pig.png
	PIG_HAPPY		// frame 1, bottom half of Pig.png
} PigState;

// Defined in PigCandy.c, shared with every level that includes this header.
//I had AI help me troubleshoot this part, it kept coming up as undeclared identifier, so I had to declare it as extern to be able to use it in other files.
//This was originally in ropegame.c, but I moved it to PigCandy.c so that it can be used in other levels, instead of repeating and using static
extern FallingCandy candy;
extern CP_Vector candy_start;
extern int candy_reached_pig;
extern PigState pig_state;
extern CP_Image candy_img;
extern CP_Image pig_img;

// Resets the candy to its start position, clears the candy_reached_pig flag, and sets the candy's fall speed to 0.
void Candy_Reset(void);

// The candy has fully left the bottom or either side of the screen.
int Candy_IsOffScreen(void);

// Gravity: speed builds up each frame until it hits the limit, then the candy moves by that speed.
void Candy_Update(float dt);