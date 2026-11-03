// file:	ropegame.c
// author:	NG JIA XUAN
// email:	n.jiaxuan@digipen.edu
//
// Overview:Game state for the Verlet rope. See ropegame.h for the hook-up
//			line and the controls.
//
//			Frame order in RopeGame_Update:
//			  1. physics   - VerletRope_Update, then the cutter (blade, loose pieces, fray)
//			  2. read back - the candy is drawn where the last hinge is
//			  3. draw      - background, rope, loose pieces, anchor, candy, pig
//
//			The start angle comes from CP_Matrix_Rotate (found in cprocessing.h)
//			instead of sin / cos.
//
//			CUT: the lines marked CUT: hook ropecut.h in. Left mouse drag cuts the rope, R restarts.
//			ROPE2: the lines marked ROPE2: hook roperig.h in: a second anchor off to the side with its own rope
//			       (same rope physics, same cutting) on the same candy. Cut one rope and the candy swings on the
//			       other; cut both (or swipe through both) and it falls.
#include "ropegame.h"
#include "rope.h"
#include "ropecut.h"	// CUT: cutting the rope, loose pieces, fray effect
#include "roperig.h"	// ROPE2: a second anchor and rope sharing the candy
#include "mainmenu.h"	// Main_Menu_* so Q can return to the menu (called)
#include "LevelSelect.h"
#include "collision.h"	// IsCircleOverlapping, for the candy-pig test

// colours, same values as game.c so the state looks like the rest of the game
#define LIGHT_GREY_BG	CP_Color_Create(200, 200, 200, 255)
#define BROWN_BG		CP_Color_Create(120, 80, 40, 255)
#define RED_BG			CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG		CP_Color_Create(0, 255, 0, 255)
#define DARK_GREY_BG	CP_Color_Create(60, 60, 60, 255)
#define YELLOW_BG		CP_Color_Create(255, 220, 0, 255)	// pig colour once the candy has touched it


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
#define ANCHOR2_OFFSET_X	400.0f	// ROPE2: the second anchor sits this far to the side of the first, at the same height, on the
									//        side the candy starts. 400 px apart with two 300 px ropes puts the candy at the bottom of
									//        a V (each rope ~42 degrees from vertical) with the second rope a little slack at the start.
#define MAX_FALL_SPEED	1200.0f			// the free candy never falls faster than this, pixels / second
#define MAX_DT			(1.0f / 30.0f)	// longest frame step used, so a long frame cannot make the candy jump
typedef struct
{
	float x;
	float y;
	float r;
} Circle;

// The candy has its own fall speed because it is not held by the rope once cut.
typedef struct
{
	float x;
	float y;
	float r;
	float vy;		// fall speed, pixels / second, +y is down
} FallingCandy;

static Circle anchor;
static FallingCandy candy;		// follows the rope end while attached, moves by its own gravity once cut
static CP_Vector candy_start;	// where the candy is placed on init
static int candy_reached_pig;	// 1 once the candy has touched the pig
static Circle pig;
static VerletRope rope;
static RopeCutter cutter;
static RopeRig rig;

// Resets the candy to its start position, clears the candy_reached_pig flag, and sets the candy's fall speed to 0.
static void Candy_Reset(void)
{
	candy.x = candy_start.x;
	candy.y = candy_start.y;
	candy.vy = 0.0f;
	candy_reached_pig = 0;
}

// Gravity: speed builds up each frame until it hits the limit, then the candy moves by that speed.
static void Candy_Update(float dt)
{
	candy.vy += GRAVITY_Y * dt;
	if (candy.vy > MAX_FALL_SPEED)
		candy.vy = MAX_FALL_SPEED;
	candy.y += candy.vy * dt;
}

void RopeGame_Init(void)
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

	// candy start: "straight down" (0, 1) rotated by the start angle, one rope length out,
	// so the rope swings as soon as the state starts. The angle's sign only picks the side.
	start_dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(ROPE_START_ANGLE), CP_Vector_Set(0.0f, 1.0f));
	candy_start = CP_Vector_Add(anchor_pos, CP_Vector_Scale(start_dir, ROPE_LENGTH));
	candy.x = candy_start.x;
	candy.y = candy_start.y;
	candy.r = CANDY_RADIUS;

	candy_start = CP_Vector_Add(anchor_pos, CP_Vector_Scale(start_dir, ROPE_LENGTH));
	candy.r = CANDY_RADIUS;
	Candy_Reset();

	// rope: hinges from the anchor to the candy, anchor pinned, candy heavy, gravity as a vector
	VerletRope_Init(&rope, anchor_pos, candy_start, ROPE_LENGTH, ROPE_POINTS);
	VerletRope_SetEndMass(&rope, CANDY_MASS);
	VerletRope_SetGravity(&rope, CP_Vector_Set(GRAVITY_X, GRAVITY_Y));	// merge point for the group's gravity vector
	RopeCutter_Init(&cutter, &rope);	// CUT: after the rope is built; the cutter copies its gravity for the fray
	

	// Anchor Point 2 and Rope: second anchor on the side the candy starts (so its rope reaches the candy), with a rope and a
	//        cutter built as copies of the ones above: same length and hinges, the rope's gravity, damping,
	//        iterations and candy mass, the cutter's floor, fade and blade settings.
	RopeRig_Init(&rig, &rope, &cutter);
	RopeRig_AddRope(&rig, CP_Vector_Set(candy.x - 150.0f, candy.y - 100.0f), ROPE_LENGTH, ROPE_POINTS);
	//Anchor Point 3 and Rope
	RopeRig_AddRope(&rig, CP_Vector_Set(candy.x + 400.0f, candy.y - 150.0f), ROPE_LENGTH, ROPE_POINTS);	// for the third anchor, where it sets the position and rope is connected to anchor point 3 with the same cutting effect.
	//Copy and paste the RopeRig_AddRope if want more anchor points attacted to the candy with the same cutting rope effect. As it is just a replicate of the first 2 anchor points.
	
	// pig: bottom of the screen, directly below the anchor
	pig.r = PIG_RADIUS;
	pig.x = anchor.x;
	pig.y = screen_h - pig.r;
}

