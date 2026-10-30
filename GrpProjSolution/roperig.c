// file:	roperig.c
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:
// The extra ropes are built with VerletRope_Init and stepped / drawn with VerletRope_Update / VerletRope_Draw; the extra cutters with
// RopeCutter_Init / HandleMouse / Update / Draw.

#include "roperig.h"

// Every cut takes hinges off the end of the attached rope (ropecut.c, Cut_Split).
static int Rig_IsHolding(const RopeRig* rig, int i)
{
	return rig->rope[i]->point_count == rig->full_count[i];
}

static int Rig_HolderCount(const RopeRig* rig)
{
	int i;
	int n = 0;
	for (i = 0; i < rig->rope_count; ++i)
		n += Rig_IsHolding(rig, i);
	return n;
}

// Tells every cutter whether the candy is its to drop. RopeCutter_Cut hands the candy to the loose
// piece only when candy_attached is 1, so it is 1 for the last rope still holding the candy and 0 for
// every other cutter: a rope cut while another still holds the candy falls away without it.
static void Rig_AssignCandy(RopeRig* rig)
{
	int i;
	int holders = 0;
	int last = -1;

	for (i = 0; i < rig->rope_count; ++i)
	{
		if (Rig_IsHolding(rig, i))
		{
			++holders;
			last = i;
		}
	}
	if (holders == 1)
		rig->candy_owner = last;	// once this rope is cut, its cutter's piece carries the candy
	for (i = 0; i < rig->rope_count; ++i)
		rig->cutter[i]->candy_attached = (holders == 1 && i == last);
}

// Take two ropes in a V at angle theta from the vertical, candy at rest at the
// bottom. Gravity moves the candy down by g*dt*dt. Each rope's clamp removes the radial part of that
// move and leaves the tangential part, which points down and towards the rope's own anchor.
//
// prev_pos: Rope_Integrate sets prev_pos = the old pos on every rope, so the averaged prev_pos is that
// same old position and the candy's velocity next frame is (merged - old), one velocity for all ropes.
static void Rig_MergeCandy(RopeRig* rig)
{
	CP_Vector pos = CP_Vector_Zero();
	CP_Vector prev = CP_Vector_Zero();
	int n = 0;
	int i;

	for (i = 0; i < rig->rope_count; ++i)
	{
		const VerletRope* r = rig->rope[i];
		if (!Rig_IsHolding(rig, i))
			continue;
		pos = CP_Vector_Add(pos, r->points[r->point_count - 1].pos);
		prev = CP_Vector_Add(prev, r->points[r->point_count - 1].prev_pos);
		++n;
	}
	if (n == 0)
		return;		// every rope is cut: the candy is a loose piece in candy_owner's cutter

	pos = CP_Vector_Scale(pos, 1.0f / (float)n);
	prev = CP_Vector_Scale(prev, 1.0f / (float)n);
	for (i = 0; i < rig->rope_count; ++i)
	{
		VerletRope* r = rig->rope[i];
		if (!Rig_IsHolding(rig, i))
			continue;
		r->points[r->point_count - 1].pos = pos;
		r->points[r->point_count - 1].prev_pos = prev;
	}
	rig->candy_pos = pos;
}

// Every cutter adds a streak for every blade it is given. The game's cutter draws this swipe's streak;
// an extra cutter's copy of it is dropped so the streak is not drawn once per rope (twice as bright).
static void Rig_DropNewestSlash(RopeCutter* c, int slash_next_before)
{
	if (c->slash_next != slash_next_before)
		c->slashes[(c->slash_next + ROPECUT_MAX_SLASHES - 1) % ROPECUT_MAX_SLASHES].life = 0.0f;
}

// The candy flags are set first, so a rope cut while another still holds the candy lets go of it, and
// the last holder to be cut takes it. Returns the cuts made.
static int Rig_CutOne(RopeRig* rig, int i, int by_mouse, CP_Vector from, CP_Vector to)
{
	RopeCutter* c = rig->cutter[i];
	int slash_before = c->slash_next;
	int cuts;

	Rig_AssignCandy(rig);
	cuts = by_mouse ? RopeCutter_HandleMouse(c) : RopeCutter_Cut(c, from, to);
	if (i > 0)
		Rig_DropNewestSlash(c, slash_before);
	return cuts;
}

//Public Variable
void RopeRig_Init(RopeRig* rig, VerletRope* rope, RopeCutter* cutter)
{
	if (rig == NULL)
		return;

	rig->rope_count = 0;
	rig->candy_owner = -1;
	rig->candy_pos = CP_Vector_Zero();
	if (rope == NULL || cutter == NULL)
		return;

	rig->rope[0] = rope;
	rig->cutter[0] = cutter;
	rig->full_count[0] = rope->point_count;
	rig->anchor[0] = rope->points[0].pos;
	rig->rope_count = 1;
	rig->candy_pos = VerletRope_GetEnd(rope);
	Rig_AssignCandy(rig);		// one rope: it holds the candy.
}

