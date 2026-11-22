// file:	ropegame.c
// author:	Everyone
//
// Jia Xuans Notes:
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
//
//			LEVEL: the lines marked LEVEL: read the layout (anchors, pig, power-ups, stars) from the table in
//			       LevelData.c for current_level, instead of the fixed numbers this file used to hold.

#include "ropegame.h"
#include "rope.h"
#include "ropecut.h"	// CUT: cutting the rope, loose pieces, fray effect
#include "roperig.h"	// ROPE2: a second anchor and rope sharing the candy
#include "mainmenu.h"	// Main_Menu_* so Q can return to the menu (called)
#include "LevelSelect.h"
#include "collision.h"	// IsCircleOverlapping, for the candy-pig test
#include "gamestate.h"
#include "PigCandy.h"
#include "Powerups.h"	// Bubble, Magnet power-up
#include "Pause.h"      // Pause
#include "Stars.h" 		// Stars
#include "utils.h"		// Background image
#include "SaveData.h"
#include "LevelData.h"	// LEVEL: the per-level layout table

// colours, same values as game.c so the state looks like the rest of the game
#define LIGHT_GREY_BG	CP_Color_Create(200, 200, 200, 255)
#define BROWN_BG		CP_Color_Create(120, 80, 40, 255)
#define RED_BG			CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG		CP_Color_Create(0, 255, 0, 255)
#define DARK_GREY_BG	CP_Color_Create(60, 60, 60, 255)
#define YELLOW_BG		CP_Color_Create(255, 220, 0, 255)	// pig colour once the candy has touched it

// rope tuning shared by every level. The layout numbers (anchor positions, rope length, start angle,
// pig size, where the power-ups and stars sit) live in LevelData.c, one entry per level.
#define ANCHOR_RADIUS		12.0f
#define ROPE_POINTS			16		// hinges in the rope: more = smoother curve, a little more work per frame
#define ROPE_THICKNESS		4.0f
#define HINGE_DOT_DIAMETER	8.0f	// size of the hinge dots drawn while H is held
#define CANDY_RADIUS		19.0f
#define CANDY_MASS			8.0f	// much heavier than a rope hinge (1.0) so the rope follows the candy, not the other way round
#define GRAVITY_X			0.0f	// gravity vector, pixels / second^2, +y is down.
#define GRAVITY_Y			980.0f	// When the group's shared gravity vector is ready, pass into RopeGame_Init instead.

typedef struct
{
	float x;
	float y;
	float r;
} Circle;

static Circle anchor;
static Circle pig;
static VerletRope rope;
static RopeCutter cutter;
static RopeRig rig;
static CP_Image level_bg;

// LEVEL: the table entry being played
static const LevelData* level;
static int save_written;	// 1 once this run's stars have been written to the save file

//flag for pause and win state
typedef enum
{
	GamePlay,
	GameOver,
	GameWin
} GameState;

GameState game_state;

extern int current_level;
extern int star_count;

void RopeGame_Init(void)
{

	Pause_Init();

	CP_Vector anchor_pos;
	CP_Vector start_dir;
	int i;

	// LEVEL: pick the layout for the level chosen in the level select
	level = Level_Get(current_level);

	// anchor: the main rope's anchor from the table (top middle of the screen in level 1)
	anchor.x = level->anchor[0].x;
	anchor.y = level->anchor[0].y;
	anchor.r = ANCHOR_RADIUS;
	anchor_pos = CP_Vector_Set(anchor.x, anchor.y);

	// candy start: "straight down" (0, 1) rotated by the start angle, one rope length out,
	// so the rope swings as soon as the state starts. The angle's sign only picks the side.
	start_dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(level->start_angle), CP_Vector_Set(0.0f, 1.0f));
	candy_start = CP_Vector_Add(anchor_pos, CP_Vector_Scale(start_dir, level->rope_length));
	candy.r = CANDY_RADIUS;
	Candy_Reset();

	// rope: hinges from the anchor to the candy, anchor pinned, candy heavy, gravity as a vector
	VerletRope_Init(&rope, anchor_pos, candy_start, level->rope_length, ROPE_POINTS);
	VerletRope_SetEndMass(&rope, CANDY_MASS);
	VerletRope_SetGravity(&rope, CP_Vector_Set(GRAVITY_X, GRAVITY_Y));	// merge point for the group's gravity vector
	RopeCutter_Init(&cutter, &rope);	// CUT: after the rope is built; the cutter copies its gravity for the fray


	// ROPE2: every further anchor in the table gets a rope and a cutter built as copies of the ones above:
	//        same length and hinges, the rope's gravity, damping, iterations and candy mass, the cutter's
	//        floor, fade and blade settings. Level 1 has three anchors, level 2 two, level 3 three.
	RopeRig_Init(&rig, &rope, &cutter);
	for (i = 1; i < level->anchor_count; ++i)
		RopeRig_AddRope(&rig, CP_Vector_Set(level->anchor[i].x, level->anchor[i].y), level->rope_length, ROPE_POINTS);

	// pig: where the table puts it (bottom of the screen, directly below the anchor in level 1)
	pig.r = level->pig_radius;
	pig.x = level->pig.x;
	pig.y = level->pig.y;

	// sprites: loaded once (Init runs again on R and on candy lost), freed in RopeGame_Exit
	if (candy_img == NULL)
		candy_img = CP_Image_Load("Assets/Candy.png");
	if (pig_img == NULL)
		pig_img = CP_Image_Load("Assets/Pig.png");
	if (level_bg == NULL)
		level_bg = CP_Image_Load("Assets/levelbackground.png");
	CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_NEAREST);	// keeps pixel art crisp when scaled up

	// LEVEL: power-ups where the table puts them (a level that does not want one parks it off screen)
	Bubble_Init(level->bubble.x, level->bubble.y);
	Magnet_Init(level->magnet.x, level->magnet.y);

	game_state = GamePlay; // set the game state to gameplay when the game starts
	save_written = 0;

	Stars_Init(); // Initialize the star image
}

