//---------------------------------------------------------
// file:	Powerups.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Bubble power-up. Captures the free-falling candy, floats it up until the player clicks the bubble
//			or it reaches the top of the screen, then lets the candy fall normally again.
//---------------------------------------------------------

#include <math.h>	// fabsf
#include "Powerups.h"
#include "PigCandy.h"	// candy (extern)
#include "collision.h"
#include "utils.h"

CP_Image magnet_img;

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
	if (magnet_img == NULL)
		magnet_img = CP_Image_Load("Assets/Magnet.png");
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
		Magnet_Cancel();		// the bubble has priority: if the magnet was pulling the candy, it lets go
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

	// click on the bubble to pop it
	if (IsCircleClicked(bubble.x, bubble.y, bubble.r * 2.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		Bubble_Pop();
		return;
	}

	bubble.y -= BUBBLE_RISE_SPEED * dt;
	candy.x = bubble.x;
	candy.y = bubble.y;

	if (bubble.y - bubble.r <= 0.0f)		// top of the bubble touched the top of the screen
		Bubble_Pop();
}

void Bubble_Draw(void)
{
	if (bubble.state == BUBBLE_POPPED)
		return;

	CP_Settings_Fill(CP_Color_Create(80, 160, 255, 120));	// last number is alpha, so the candy shows, basically priority over the bubble
	CP_Graphics_DrawCircle(bubble.x, bubble.y, bubble.r * 2.0f);
}

// Magnet: click on it and keep holding the mouse button. While held, the magnet is ATTRACTING and checks every frame
// whether the free candy y level is equal to magnet y (candy Y within magnet Y +- magnet radius).
// The moment candy Y is = magnet Y, slide it sideways toward the magnet.
// It lets go when the candy touches the magnet or the mouse button is released, and then it can be clicked again.
// A bubble takes priority: if the candy touches a bubble, the bubble takes it (Magnet_Cancel).
typedef enum
{
	MAGNET_READY,		// switched off, waiting for a click on it
	MAGNET_ATTRACTING,	// mouse held since the click, waiting for the candy to be level with the magnet
	MAGNET_PULLING		// candy pulleds toward the magnet
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
	magnet.state = MAGNET_READY;
}

// Call every frame, whatever is holding the candy.
// Click on the magnet: it starts attracting. Let go of the mouse: it switches off again.
void Magnet_Poll(void)
{
	if (magnet.state == MAGNET_READY)
	{
		// The mouse is treated as a circle of radius 0, so the same overlap test works.
		if (CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT) &&
			IsCircleOverlapping(magnet.x, magnet.y, magnet.r, CP_Input_GetMouseX(), CP_Input_GetMouseY(), 0.0f))
			magnet.state = MAGNET_ATTRACTING;
		return;
	}

	if (!CP_Input_MouseDown(MOUSE_BUTTON_LEFT))
	{
		if (magnet.state == MAGNET_PULLING)
			candy.vy = 0.0f;		// let go early: the candy falls from rest
		magnet.state = MAGNET_READY;
	}
}

// Call when the candy is free (falling). If the magnet is attracting and the candy is level with it, start pulling.
void Magnet_TryCapture(void)
{
	if (magnet.state != MAGNET_ATTRACTING)
		return;

	// Check only the vertical position.
	if (fabsf(candy.y - magnet.y) <= magnet.r)
	{
		candy.vy = 0.0f;
		magnet.state = MAGNET_PULLING;
	}
}

//Again this just checks if the magnet is pulling the candy, if it is, it returns 1, else 0
int Magnet_IsCarrying(void)
{
	return magnet.state == MAGNET_PULLING;
}

// The bubble took the candy: the magnet stops pulling but stays attracting while the mouse is held,
// so it can pick the candy up again after the bubble pops.
void Magnet_Cancel(void)
{
	if (magnet.state == MAGNET_PULLING)
		magnet.state = MAGNET_ATTRACTING;
}

void Magnet_Update(float dt)
{
	float dx;
	float step;

	if (magnet.state != MAGNET_PULLING)
		return;

	// candy touched the magnet: let it go, it falls from rest. The magnet is ready to be clicked again.
	if (IsCircleOverlapping(magnet.x, magnet.y, magnet.r, candy.x, candy.y, candy.r))
	{
		magnet.state = MAGNET_READY;
		candy.vy = 0.0f;
		return;
	}

	// X axis only: the candy slides sideways toward the magnet and keeps its own height
	dx = magnet.x - candy.x;
	step = MAGNET_PULL_SPEED * dt;
	if (fabsf(dx) <= step)
		candy.x = magnet.x;
	else
		candy.x += (dx > 0.0f) ? step : -step;
}

void Magnet_Draw(void)
{
	float rotation = 90.0f;

	if (candy.x < magnet.x)
	{
		rotation = -90.0f;  // Candy on the left
	}

	CP_Image_DrawAdvanced(
		magnet_img,
		magnet.x,
		magnet.y,
		magnet.r * 4.0f,
		magnet.r * 4.0f,
		255,
		rotation
	);
}

void Magnet_Exit(void)
{
	if (magnet_img != NULL)
	{
		CP_Image_Free(magnet_img);
		magnet_img = NULL;
	}
}