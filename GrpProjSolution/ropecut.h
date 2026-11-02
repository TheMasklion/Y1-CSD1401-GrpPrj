// file:	ropecut.h
// author:	Kavian Wee
// email:	k.wee@digipen.edu
//
// Overview:
//		Cutting for the Verlet rope in rope.h / rope.c (NG JIA XUAN's section). Nothing in those two
//		files is changed. A cut shortens the game's rope in place (its point_count drops) and moves the
//		hinges below the cut into a "loose piece". A loose piece is itself a VerletRope with no pinned
//		hinge, so it is stepped by VerletRope_Update and drawn by VerletRope_Draw exactly like the
//		rope that is still on the anchor: same gravity vector, same constraint solver, same look.
//
//		Physics of one cut:
//		  1. Detect  - this frame's mouse drag (previous -> current position) is the blade. The first
//		               rope segment the blade crosses, or passes within blade_radius of, is cut.
//		  2. Split   - hinges 0..k stay on the anchor, hinges k+1..n-1 become a loose piece. Verlet
//		               stores velocity as pos - prev_pos, so every hinge keeps the velocity it had:
//		               the candy carries on along its swing and falls under the rope's own gravity.
//		  3. Recoil  - a taut rope is under tension. When it is cut the two new ends snap away from
//		               the cut (speed = recoil * tautness, tautness = end-to-end distance / rest length,
//		               so a slack rope barely twitches), and the blade pushes both ends a little in the
//		               direction it was moving.
//		  4. Fray    - a burst of short fibre particles (Euler integration with gravity and air drag)
//		               and a fading slash streak along the blade.
//		  5. Vanish  - (vanish_on, the default) once cut, the rope is finished: the stub still hanging
//		               from the anchor and the loose length both fade out over vanish_time (0.4 s)
//		               while they swing and fall, then they are removed. The candy comes off as a
//		               piece of its own, keeps the speed it had, and falls alone.
//		               With vanish_on = 0 the cut rope is kept instead: the loose length stays on
//		               the candy and can be cut again.
//		  6. Land    - a loose piece (the candy, or kept rope) falls until it meets the floor
//		               (floor_y, the bottom of the window by default), where it bounces a little,
//		               loses speed to friction and comes to rest. Only a piece that drifts out of the
//		               window sideways is removed.
//
//		The candy stays on whichever piece holds the original last hinge. RopeCutter_GetCandy says
//		where to draw it, RopeCutter_GetCandyState whether it is attached, falling, landed or gone.
//
// Hook-up (the lines marked CUT: in ropegame.c):
//		static RopeCutter cutter;                     next to the VerletRope
//		RopeCutter_Init(&cutter, &rope);              after VerletRope_Init / SetEndMass / SetGravity
//		cutter.candy_radius = CANDY_RADIUS;           so the candy rests on the floor on its rim
//		RopeCutter_HandleMouse(&cutter);              once per frame, after VerletRope_Update
//		RopeCutter_Update(&cutter, CP_System_GetDt());
//		candy = RopeCutter_GetCandy(&cutter);         in place of VerletRope_GetEnd(&rope)
//		RopeCutter_Draw(&cutter, colour, thickness);  right after VerletRope_Draw
//
//		Only CProcessing is used (vectors, matrix rotation, random, graphics, input).

#pragma once

#include "cprocessing.h"
#include "rope.h"

#define ROPECUT_MAX_LOOSE				16		// loose pieces alive at once (every cut adds one; pieces rest on the floor until R)
#define ROPECUT_MAX_FIBERS				96		// fibre particles alive at once
#define ROPECUT_MAX_SLASHES				24		// blade streak segments kept (one per drag frame)
#define ROPECUT_FIBERS_PER_CUT			12		// fibres thrown out by one cut
#define ROPECUT_DEFAULT_BLADE_RADIUS	6.0f	// px: a swipe this close to a segment still cuts it
#define ROPECUT_DEFAULT_RECOIL			420.0f	// px/s the cut ends snap away from the cut at full tautness
#define ROPECUT_DEFAULT_BLADE_PUSH		0.25f	// fraction of the blade's velocity handed to the cut ends
#define ROPECUT_DEFAULT_LOOSE_DAMPING	0.985f	// velocity a loose piece keeps per frame (rope.h: 0.99 on the anchor); a little extra air drag
#define ROPECUT_OFFSCREEN_MARGIN		200.0f	// px past the window edge before a loose piece is dropped
#define ROPECUT_SLASH_LIFE				0.16f	// seconds a blade streak segment stays visible
#define ROPECUT_DEFAULT_RECUT_COOLDOWN	0.25f	// seconds after a cut before the same piece can be cut again (one swipe = one cut)
#define ROPECUT_DEFAULT_FLOOR_BOUNCE	0.25f	// fraction of the downward speed a hinge keeps (upwards) when it hits the floor
#define ROPECUT_DEFAULT_FLOOR_FRICTION	0.35f	// fraction of the sideways speed a hinge loses each frame it touches the floor
#define ROPECUT_DEFAULT_VANISH_TIME		0.4f	// seconds a cut rope takes to fade out (vanish_on)

