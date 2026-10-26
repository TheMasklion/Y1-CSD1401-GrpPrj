// file:	rope.c
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:
//			Three steps every frame, in VerletRope_Update:
//
//			1. Integrate (Rope_Integrate) - for every free hinge:
//			     velocity = (pos - prev_pos) * damping     velocity is just last frame's move
//			     prev_pos = pos
//			     pos      = pos + velocity + gravity * dt * dt
//			   The weight on a hinge is mass * gravity. Dividing that weight by
//			   the mass again gives its acceleration, which is simply gravity, so
//			   every hinge falls at the same rate however heavy it is. Because all
//			   the hinges are pulled down by their weight, a slack rope sinks into
//			   a curve and a hanging rope swings like a pendulum.
//
//			2. Constraints (Rope_SolveConstraints), repeated `iterations` times -
//			   for every segment: measure the distance between its two hinges and
//			   move them back together (or apart) so the segment is at its rest
//			   length. The move is shared by inverse mass, so the heavy candy
//			   barely moves and the light rope bends around it. The pinned anchor
//			   never moves.
//
//			3. Fixed length (Rope_ClampToReach) - the free end is never left farther
//			   from the anchor than the rope is long, so the candy cannot overshoot
//			   the rope on a fast swing; a real rope just goes taut.
//
// Hinge count and rope length are chosen by the caller (ropegame.c uses 16 hinges, 300 px).

#include "rope.h"

#define ROPE_MAX_DT			(1.0f / 30.0f)	// longest step allowed: a long frame becomes slow motion, prevents long frame to cause a slow motion from happening.
#define ROPE_MIN_MASS		0.001f			// masses are divided by, to keep them positive
#define ROPE_MIN_DISTANCE	0.0001f			// two hinges closer than this sit on top of each other (avoids divide by zero) (Prevents two hinges from being set on top of each other)


// Rope_Integrate: Verlet integration of every free hinge.
static void Rope_Integrate(VerletRope* rope, float dt)
{
	int i;
	for (i = 0; i < rope->point_count; ++i)
	{
		RopePoint* p = &rope->points[i];
		CP_Vector velocity;

		if (p->pinned)
			continue;

		// weight = mass * gravity, acceleration = weight / mass = gravity
		velocity = CP_Vector_Scale(CP_Vector_Subtract(p->pos, p->prev_pos), rope->damping);
		p->prev_pos = p->pos;
		p->pos = CP_Vector_Add(p->pos, CP_Vector_Add(velocity, CP_Vector_Scale(rope->gravity, dt * dt)));
	}
}

// Rope_SolveConstraints: makes the chain of hinges behave like a rope instead of a handful of loose falling points.
//   error      = (distance - rest) / distance   fraction of delta that is stretch (+) or squash (-)
//   correction = delta * error                  vector of exactly the excess length
// Each hinge moves its share of the correction, share = its inverse mass / (sum of both).
// A pinned hinge has inverse mass 0 (infinitely heavy) and takes none of it.

static void Rope_SolveConstraints(VerletRope* rope)
{
	int i;
	for (i = 0; i < rope->point_count - 1; ++i)
	{
		RopePoint* a = &rope->points[i];
		RopePoint* b = &rope->points[i + 1];
		CP_Vector delta = CP_Vector_Subtract(b->pos, a->pos);
		float distance = CP_Vector_Length(delta);
		float inv_a;
		float inv_b;
		float error;
		CP_Vector correction;

		if (distance < ROPE_MIN_DISTANCE)
			continue;	// no direction to push along

		inv_a = a->pinned ? 0.0f : 1.0f / a->mass;
		inv_b = b->pinned ? 0.0f : 1.0f / b->mass;
		if (inv_a + inv_b <= 0.0f)
			continue;	// both ends pinned, nothing may move

		error = (distance - rope->segment_length) / distance;
		correction = CP_Vector_Scale(delta, error);
		a->pos = CP_Vector_Add(a->pos, CP_Vector_Scale(correction, inv_a / (inv_a + inv_b)));
		b->pos = CP_Vector_Subtract(b->pos, CP_Vector_Scale(correction, inv_b / (inv_a + inv_b)));
	}
}


// Rope_ClampToReach: the rope has a fixed length, so no point of it can be farther
// from the anchor (hinge 0) than that. Pulls pos back onto the reach circle if it is outside.
// (reach is at least 1 px, so a point outside it is never on the anchor.)

