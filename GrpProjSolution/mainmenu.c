//---------------------------------------------------------
// file:	mainmenu.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Main menu for solo proj 2, creates a main menu using rectangles and texts, to form buttons.
//			Utilizing utils.c functions onclick to interact with the shapes.
//
//---------------------------------------------------------

#include "utils.h"
#include "game.h"
#include "ropegame.h"	// ROPE: the rope game state (rope on the candy, fixed length)
#include "mainmenu.h"
#include "cprocessing.h"
#include <stdio.h>


#define GREY_BG CP_Color_Create(100, 100, 100, 255)
#define BLACK_BG CP_Color_Create(0, 0, 0, 255)
#define REDDISH_PINK_BG CP_Color_Create(255, 125, 125, 255)

#define FONT_SIZE 75.0f
#define FONT_ASSET "Assets/Exo2-Regular.ttf"

#define BTN_WIDTH 325.0f
#define BTN_HEIGHT 175.0f
#define BTN_X (float)(CP_System_GetWindowWidth() / 2)
#define BTN_Y_PLAY (float)(CP_System_GetWindowHeight() / 4.25f)
#define BTN_Y_EXIT (float)(CP_System_GetWindowHeight() / 1.45f)

// TEMP: Play2Game button that opens the old game.c state. Set to 0 to disable it.
#define ENABLE_PLAY2GAME 1
#if ENABLE_PLAY2GAME
#define BTN_Y_PLAY2 (float)(CP_System_GetWindowHeight() / 2.15f)	// sits between Play and Exit
#define FONT_SIZE_PLAY2 50.0f	// smaller so "Play2Game" fits inside the button
#endif

CP_Font myFont;


void Main_Menu_Init(void)
{
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
}

void Main_Menu_Update(void)
{
	//Drawing the main menu
	CP_Graphics_ClearBackground(GREY_BG);
	CP_Settings_Fill(REDDISH_PINK_BG); //Play Button colour
	CP_Graphics_DrawRect(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT); //Play Button
#if ENABLE_PLAY2GAME
	CP_Graphics_DrawRect(BTN_X, BTN_Y_PLAY2, BTN_WIDTH, BTN_HEIGHT); //TEMP: Play2Game Button
#endif
	CP_Graphics_DrawRect(BTN_X, BTN_Y_EXIT, BTN_WIDTH, BTN_HEIGHT); //Exit Button


	//Drawing the text for the buttons
	CP_Font_Set(myFont);
	CP_Settings_TextSize(FONT_SIZE);
	CP_Settings_Fill(BLACK_BG); //Play text colour
	CP_Font_DrawText("Play", BTN_X, BTN_Y_PLAY); //Play Button Text
	CP_Font_DrawText("Exit", BTN_X, BTN_Y_EXIT); //Exit Button Text)
#if ENABLE_PLAY2GAME
	CP_Settings_TextSize(FONT_SIZE_PLAY2);
	CP_Font_DrawText("Play2Game", BTN_X, BTN_Y_PLAY2); //TEMP: Play2Game Button Text
#endif


	//Checking if the buttons are clicked
	//float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y
	//printf("%d",IsAreaClicked(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()));
	if (CP_Input_MouseClicked() && IsAreaClicked(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(RopeGame_Init, RopeGame_Update, RopeGame_Exit);	// ROPE: Play opens the rope state (was Game_Init, Game_Update, Game_Exit)
	}

#if ENABLE_PLAY2GAME
	// TEMP: Play2Game opens the old game.c state (no rope physics)
	if (CP_Input_MouseClicked() && IsAreaClicked(BTN_X, BTN_Y_PLAY2, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
	}
#endif

	if (CP_Input_MouseClicked() && IsAreaClicked(BTN_X, BTN_Y_EXIT, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_Terminate();
	}


	/*
	CP_Settings_Fill(CP_Color_Create(255, 0, 0, 255));
	CP_Graphics_DrawCircle(100, 100, 50.0f);
	CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
	CP_Graphics_DrawCircle(100, 100, 50.0f);
	*/
	//shift++;

	//Testing area
}

void Main_Menu_Exit(void)
{
	CP_Font_Free(myFont);
}