void RopeGame_Update(void)
{

	if (game_state == GamePlay)
	{
		Pause_Update();

		if (Pause_IsPaused())
		{
			return;
		}
	}
	if (game_state == GameOver)
	{
		GameOver_Screen();
		return;
	}
	else if (game_state == GameWin)
	{
		//update save file to new star, once (every star on the way to the pig has been counted by now)
		if (!save_written)
		{
			UpdateSave(current_level, star_count);
			save_written = 1;
		}
		GameWin_Screen();
		return;
	}


	CP_Vector rope_end;
	int i;

	float dt = CP_System_GetDt();


	// 1. physics: every hinge is pulled down by its weight, the segments hold the rope together
	VerletRope_Update(&rope, dt);
	RopeRig_Update(&rig, dt);			// ROPE2: steps the second rope, merges the shared candy, offers this frame's mouse move as
	//        the blade to BOTH cutters (so the HandleMouse line below finds no new movement)
	RopeCutter_HandleMouse(&cutter);					// CUT: left drag = blade, cuts rope / loose pieces it crosses
	RopeCutter_Update(&cutter, dt);		// CUT: loose pieces fall (VerletRope_Update), fray and streak fade

	// 2. the candy hangs from the last hinge
	if (!candy_reached_pig)
	{
		Magnet_Poll();				// every frame: a click on the magnet switches it on, letting go of the mouse switches it off
		if (RopeRig_GetCandyState(&rig) == ROPECUT_CANDY_ATTACHED)
		{
			rope_end = RopeRig_GetCandy(&rig);
			candy.x = rope_end.x;
			candy.y = rope_end.y;
			candy.vy = 0.0f;
		}
		else if (Bubble_IsCarrying())
		{
			Bubble_Update(dt);		// the bubble moves the candy up instead of gravity
		}
		else
		{
			if (Magnet_IsCarrying())
			{
				Magnet_Update(dt);		// the magnet pulls the candy sideways instead of gravity
			}
			else
			{
				Candy_Update(dt);
				Magnet_TryCapture();	// a magnet that is on pulls the free candy once it falls level with it
			}
			Bubble_TryCapture();	// bubble has priority: checked after the candy moved (falling or pulled), and it cancels the magnet
		}
		//Candy out of screen, display game over //*Jayden part links from here game over screen
		if (Candy_IsOffScreen())
		{
			game_state = GameOver; // set the game state to game over
		}
		// candy-pig collision: once set, the candy is gone
		if (IsCircleOverlapping(candy.x, candy.y, candy.r, pig.x, pig.y, pig.r)) //passing middle point and radius of both candy and pig to check if they are overlapping
		{
			candy_reached_pig = 1;
			pig_state = PIG_HAPPY;
			game_state = GameWin; // set the game state to game win
		}
	}




	// 3. draw: rope first so the anchor and candy sit on top of it
	DrawBackground(level_bg);
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
		CP_Image_Draw(candy_img, candy.x, candy.y, candy.r * 2.0f, candy.r * 2.0f, 255);

	{
		float frame_w = (float)CP_Image_GetWidth(pig_img);
		float frame_h = (float)CP_Image_GetHeight(pig_img) / PIG_FRAMES;
		float v0 = (float)pig_state * frame_h;		// top edge of the chosen frame

		Bubble_Draw();
		Magnet_Draw();

		CP_Image_DrawSubImage(pig_img, pig.x, pig.y, pig.r * 2.0f, pig.r * 2.0f,
			0.0f, v0, frame_w, v0 + frame_h, 255);
	}


	// LEVEL: the three stars where the table puts them for this level
	for (i = 0; i < LEVEL_STARS; ++i)
		Stars_Draw(i + 1, level->star[i].x, level->star[i].y);

	Star_Counter(); // Counts the stars collected and displays the count on the screen

	// R: new rope (CUT: handy once the candy has been dropped)
	if (CP_Input_KeyTriggered(KEY_R))
		RopeGame_Init();

	// Q: back to the main menu
	if (CP_Input_KeyTriggered(KEY_Q))
		CP_Engine_SetNextGameState(Level_Select_Init, Level_Select_Update, Level_Select_Exit);
}


void RopeGame_Exit(void)
{
	CP_Image_Free(candy_img);
	candy_img = NULL;
	CP_Image_Free(pig_img);
	pig_img = NULL;
	CP_Image_Free(level_bg);
	level_bg = NULL;

	Magnet_Exit();
	Stars_Exit(); // Free the star image


}
