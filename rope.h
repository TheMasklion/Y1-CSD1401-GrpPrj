// file:	rope.h
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:
// Every frame each free hinge is
// pulled down by its weight (mass * gravity) using Verlet
// integration, then the segments are pulled back to their rest
// length. Hinge point 0 is pinned to the anchor, rope hangs
// from it, swings and sags when it is slack. The free end is never
// let farther from the anchor than the rope is long.
// 
// Needs:
// 1. Weight (Gravity), where it is placed on the hinges on the rope.
// 2. Acceleration
// 3. Velocity
// 4. Rope is made from sticks connected to the hinges.


#pragma once //include guard that tells the compiler if header is included in current file, skip it.

#include "cprocessing.h"

#define ROPE_MAX_POINTS			64		// hinges per rope.
#define ROPE_DEFAULT_GRAVITY	980.0f	// pixels / second^2 downwards (9.8 m/s^2 at 100 px per metre)
#define ROPE_DEFAULT_DAMPING	0.99f	// velocity kept each frame (1.0 = almost no energy lost)
#define ROPE_DEFAULT_ITERATIONS	20		// constraint passes per frame (more = stiffer rope)

// One hinge of the rope.
typedef struct
{
	CP_Vector pos;			// where the hinge is now
	CP_Vector prev_pos;		// where it was last frame. Velocity = pos - prev_pos (Verlet stores no velocity).
	float mass;				// weight of the hinge. Heavier hinges move less when the rope pulls on them.
	int pinned;				// 1 = held in place (the anchor), 0 = free to move
} RopePoint;//labelling the struct to declare / call it easier for other functions.

// The creation of the rope. A plain value: declare it, call VerletRope_Init.
typedef struct
{
	RopePoint points[ROPE_MAX_POINTS];	// points[0] is the anchor end, points[point_count - 1] is the free end. Point[0] is the end of the rope that is connected to anchor point.
	int point_count;		// hinges in use, 2. ROPE_MAX_POINTS shows hinges in use.
	float segment_length;	// rest length of every segment = rope length / (point_count - 1)
	CP_Vector gravity;		// acceleration applied to every free hinge, pixels / second^2 (pixels per second), +y is down (based on gravity where opposite vector aka positive vector is acting downwards force)
	float damping;			// velocity kept in each frame, 0..1
	int iterations;			// constraint passes per update
} VerletRope;

// Builds the rope: point_count hinges in a straight line from anchor to end, hinge 0 pinned,
// every hinge mass 1, default gravity / damping / iterations. rope_length is the total rest
// length; longer than the anchor-to-end distance means the rope starts slack and sags.
// point_count is clamped to 2 point of hinges in ROPE_MAX_POINTS, rope_length to at least 1.
void VerletRope_Init(VerletRope* rope, CP_Vector anchor, CP_Vector end, float rope_length, int point_count);

// Sets the gravity vector (pixels / second^2 [pixels per second], +y down). Gravity vector included into here.
void VerletRope_SetGravity(VerletRope* rope, CP_Vector gravity);

// Sets the mass of the last hinge: how heavy the candy is compared to a rope hinge (mass 1). [Aka comparison of the rope's weight to the candy's weight.]
void VerletRope_SetEndMass(VerletRope* rope, float mass);

// Advances the rope by dt seconds (pass CP_System_GetDt()). Call once per frame, before drawing.
// dt is clamped to 1/30 s so a long frame cannot make the rope explode. The free end is kept
// within one rope length (one stick) of the anchor: the rope has a fixed length and never stretches past it.
void VerletRope_Update(VerletRope* rope, float dt);

// Position of the free end (where the candy is drawn). (0, 0) if rope is NULL. (Creating the end point (free end) of the rope)
CP_Vector VerletRope_GetEnd(const VerletRope* rope);

// Draws every segment as a line with round caps. Sets the stroke colour and weight. (Drawing of the rope)
void VerletRope_Draw(const VerletRope* rope, CP_Color color, float thickness);

// Draws a filled circle on every hinge, to show the hinges like the dots in the design sketch. (Shape of the hinges)
void VerletRope_DrawPoints(const VerletRope* rope, CP_Color color, float diameter);
