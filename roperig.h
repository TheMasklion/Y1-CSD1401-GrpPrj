// file:	roperig.h
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:
//		A second (apply same for third anchor) anchor for the candy. Every extra anchor gets its own VerletRope (rope.h,
//		rope.c physics) and its own RopeCutter (ropecut.h, unchanged), so each extra
//		rope swings, sags, goes taut and is cut exactly like the first one.
//
//		The rig don't own the game's first rope and cutter: ropegame.c builds, steps and draws those
//		exactly as before. The rig owns only the ropes it adds (RopeRig_AddRope), which it builds as copies of the first anchor: same hinge count and length,
//		the first rope's gravity vector, damping, iterations and candy mass, the first cutter's blade, recoil, floor, fade and vanish settings.
//
//		One shared candy, every frame (RopeRig_Update):
//		  1. Step    - each extra rope is stepped with VerletRope_Update (the game has just stepped its
//		               own rope). A rope's last hinge is its own copy of the candy, so each rope works out
//		               where IT would put the candy this frame.
//		  2. Merge   - the candy goes to the average of those proposals, and that one position (prev_pos
//		               too, so the candy has one velocity) is written back into the last hinge of every
//		               rope that still holds it. Verlet is position based, so this is just one more
//		               constraint projection: a taut rope has already clamped its proposal onto its reach
//		               circle (rope.c, Rope_ClampToReach), a slack one hardly moves it, and the average
//		               settles where the ropes agree to within a fraction of a pixel (the maths is in
//		               roperig.c, above Rig_MergeCandy). Hanging from two ropes the candy sits at the
//		               bottom of the V; cut one rope and it swings on the other as a plain pendulum.
//		  3. Cut     - the mouse blade is offered to every cutter in turn (RopeCutter_HandleMouse), the
//		               game's included. Before each one the rig sets the cutter's candy_attached flag:
//		               1 only for the rope that is the LAST one still holding the candy. So cutting one
//		               of two ropes lets that rope fall away from the candy and fade (vanish mode) while
//		               the candy stays on the other rope; cutting the last rope lets the candy go with
//		               the swing speed it had, exactly as a single rope did in Version 4. One swipe is
//		               one blade for every rope, so a swipe through both ropes drops the candy at once.
//		  4. Pieces  - each extra cutter is stepped (RopeCutter_Update) so its loose pieces fall, land,
//		               fray and fade like the first cutter's, which the game steps itself.

#pragma once

#include "cprocessing.h"
#include "rope.h"
#include "ropecut.h"

#define ROPERIG_MAX_ROPES	4		// the game's rope plus up to three added anchors

typedef struct
{
	VerletRope* rope[ROPERIG_MAX_ROPES];		// rope[0] is the game's own rope (not owned), the rest point into extra_rope
	RopeCutter* cutter[ROPERIG_MAX_ROPES];		// cutter[0] is the game's own cutter (not owned), the rest into extra_cutter
	int full_count[ROPERIG_MAX_ROPES];			// point_count of each rope while whole: fewer hinges = it has been cut
	CP_Vector anchor[ROPERIG_MAX_ROPES];		// where each rope is pinned (for RopeRig_DrawAnchors)
	int rope_count;								// ropes in use, 1..ROPERIG_MAX_ROPES (1 = just the game's rope)

	VerletRope extra_rope[ROPERIG_MAX_ROPES - 1];		// storage for the added ropes
	RopeCutter extra_cutter[ROPERIG_MAX_ROPES - 1];		// storage for the added cutters

	int candy_owner;		// index of the cutter whose piece carries (or will carry) the candy once the last rope is cut, -1 = two or more ropes still hold it
	CP_Vector candy_pos;	// the merged candy position of the last update
} RopeRig;

// Points the rig at the game's rope and cutter. Call after RopeCutter_Init and after the game has set
// cutter.candy_radius etc., because RopeRig_AddRope copies those settings into the ropes it adds.
void RopeRig_Init(RopeRig* rig, VerletRope* rope, RopeCutter* cutter);

// Adds an anchor at `anchor` with a rope of rope_length pixels and point_count hinges running to the
// candy (the end of the game's rope). The rope copies the game rope's gravity, damping, iterations and
// candy mass; its cutter copies the game cutter's settings. Returns the rope's index (1, 2, ...) or -1
// if the rig is full or has not been initialised. Place the anchor within rope_length of the candy, or
// the rope starts stretched and snaps taut on the first frame.
int RopeRig_AddRope(RopeRig* rig, CP_Vector anchor, float rope_length, int point_count);

// One frame: steps the extra ropes, merges the shared candy into every rope that still holds it, offers
// this frame's mouse move as a blade to every cutter (the game's first), steps the extra cutters.
// Call right after VerletRope_Update on the game's rope and before RopeCutter_HandleMouse(&cutter).
void RopeRig_Update(RopeRig* rig, float dt);

// Cuts with an explicit blade segment on every rope and loose piece it crosses (for other input schemes,
// or tests), with the same candy hand-over as the mouse path. Returns the total number of cuts.
int RopeRig_Cut(RopeRig* rig, CP_Vector from, CP_Vector to);

// Where the candy is: the shared end while any rope holds it, else wherever the cutter that let it go
// says it is (falling, landed or gone).
CP_Vector RopeRig_GetCandy(const RopeRig* rig);
RopeCutCandyState RopeRig_GetCandyState(const RopeRig* rig);

// Ropes still running whole from their anchor to the candy.
int RopeRig_GetHoldingCount(const RopeRig* rig);

// Draws the extra ropes (VerletRope_Draw) and their cutters' loose pieces, fray and streaks
// (RopeCutter_Draw). The game's own rope and cutter are drawn by the game as before.
void RopeRig_Draw(const RopeRig* rig, CP_Color rope_color, float thickness);

// Hinge dots on the extra ropes and their loose pieces, companion to VerletRope_DrawPoints.
void RopeRig_DrawPoints(const RopeRig* rig, CP_Color color, float diameter);

// A filled circle on every extra anchor (CP_Graphics_DrawCircle takes a diameter). The first anchor is
// drawn by the game.
void RopeRig_DrawAnchors(const RopeRig* rig, CP_Color color, float diameter);