int RopeRig_AddRope(RopeRig* rig, CP_Vector anchor, float rope_length, int point_count)
{
	const VerletRope* src;
	const RopeCutter* sc;
	VerletRope* r;
	RopeCutter* c;
	int i;

	if (rig == NULL || rig->rope_count < 1 || rig->rope_count >= ROPERIG_MAX_ROPES)
		return -1;
	i = rig->rope_count;
	src = rig->rope[0];
	sc = rig->cutter[0];
	r = &rig->extra_rope[i - 1];
	c = &rig->extra_cutter[i - 1];

	// the rope: from the new anchor to the candy.
	VerletRope_Init(r, anchor, VerletRope_GetEnd(src), rope_length, point_count);
	VerletRope_SetGravity(r, src->gravity); //gravity vector from gasper's part
	r->damping = src->damping;
	r->iterations = src->iterations;
	VerletRope_SetEndMass(r, src->points[src->point_count - 1].mass);	// the same candy weighs the same on every rope

	// the cutter: the game cutter's settings, so every rope cuts, recoils, frays, fades and lands alike
	RopeCutter_Init(c, r);
	c->kill_y = sc->kill_y;
	c->kill_x_min = sc->kill_x_min;
	c->kill_x_max = sc->kill_x_max;
	c->vanish_on = sc->vanish_on;
	c->floor_on = sc->floor_on;
	c->floor_y = sc->floor_y;
	c->candy_radius = sc->candy_radius;
	c->blade_radius = sc->blade_radius;
	c->recoil = sc->recoil;
	c->blade_push = sc->blade_push;
	c->loose_damping = sc->loose_damping;
	c->recut_cooldown = sc->recut_cooldown;
	c->floor_bounce = sc->floor_bounce;
	c->floor_friction = sc->floor_friction;
	c->vanish_time = sc->vanish_time;

	rig->rope[i] = r;
	rig->cutter[i] = c;
	rig->full_count[i] = r->point_count;
	rig->anchor[i] = anchor;
	rig->rope_count = i + 1;
	Rig_AssignCandy(rig);		// two or more holders now: no cutter may drop the candy on its own
	return i;
}

void RopeRig_Update(RopeRig* rig, float dt)
{
	int i;

	if (rig == NULL || rig->rope_count < 1)
		return;

	// 1. step the extra ropes (the game has just stepped rope 0)
	for (i = 1; i < rig->rope_count; ++i)
		VerletRope_Update(rig->rope[i], dt);

	// 2. one candy for all the ropes that still hold it
	Rig_MergeCandy(rig);

	// 3. this frame's mouse move is the blade, offered to every cutter; the game's own
	//    RopeCutter_HandleMouse(&cutter) call later this frame then finds no new movement
	for (i = 0; i < rig->rope_count; ++i)
		Rig_CutOne(rig, i, 1, CP_Vector_Zero(), CP_Vector_Zero());
	Rig_AssignCandy(rig);

	// 4. step the extra cutters' loose pieces, fray and streaks (the game steps cutter 0)
	for (i = 1; i < rig->rope_count; ++i)
		RopeCutter_Update(rig->cutter[i], dt);
}

int RopeRig_Cut(RopeRig* rig, CP_Vector from, CP_Vector to)
{
	int cuts = 0;
	int i;

	if (rig == NULL)
		return 0;
	for (i = 0; i < rig->rope_count; ++i)
		cuts += Rig_CutOne(rig, i, 0, from, to);
	Rig_AssignCandy(rig);
	return cuts;
}

CP_Vector RopeRig_GetCandy(const RopeRig* rig)
{
	int i;

	if (rig == NULL || rig->rope_count < 1)
		return CP_Vector_Zero();
	for (i = 0; i < rig->rope_count; ++i)
	{
		if (Rig_IsHolding(rig, i))
			return VerletRope_GetEnd(rig->rope[i]);	// every holder's last hinge is the merged candy
	}
	if (rig->candy_owner >= 0)
		return RopeCutter_GetCandy(rig->cutter[rig->candy_owner]);
	return rig->candy_pos;
}

RopeCutCandyState RopeRig_GetCandyState(const RopeRig* rig)
{
	if (rig == NULL || rig->rope_count < 1)
		return ROPECUT_CANDY_GONE;
	if (Rig_HolderCount(rig) > 0)
		return ROPECUT_CANDY_ATTACHED;
	if (rig->candy_owner >= 0)
		return RopeCutter_GetCandyState(rig->cutter[rig->candy_owner]);
	return ROPECUT_CANDY_GONE;
}

int RopeRig_GetHoldingCount(const RopeRig* rig)
{
	return (rig == NULL) ? 0 : Rig_HolderCount(rig);
}

void RopeRig_Draw(const RopeRig* rig, CP_Color rope_color, float thickness)
{
	int i;

	if (rig == NULL)
		return;
	for (i = 1; i < rig->rope_count; ++i)
	{
		VerletRope_Draw(rig->rope[i], rope_color, thickness);
		RopeCutter_Draw(rig->cutter[i], rope_color, thickness);
	}
}

void RopeRig_DrawPoints(const RopeRig* rig, CP_Color color, float diameter)
{
	int i;

	if (rig == NULL)
		return;
	for (i = 1; i < rig->rope_count; ++i)
	{
		VerletRope_DrawPoints(rig->rope[i], color, diameter);
		RopeCutter_DrawPoints(rig->cutter[i], color, diameter);
	}
}

void RopeRig_DrawAnchors(const RopeRig* rig, CP_Color color, float diameter)
{
	int i;

	if (rig == NULL)
		return;
	CP_Settings_Fill(color);
	for (i = 1; i < rig->rope_count; ++i)
		CP_Graphics_DrawCircle(rig->anchor[i].x, rig->anchor[i].y, diameter);
}