static void Rope_ClampToReach(const VerletRope* rope, CP_Vector* pos)
{
	CP_Vector anchor = rope->points[0].pos;
	CP_Vector offset = CP_Vector_Subtract(*pos, anchor); //offset is the vector from the anchor to the point.
	float reach = rope->segment_length * (float)(rope->point_count - 1); // reach is the rope’s full length: 15 segments of 20 px, so 300 px.
	float distance = CP_Vector_Length(offset); //distance is length of rope.

	if (distance > reach)
		*pos = CP_Vector_Add(anchor, CP_Vector_Scale(offset, reach / distance));
}

// VerletRope_Init - lay the hinges out evenly from anchor to end, pin hinge 0.

void VerletRope_Init(VerletRope* rope, CP_Vector anchor, CP_Vector end, float rope_length, int point_count)
{
	int i;
	CP_Vector step;

	if (rope == NULL)
		return;
	if (point_count < 2)
		point_count = 2;
	if (point_count > ROPE_MAX_POINTS)
		point_count = ROPE_MAX_POINTS;
	if (rope_length < 1.0f)
		rope_length = 1.0f;

	rope->point_count = point_count;
	rope->segment_length = rope_length / (float)(point_count - 1);
	rope->gravity = CP_Vector_Set(0.0f, ROPE_DEFAULT_GRAVITY);
	rope->damping = ROPE_DEFAULT_DAMPING;
	rope->iterations = ROPE_DEFAULT_ITERATIONS;

	step = CP_Vector_Scale(CP_Vector_Subtract(end, anchor), 1.0f / (float)(point_count - 1));
	for (i = 0; i < point_count; ++i)
	{
		RopePoint* p = &rope->points[i];
		p->pos = CP_Vector_Add(anchor, CP_Vector_Scale(step, (float)i));
		p->prev_pos = p->pos;		// same as pos: starts at rest
		p->mass = 1.0f;
		p->pinned = (i == 0);		// only the anchor is pinned
	}
}


// VerletRope_SetGravity - Setting the gravity onto the rope hinges to have the pulling effect.

void VerletRope_SetGravity(VerletRope* rope, CP_Vector gravity)
{
	if (rope != NULL)
		rope->gravity = gravity;
}


// VerletRope_SetEndMass - the candy's weight, clamped so it can be divided by.
// Sets how heavy the last hinge of the rope is.
// When mass changes, every hinge falls at the same rate.

void VerletRope_SetEndMass(VerletRope* rope, float mass)
{
	if (rope == NULL)
		return;
	if (mass < ROPE_MIN_MASS)
		mass = ROPE_MIN_MASS;
	rope->points[rope->point_count - 1].mass = mass; // A zero or negative mass would divide by zero or make the hinge behave backwards. So the clamp makes every mass be positive.
}

// VerletRope_Update - clamp dt, integrate once, relax the constraints `iterations` times,
// then keep the free end within the rope's length.
// Updating rope's hinges and rope every position.

void VerletRope_Update(VerletRope* rope, float dt)
{
	int i;
	RopePoint* end;

	if (rope == NULL || dt <= 0.0f)
		return;
	if (dt > ROPE_MAX_DT)
		dt = ROPE_MAX_DT;

	Rope_Integrate(rope, dt);
	for (i = 0; i < rope->iterations; ++i)
		Rope_SolveConstraints(rope);

	end = &rope->points[rope->point_count - 1];
	if (!end->pinned)
		Rope_ClampToReach(rope, &end->pos);
}

// VerletRope_GetEnd - Getting the end part of the rope.

CP_Vector VerletRope_GetEnd(const VerletRope* rope)
{
	if (rope == NULL)
		return CP_Vector_Zero();
	return rope->points[rope->point_count - 1].pos;
}


// VerletRope_Draw - one line per segment. Round caps hide the joins between segments,

void VerletRope_Draw(const VerletRope* rope, CP_Color color, float thickness)
{
	int i;

	if (rope == NULL)
		return;

	CP_Settings_Stroke(color);
	CP_Settings_StrokeWeight(thickness);
	CP_Settings_LineCapMode(CP_LINE_CAP_ROUND);
	for (i = 0; i < rope->point_count - 1; ++i)
	{
		const RopePoint* a = &rope->points[i];
		const RopePoint* b = &rope->points[i + 1];
		CP_Graphics_DrawLine(a->pos.x, a->pos.y, b->pos.x, b->pos.y);
	}
	CP_Settings_LineCapMode(CP_LINE_CAP_BUTT);
}


// VerletRope_DrawPoints - a dot on every hinge (CP_Graphics_DrawCircle takes a diameter).

void VerletRope_DrawPoints(const VerletRope* rope, CP_Color color, float diameter)
{
	int i;

	if (rope == NULL)
		return;

	CP_Settings_Fill(color);
	for (i = 0; i < rope->point_count; ++i)
		CP_Graphics_DrawCircle(rope->points[i].pos.x, rope->points[i].pos.y, diameter);
}
