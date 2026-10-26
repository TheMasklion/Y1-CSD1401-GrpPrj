// file:	ropegame.c
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:Game state for the Verlet rope. See ropegame.h for the hook-up
//			line and the controls.
//
//			Frame order in RopeGame_Update:
//			  1. physics   - VerletRope_Update
//			  2. read back - the candy is drawn where the last hinge is
//			  3. draw      - background, rope, anchor, candy, pig
//
//			The start angle comes from CP_Matrix_Rotate (found in cprocessing.h)
//			instead of sin / cos.
#include "ropegame.h"
#include "rope.h"
#include "mainmenu.h"	// Main_Menu_* so Q can return to the menu (called)

// colours, same values as game.c so the state looks like the rest of the game
#define LIGHT_GREY_BG	CP_Color_Create(200, 200, 200, 255)
#define BROWN_BG		CP_Color_Create(120, 80, 40, 255)
#define RED_BG			CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG		CP_Color_Create(0, 255, 0, 255)
#define DARK_GREY_BG	CP_Color_Create(60, 60, 60, 255)

// layout and rope tuning
#define ANCHOR_RADIUS		12.0f
#define ANCHOR_Y			60.0f	// anchor sits this far below the top edge, horizontally centred
#define ROPE_LENGTH			300.0f	// total rest length of the rope in pixels
#define ROPE_POINTS			16		// hinges in the rope: more = smoother curve, a little more work per frame
#define ROPE_THICKNESS		4.0f
#define ROPE_START_ANGLE	45.0f	// degrees away from hanging straight down that the candy starts at
#define HINGE_DOT_DIAMETER	8.0f	// size of the hinge dots drawn while H is held
#define CANDY_RADIUS		25.0f
#define CANDY_MASS			8.0f	// much heavier than a rope hinge (1.0) so the rope follows the candy, not the other way round
#define PIG_RADIUS			50.0f
#define GRAVITY_X			0.0f	// gravity vector, pixels / second^2, +y is down.
#define GRAVITY_Y			980.0f	// When the group's shared gravity vector is ready, pass into RopeGame_Init instead.

typedef struct
{
	float x;
	float y;
	float r;
} Circle;

static Circle anchor;			// fixed point the rope hangs from
static Circle candy;			// drawn at the last hinge of the rope every frame
static Circle pig;				// bottom of the screen, under the anchor
static VerletRope rope;			// hinge 0 at the anchor, last hinge is the candy

void RopeGame_Init(void)
{
	float screen_w = (float)CP_System_GetWindowWidth();
	float screen_h = (float)CP_System_GetWindowHeight();
	CP_Vector anchor_pos;
	CP_Vector start_dir;
	CP_Vector candy_start;

	// anchor: top middle of the screen
	anchor.x = screen_w / 2.0f;
	anchor.y = ANCHOR_Y;
	anchor.r = ANCHOR_RADIUS;
	anchor_pos = CP_Vector_Set(anchor.x, anchor.y);

	// candy start: "straight down" (0, 1) rotated by the start angle, one rope length out,
	// so the rope swings as soon as the state starts. The angle's sign only picks the side.
	start_dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(ROPE_START_ANGLE), CP_Vector_Set(0.0f, 1.0f));
	candy_start = CP_Vector_Add(anchor_pos, CP_Vector_Scale(start_dir, ROPE_LENGTH));
	candy.x = candy_start.x;
	candy.y = candy_start.y;
	candy.r = CANDY_RADIUS;

	// rope: hinges from the anchor to the candy, anchor pinned, candy heavy, gravity as a vector
	VerletRope_Init(&rope, anchor_pos, candy_start, ROPE_LENGTH, ROPE_POINTS);
	VerletRope_SetEndMass(&rope, CANDY_MASS);
	VerletRope_SetGravity(&rope, CP_Vector_Set(GRAVITY_X, GRAVITY_Y));	// merge point for the group's gravity vector

	// pig: bottom of the screen, directly below the anchor
	pig.r = PIG_RADIUS;
	pig.x = anchor.x;
	pig.y = screen_h - pig.r;
}

void RopeGame_Update(void)
{
	CP_Vector rope_end;

	// 1. physics: every hinge is pulled down by its weight, the segments hold the rope together
	VerletRope_Update(&rope, CP_System_GetDt());

	// 2. the candy hangs from the last hinge
	rope_end = VerletRope_GetEnd(&rope);
	candy.x = rope_end.x;
	candy.y = rope_end.y;

	// 3. draw: rope first so the anchor and candy sit on top of it
	CP_Graphics_ClearBackground(LIGHT_GREY_BG);
	VerletRope_Draw(&rope, BROWN_BG, ROPE_THICKNESS);

	CP_Settings_Stroke(DARK_GREY_BG);		// thin outline on the shapes (DrawCircle takes a diameter)
	CP_Settings_StrokeWeight(2.0f);
	if (CP_Input_KeyDown(KEY_H))
		VerletRope_DrawPoints(&rope, GREEN_BG, HINGE_DOT_DIAMETER);
	CP_Settings_Fill(DARK_GREY_BG);
	CP_Graphics_DrawCircle(anchor.x, anchor.y, anchor.r * 2.0f);
	CP_Settings_Fill(RED_BG);
	CP_Graphics_DrawCircle(candy.x, candy.y, candy.r * 2.0f);
	CP_Settings_Fill(GREEN_BG);
	CP_Graphics_DrawCircle(pig.x, pig.y, pig.r * 2.0f);

	// Q: back to the main menu
	if (CP_Input_KeyDown(KEY_Q))
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
}

void RopeGame_Exit(void)
{
	// the rope is a plain struct with no heap memory: nothing to free
}
