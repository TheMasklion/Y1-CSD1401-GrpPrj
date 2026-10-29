// file:	game.c
// author:	NG JIA XUAN & GASPER
// email:	n.jiaxuan@digipen.edu
//
// Overview:Game state for the Verlet rope. See ropegame.h for the hook-up
//			line and the controls.
//
//			Frame order in Game_Update:
//			  1. physics   - VerletRope_Update (ENABLE_ROPE 1 only)
//			  2. candy     - read back from the last hinge (ENABLE_ROPE 1) or Candy_Update gravity (ENABLE_ROPE 0)
//			  3. draw      - background, rope, anchor, candy, pig
//
//			The start angle comes from CP_Matrix_Rotate (found in cprocessing.h)
//			instead of sin / cos.

// 		A copy of ropegame.c, but with the function names changed to Game_* so the main menu can call it, for Gasper's Testing purposes.
//
//		GASPER TEST CHANGE: set ENABLE_ROPE (below) to turn the rope on or off.
//		  1 = rope ON: the rope is simulated and drawn, and the candy hangs from its last hinge, same as ropegame.c.
//		  0 = rope OFF: no rope at all (not simulated, not drawn). The candy has its own position and fall speed, so
//		      gravity can be tested on its own. Gravity speeds the candy up every frame until it reaches MAX_FALL_SPEED.
//
//		Extra control:  R - put the candy back at its start position
//
//		COLLISION TEST: when the candy touches the pig (IsCircleOverlapping from collision.c), the pig turns yellow
//		and stays yellow until R is pressed.

#include "game.h"
#include "rope.h"
#include "mainmenu.h"	// Main_Menu_* so Q can return to the menu (called)
#include "collision.h"	// IsCircleOverlapping, for the candy-pig test

// colours, same values as game.c so the state looks like the rest of the game
#define LIGHT_GREY_BG	CP_Color_Create(200, 200, 200, 255)
#define BROWN_BG		CP_Color_Create(120, 80, 40, 255)
#define RED_BG			CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG		CP_Color_Create(0, 255, 0, 255)
#define DARK_GREY_BG	CP_Color_Create(60, 60, 60, 255)
#define YELLOW_BG		CP_Color_Create(255, 220, 0, 255)	// pig colour once the candy has touched it

//Gasper TEMP: 1 = rope on, candy hangs from it like ropegame.c.  0 = rope off, candy falls freely by its own gravity.
#define ENABLE_ROPE			1

// layout and rope tuning
#define ANCHOR_RADIUS		12.0f
#define ANCHOR_Y			60.0f	// anchor sits this far below the top edge, horizontally centred
#define ROPE_LENGTH			300.0f	// total rest length of the rope in pixels
#define ROPE_POINTS			16		// hinges in the rope: more = smoother curve, a little more work per frame
#define ROPE_THICKNESS		4.0f
#define ROPE_START_ANGLE	45.0f	// degrees away from hanging straight down that the candy starts at
#define HINGE_DOT_DIAMETER	8.0f	// size of the hinge dots drawn while H is held
#define CANDY_RADIUS		25.0f

#if ENABLE_ROPE
#define CANDY_MASS			8.0f	// much heavier than a rope hinge (1.0) so the rope follows the candy, not the other way round
#endif

#define PIG_RADIUS			50.0f
#define GRAVITY_X			0.0f	// gravity vector, pixels / second^2, +y is down.
#define GRAVITY_Y			980.0f	// When the group's shared gravity vector is ready, pass into Game_Init instead.
#define MAX_FALL_SPEED		1200.0f	// the free candy never falls faster than this, pixels / second
#define MAX_DT				(1.0f / 30.0f)	// longest frame step used, so a long frame cannot make the candy jump

typedef struct
{
	float x;
	float y;
	float r;
} Circle;

// The candy has its own fall speed because it is not held by the rope.
typedef struct
{
	float x;
	float y;
	float r;
	float vy;		// fall speed, pixels / second, +y is down
} FallingCandy;

static Circle anchor;			// fixed point the rope hangs from
static FallingCandy candy;		// moves by its own gravity, or follows the rope end when ENABLE_ROPE is 1
static CP_Vector candy_start;	// where the candy is placed on init and on R
static int candy_reached_pig;	// 1 once the candy has touched the pig, cleared on R
static Circle pig;				// bottom of the screen, under the anchor
#if ENABLE_ROPE
static VerletRope rope;			// hinge 0 at the anchor, last hinge is the candy
#endif