void RopeGame_Update(void)
{
	CP_Vector rope_end;
	float dt = CP_System_GetDt();
	if (dt > MAX_DT)
		dt = MAX_DT;

	// 1. physics: every hinge is pulled down by its weight, the segments hold the rope together
	VerletRope_Update(&rope, dt);
	RopeRig_Update(&rig, dt);			// ROPE2: steps the second rope, merges the shared candy, offers this frame's mouse move as
														//        the blade to BOTH cutters (so the HandleMouse line below finds no new movement)
	RopeCutter_HandleMouse(&cutter);					// CUT: left drag = blade, cuts rope / loose pieces it crosses
	RopeCutter_Update(&cutter, dt);		// CUT: loose pieces fall (VerletRope_Update), fray and streak fade

	// 2. the candy hangs from the last hinge
	if (!candy_reached_pig)
	{
		if (RopeRig_GetCandyState(&rig) == ROPECUT_CANDY_ATTACHED)
		{
			rope_end = RopeRig_GetCandy(&rig);
			candy.x = rope_end.x;
			candy.y = rope_end.y;
			candy.vy = 0.0f;
		}
		else
		{
			Candy_Update(dt);
		}

		// candy-pig collision: once set, the candy is gone
		if (IsCircleOverlapping(candy.x, candy.y, candy.r, pig.x, pig.y, pig.r))
			candy_reached_pig = 1;
	}

	// candy-pig collision: stays set after the first touch
	if (IsCircleOverlapping(candy.x, candy.y, candy.r, pig.x, pig.y, pig.r))
		candy_reached_pig = 1;

	// 3. draw: rope first so the anchor and candy sit on top of it
	CP_Graphics_ClearBackground(LIGHT_GREY_BG);
	VerletRope_Draw(&rope, BROWN_BG, ROPE_THICKNESS);
	RopeRig_Draw(&rig, BROWN_BG, ROPE_THICKNESS);		// ROPE2: the second rope and its cut pieces and fray, same colour
	RopeCutter_Draw(&cutter, BROWN_BG, ROPE_THICKNESS);	// CUT: loose pieces in the same colour, then fray and blade streak

	CP_Settings_Stroke(DARK_GREY_BG);		// thin outline on the shapes (DrawCircle takes a diameter)
	CP_Settings_StrokeWeight(2.0f);
	if (CP_Input_KeyDown(KEY_H))
	{
		VerletRope_DrawPoints(&rope, GREEN_BG, HINGE_DOT_DIAMETER);
		RopeCutter_DrawPoints(&cutter, GREEN_BG, HINGE_DOT_DIAMETER);	// CUT: hinges of the loose pieces too
		RopeRig_DrawPoints(&rig, GREEN_BG, HINGE_DOT_DIAMETER);			// ROPE2: hinges of the second rope and its pieces
	}
	CP_Settings_Fill(DARK_GREY_BG);
	CP_Graphics_DrawCircle(anchor.x, anchor.y, anchor.r * 2.0f);
	RopeRig_DrawAnchors(&rig, DARK_GREY_BG, anchor.r * 2.0f);	// ROPE2: the second anchor, same look
	if (!candy_reached_pig)
	{
		CP_Settings_Fill(RED_BG);
		CP_Graphics_DrawCircle(candy.x, candy.y, candy.r * 2.0f);
	}
	CP_Settings_Fill(candy_reached_pig ? YELLOW_BG : GREEN_BG);
	CP_Graphics_DrawCircle(pig.x, pig.y, pig.r * 2.0f);


	// R: new rope (CUT: handy once the candy has been dropped)
	if (CP_Input_KeyTriggered(KEY_R))
		RopeGame_Init();

	// Q: back to the main menu
	if (CP_Input_KeyTriggered(KEY_Q))
		CP_Engine_SetNextGameState(Level_Select_Init, Level_Select_Update, Level_Select_Exit);
}

void RopeGame_Exit(void)
{

}
