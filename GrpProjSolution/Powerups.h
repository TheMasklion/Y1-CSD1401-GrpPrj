//---------------------------------------------------------
// file:	Powerups.h
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Bubble power-up. Captures the free-falling candy, floats it up until the player clicks the bubble
//			or it reaches the top of the screen, then lets the candy fall normally again.
//---------------------------------------------------------

#pragma once

#include "cprocessing.h"

#define BUBBLE_RADIUS		35.0f	// bigger than CANDY_RADIUS so the candy fits inside
#define BUBBLE_RISE_SPEED	150.0f	// pixels / second, upward
#define MAGNET_RADIUS		30.0f	// bigger than CANDY_RADIUS so the candy fits inside

void Bubble_Init(float x, float y);	// places a waiting bubble. Call from RopeGame_Init so R resets it too
void Bubble_TryCapture(void);		// if the bubble is waiting and the candy touches it, the bubble takes the candy
int  Bubble_IsCarrying(void);		// 1 while the candy is inside the bubble
void Bubble_Update(float dt);		// moves bubble and candy up, handles click-to-pop and the top edge
void Bubble_Draw(void);

void Magnet_Init(float x, float y);
void Magnet_TryCapture(void);	
int  Magnet_IsCarrying(void);	
void Magnet_Update();		
void Magnet_Draw(void);