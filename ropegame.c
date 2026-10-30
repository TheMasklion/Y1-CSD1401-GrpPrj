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
#define ANCHOR2_OFFSET_X	400.0f	// ROPE2: the second anchor sits this far to the side of the first, at the same height, on the
									//        side the candy starts. 400 px apart with two 300 px ropes puts the candy at the bottom of
									//        a V (each rope ~42 degrees from vertical) with the second rope a little slack at the start.

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
static RopeCutter cutter;		// CUT: cuts rope in place, owns the loose pieces and the fray effect
static RopeRig rig;				// ROPE2: the rope and cutter above plus the second anchor's own rope and cutter, one shared candy

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
	RopeCutter_Init(&cutter, &rope);	// CUT: after the rope is built; the cutter copies its gravity for the fray
	cutter.candy_radius = CANDY_RADIUS;	// CUT: cut rope lands on the window bottom and stays; the candy rests on its rim

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

	// 1. physics: every hinge is pulled down by its weight, the segments hold the rope together
	VerletRope_Update(&rope, CP_System_GetDt());
	RopeRig_Update(&rig, CP_System_GetDt());			// ROPE2: steps the second rope, merges the shared candy, offers this frame's mouse move as
														//        the blade to BOTH cutters (so the HandleMouse line below finds no new movement)
	RopeCutter_HandleMouse(&cutter);					// CUT: left drag = blade, cuts rope / loose pieces it crosses
	RopeCutter_Update(&cutter, CP_System_GetDt());		// CUT: loose pieces fall (VerletRope_Update), fray and streak fade

	// 2. the candy hangs from the last hinge
	rope_end = RopeRig_GetCandy(&rig);			// ROPE2: was RopeCutter_GetCandy(&cutter): the shared end while any rope holds the candy, the falling piece's end once both are cut
	candy.x = rope_end.x;
	candy.y = rope_end.y;

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
	CP_Settings_Fill(RED_BG);
	CP_Graphics_DrawCircle(candy.x, candy.y, candy.r * 2.0f);
	CP_Settings_Fill(GREEN_BG);
	CP_Graphics_DrawCircle(pig.x, pig.y, pig.r * 2.0f);

	// R: new rope (CUT: handy once the candy has been dropped)
	if (CP_Input_KeyTriggered(KEY_R))
		RopeGame_Init();

	// Q: back to the main menu
	if (CP_Input_KeyDown(KEY_Q))
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
}

void RopeGame_Exit(void)
{

}
