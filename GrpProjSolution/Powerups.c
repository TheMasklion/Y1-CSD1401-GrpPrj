//---------------------------------------------------------
// file:	Powerups.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Bubble power-up. Captures the free-falling candy, floats it up until the player clicks the bubble
//			or it reaches the top of the screen, then lets the candy fall normally again.
//---------------------------------------------------------

#pragma once

#include "Powerups.h"
#include "PigCandy.h"	// candy (extern)
#include "collision.h"
#include "utils.h"

// Bubble power-up: captures the free-falling candy, floats it up until the player clicks the bubble or it reaches the top of the screen, then lets the candy fall normally again.
typedef enum
{
	BUBBLE_WAITING,
	BUBBLE_CARRYING,
	BUBBLE_POPPED
} BubbleState;

typedef struct
{
	float x;
	float y;
	float r;
	BubbleState state;
} Bubble;

static Bubble bubble;

void Bubble_Init(float x, float y) //Max 1 bubbl per level, I think might need to set a max bubble counter, if I want more?
{
	bubble.x = x;
	bubble.y = y;
	bubble.r = BUBBLE_RADIUS;
	bubble.state = BUBBLE_WAITING;
}

// Gone: the candy falls from rest, so Candy_Update starts it again from vy = 0.
static void Bubble_Pop(void)
{
	bubble.state = BUBBLE_POPPED;
	candy.vy = 0.0f;
}

void Bubble_TryCapture(void)
{
	if (bubble.state != BUBBLE_WAITING)
		return;

	if (IsCircleOverlapping(bubble.x, bubble.y, bubble.r, candy.x, candy.y, candy.r))
	{
		candy.x = bubble.x;		// candy in the centre of the bubble
		candy.y = bubble.y;
		candy.vy = 0.0f;
		bubble.state = BUBBLE_CARRYING;
	}
}

int Bubble_IsCarrying(void)
{
	return bubble.state == BUBBLE_CARRYING;
}

void Bubble_Update(float dt)
{
	if (bubble.state != BUBBLE_CARRYING)
		return;

	// click on the bubble to pop it (IsCircleClicked takes a diameter, like CP_Graphics_DrawCircle)
	if (IsCircleClicked(bubble.x, bubble.y, bubble.r * 2.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		Bubble_Pop();
		return;
	}

	bubble.y -= BUBBLE_RISE_SPEED * dt;		// -y is up
	candy.x = bubble.x;
	candy.y = bubble.y;

	if (bubble.y - bubble.r <= 0.0f)		// top of the bubble touched the top of the screen
		Bubble_Pop();
}

void Bubble_Draw(void)
{
	if (bubble.state == BUBBLE_POPPED)
		return;

	CP_Settings_Fill(CP_Color_Create(80, 160, 255, 120));	// last number is alpha, so the candy shows through
	CP_Graphics_DrawCircle(bubble.x, bubble.y, bubble.r * 2.0f);
}

//Magnet on click and hold, pulls candy towards it, when candy touches the magnet, release the candy fall.
//Can be released early by letting go of the Magnet
typedef enum
{
	MAGNET_WAITING,
	MAGNET_CARRYING,
	MAGNET_POPPED
} MagnetState;

typedef struct
{
	float x;
	float y;
	float r;
	MagnetState state;
} Magnet;

static Magnet magnet;

void Magnet_Init(float x, float y) 
{
	magnet.x = x;
	magnet.y = y;
	magnet.r = MAGNET_RADIUS;
	magnet.state = MAGNET_WAITING;
}

// Gone: the candy falls from rest, so Candy_Update starts it again from vy = 0.
static void Magnet_Pop(void)
{
	magnet.state = MAGNET_POPPED;
	candy.vy = 0.0f;
}

void Magnet_TryCapture(void)
{
	if (magnet.state != MAGNET_WAITING)
		return;

	// If this is the first frame the left mouse button is pressed
	if (CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT))
	{

		// If left mouse button is being held
		if (CP_Input_MouseDown(MOUSE_BUTTON_LEFT) && IsCircleClicked(magnet.x, magnet.y, magnet.r * 2.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
		{
			magnet.state = MAGNET_CARRYING;
		}
		// If left mouse button was just released
		else if (CP_Input_MouseReleased(MOUSE_BUTTON_LEFT))
		{
			magnet.state = MAGNET_POPPED;
			candy.vy = 0.0f;
		}
	}
}

//Again this just checks if the magnet is carrying the candy, if it is, it returns 1, else 0
int Magnet_IsCarrying(void)
{
	return magnet.state == MAGNET_CARRYING;
}

void Magnet_Update()
{
	if (magnet.state != MAGNET_CARRYING || magnet.state != MAGNET_WAITING)
		return;
	if (magnet.x < 0)
	{
		candy.x -= 1;
		candy.y = magnet.y;
	}
	else if (magnet.x > 0)
	{
		candy.x += 1;
		candy.y = magnet.y;
	}

	if (magnet.y - magnet.r <= 0.0f)		// top of the magnet touched the top of the screen
		Magnet_Pop();
}

void Magnet_Draw(void)
{
	if (magnet.state == MAGNET_POPPED)
		return;

	CP_Settings_Fill(CP_Color_Create(80, 160, 255, 120));	// last number is alpha, so the candy shows through
	CP_Graphics_DrawCircle(magnet.x, magnet.y, magnet.r * 2.0f);
}