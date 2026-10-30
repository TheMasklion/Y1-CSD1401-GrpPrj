// file:	ropegame.h
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:Game state that shows the Verlet rope (rope.h / rope.c): anchor at the top centre, candy
//			hanging from the rope, pig at the bottom. The candy starts to swing out to one side, swings like a pendulum and settles;
//			the rope sags whenever got slack and never stretches.
//
//
// Controls:
// H (hold)	- show the hinges as dots
// Q - back to the main menu
// R - reset the rope back to original (Use for testing only)

#pragma once

#include "cprocessing.h"

void RopeGame_Init(void);		// builds anchor, rope, candy and pig for the current window size
void RopeGame_Update(void);		// one frame: physics, then drawing
void RopeGame_Exit(void);		// nothing to free