// Where the candy is: still on the anchored rope, on a loose piece in the air, on a loose piece that
// has touched the floor, or drifted out of the window.
typedef enum
{
	ROPECUT_CANDY_ATTACHED,
	ROPECUT_CANDY_FALLING,
	ROPECUT_CANDY_LANDED,
	ROPECUT_CANDY_GONE
} RopeCutCandyState;

// A length of rope that has been cut free, or the candy on its own. Only a fading stub keeps its
// hinge 0 pinned (it still hangs from the anchor while it fades); everything else falls.
typedef struct
{
	VerletRope rope;		// stepped by VerletRope_Update, drawn by VerletRope_Draw
	int has_candy;			// 1 = its last hinge is the candy
	int landed;				// 1 once any hinge has touched the floor
	int vanishing;			// 1 = fading out, ignores the blade, removed when vanish_left reaches 0
	float vanish_left;		// seconds of fade left (alpha = vanish_left / vanish_time)
	float age;				// seconds since it was cut loose
	float cooldown;			// seconds left before this piece may be cut again
} RopeCutPiece;

// A frayed strand thrown out by a cut: a short spinning line with its own simple physics.
typedef struct
{
	CP_Vector pos;
	CP_Vector vel;			// px/s
	float angle;			// degrees
	float spin;				// degrees/s
	float length;			// px
	float life;				// seconds left
	float max_life;			// seconds it started with (for the fade)
} RopeCutFiber;

// One frame of the blade's path, drawn as a fading streak.
typedef struct
{
	CP_Vector from;
	CP_Vector to;
	float life;				// seconds left, <= 0 = unused
} RopeCutSlash;

typedef struct
{
	VerletRope* attached;				// the game's rope (hinge 0 pinned). Shortened in place by a cut. Not owned.
	int candy_attached;					// 1 while the candy hangs from attached
	float attached_cooldown;			// seconds left before the attached rope may be cut again
	CP_Vector candy_pos;				// last known candy position, used once the candy is gone

	RopeCutPiece loose[ROPECUT_MAX_LOOSE];
	int loose_count;
	RopeCutFiber fibers[ROPECUT_MAX_FIBERS];
	int fiber_count;
	RopeCutSlash slashes[ROPECUT_MAX_SLASHES];
	int slash_next;						// ring buffer write index

	int dragging;						// left button was down last frame
	CP_Vector drag_prev;				// mouse position last frame while dragging
	float last_dt;						// step of the last update, used to turn impulses into px per frame

	float kill_y;						// a loose piece whose every hinge is past these is removed
	float kill_x_min;
	float kill_x_max;

	int vanish_on;						// 1 = a cut rope fades out and only the candy goes on (Init: on); 0 = the cut rope is kept
	int floor_on;						// 1 = loose pieces land on floor_y (Init: on)
	float floor_y;						// Init: the bottom edge of the window
	float candy_radius;					// Init: 0. Set it to the candy's radius so the candy rests on its rim

	// tuning, set to the ROPECUT_DEFAULT_* values by Init, change after Init if wanted
	float blade_radius;
	float recoil;
	float blade_push;
	float loose_damping;
	float recut_cooldown;
	float floor_bounce;
	float floor_friction;
	float vanish_time;

	int cuts_made;						// total cuts since Init
} RopeCutter;

// Points the cutter at the game's rope (call after the rope is built). The candy counts as attached,
// no loose pieces, defaults for the tuning, kill box from the current window size.
void RopeCutter_Init(RopeCutter* cutter, VerletRope* rope);

// Reads the left mouse button: while it is held, the move since last frame is the blade.
// Returns the number of cuts made this frame.
int RopeCutter_HandleMouse(RopeCutter* cutter);

// Cuts with an explicit blade segment (for other input schemes, or tests). Returns the number of cuts
// made: at most one per piece per call, none on a piece cut less than recut_cooldown seconds ago
// (so one swipe through the rope is one cut, however slowly the cursor moves on through the pieces).
// A cut never leaves a lone hinge unless it is the anchor or the candy. Adds a slash streak whether
// or not anything was hit.
int RopeCutter_Cut(RopeCutter* cutter, CP_Vector from, CP_Vector to);

// Steps the loose pieces (VerletRope_Update, then the floor), the fibres and the slash streaks by
// dt seconds, then drops any loose piece that has drifted out of the window sideways. The attached
// rope is NOT stepped here: the game keeps calling VerletRope_Update on it as before.
void RopeCutter_Update(RopeCutter* cutter, float dt);

// Draws the loose pieces with VerletRope_Draw in the given colour and thickness (a vanishing piece
// with its faded alpha), then the fibres (same colour, fading) and the slash streaks. The attached
// rope is NOT drawn here.
void RopeCutter_Draw(const RopeCutter* cutter, CP_Color rope_color, float thickness);

// Hinge dots on the loose pieces, companion to VerletRope_DrawPoints.
void RopeCutter_DrawPoints(const RopeCutter* cutter, CP_Color color, float diameter);

// Where the candy is: the attached rope's end, the end of the loose piece carrying it (in the air or
// resting on the floor), or its last position once it has drifted out of the window.
CP_Vector RopeCutter_GetCandy(const RopeCutter* cutter);
RopeCutCandyState RopeCutter_GetCandyState(const RopeCutter* cutter);

// Loose pieces currently in the air.
int RopeCutter_GetLooseCount(const RopeCutter* cutter);