// Puts the candy back at its start position and stops it falling.
static void Candy_Reset(void)
{
	candy.x = candy_start.x;
	candy.y = candy_start.y;
	candy.vy = 0.0f;
	candy_reached_pig = 0;
}

#if !ENABLE_ROPE
// Gravity: speed builds up each frame until it hits the limit, then the candy moves by that speed.
static void Candy_Update(float dt)
{
	candy.vy += GRAVITY_Y * dt;
	if (candy.vy > MAX_FALL_SPEED)
		candy.vy = MAX_FALL_SPEED;
	candy.y += candy.vy * dt;
}
#endif

void Game_Init(void)
{
	float screen_w = (float)CP_System_GetWindowWidth();
	float screen_h = (float)CP_System_GetWindowHeight();
	CP_Vector anchor_pos;
	CP_Vector start_dir;

	// anchor: top middle of the screen
	anchor.x = screen_w / 2.0f;
	anchor.y = ANCHOR_Y;
	anchor.r = ANCHOR_RADIUS;
	anchor_pos = CP_Vector_Set(anchor.x, anchor.y);

	// candy start: "straight down" (0, 1) rotated by the start angle, one rope length out.
	// The angle's sign only picks the side.
	start_dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(ROPE_START_ANGLE), CP_Vector_Set(0.0f, 1.0f));
	candy_start = CP_Vector_Add(anchor_pos, CP_Vector_Scale(start_dir, ROPE_LENGTH));
	candy.r = CANDY_RADIUS;
	Candy_Reset();

#if ENABLE_ROPE
	// rope: hinges from the anchor to the candy, anchor pinned, candy heavy, gravity as a vector
	VerletRope_Init(&rope, anchor_pos, candy_start, ROPE_LENGTH, ROPE_POINTS);
	VerletRope_SetEndMass(&rope, CANDY_MASS);
	VerletRope_SetGravity(&rope, CP_Vector_Set(GRAVITY_X, GRAVITY_Y));	// merge point for the group's gravity vector
#endif

	// pig: bottom of the screen, directly below the anchor
	pig.r = PIG_RADIUS;
	pig.x = anchor.x;
	pig.y = screen_h - pig.r;
}

void Game_Update(void)
{
	float dt = CP_System_GetDt();

#if ENABLE_ROPE
	CP_Vector rope_end;
#endif

	if (dt > MAX_DT)
		dt = MAX_DT;

	// R: reset the candy for another test drop
	if (CP_Input_KeyTriggered(KEY_R))
		Candy_Reset();

#if ENABLE_ROPE
	// 1. physics: every hinge is pulled down by its weight, the segments hold the rope together
	VerletRope_Update(&rope, dt);

	// 2. the candy hangs from the last hinge
	rope_end = VerletRope_GetEnd(&rope);
	candy.x = rope_end.x;
	candy.y = rope_end.y;
#else
	// rope is off: the candy falls by its own gravity
	Candy_Update(dt);
#endif

	// candy-pig collision test: stays set after the first touch so the colour change can be seen
	if (IsCircleOverlapping(candy.x, candy.y, candy.r, pig.x, pig.y, pig.r))
		candy_reached_pig = 1;

	// 3. draw: rope first (when it is on) so the anchor and candy sit on top of it
	CP_Graphics_ClearBackground(LIGHT_GREY_BG);
#if ENABLE_ROPE
	VerletRope_Draw(&rope, BROWN_BG, ROPE_THICKNESS);
#endif

	CP_Settings_Stroke(DARK_GREY_BG);		// thin outline on the shapes (DrawCircle takes a diameter)
	CP_Settings_StrokeWeight(2.0f);
#if ENABLE_ROPE
	if (CP_Input_KeyDown(KEY_H))
		VerletRope_DrawPoints(&rope, GREEN_BG, HINGE_DOT_DIAMETER);
#endif
	CP_Settings_Fill(DARK_GREY_BG);
	CP_Graphics_DrawCircle(anchor.x, anchor.y, anchor.r * 2.0f);
	CP_Settings_Fill(RED_BG);
	CP_Graphics_DrawCircle(candy.x, candy.y, candy.r * 2.0f);
	CP_Settings_Fill(candy_reached_pig ? YELLOW_BG : GREEN_BG);	// yellow once the candy has touched the pig
	CP_Graphics_DrawCircle(pig.x, pig.y, pig.r * 2.0f);

	// Q: back to the main menu
	if (CP_Input_KeyDown(KEY_Q))
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
}

void Game_Exit(void)
{
	// the rope is a plain struct with no heap memory: nothing to free
}