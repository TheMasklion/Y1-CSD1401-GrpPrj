//---------------------------------------------------------
// file:	Powerups.h
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Bubble power-up. Captures the free-falling candy, floats it up until the player clicks the bubble
//			or it reaches the top of the screen, then lets the candy fall normally again.
//
//			Magnet power-up. Press and hold the mouse on the magnet while the candy is free and level with it (same Y,
//			within the magnet's radius, and no further than MAGNET_RANGE away in X) and it pulls the candy sideways towards it. It lets go when the candy touches it
//			or the mouse is released, and can be used again. A bubble takes priority over the magnet.
//---------------------------------------------------------

#pragma once

#include "cprocessing.h"

#define BUBBLE_RADIUS		35.0f	// bigger than CANDY_RADIUS so the candy fits inside
#define BUBBLE_RISE_SPEED	150.0f	// pixels / second, upward
#define MAGNET_RADIUS		30.0f	// bigger than CANDY_RADIUS so the candy fits inside
#define MAGNET_PULL_SPEED	300.0f	// pixels / second, how fast the candy is pulled toward the magnet

extern CP_Image magnet_img;

void Bubble_Init(float x, float y);	// places a waiting bubble. Call from RopeGame_Init so R resets it too
void Bubble_TryCapture(void);		// if the bubble is waiting and the candy touches it, the bubble takes the candy
int  Bubble_IsCarrying(void);		// 1 while the candy is inside the bubble
void Bubble_Update(float dt);		// moves bubble and candy up, handles click-to-pop and the top edge
void Bubble_Draw(void);

void Magnet_Init(float x, float y);	// places a switched-off magnet. Call from RopeGame_Init so R and every level sets it up
void Magnet_Poll(void);				// call every frame: click on the magnet switches it on, letting go of the mouse switches it off
void Magnet_TryCapture(void);		// call while the candy is free: if the magnet is on and the candy is level with it (Y within MAGNET_RADIUS, X within MAGNET_RANGE), it starts pulling
int  Magnet_IsCarrying(void);		// 1 while the magnet is pulling the candy
void Magnet_Update(float dt);		// slides the candy sideways (X only) toward the magnet, lets go when it touches
void Magnet_Cancel(void);			// stops pulling (the bubble calls this when it takes the candy); the magnet stays on while the mouse is held
void Magnet_Draw(void);
void Magnet_Exit(void);