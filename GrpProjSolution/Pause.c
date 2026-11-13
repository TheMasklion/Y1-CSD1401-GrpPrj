//---------------------------------------------------------
// file:	Pause.c
// author:	Kavian Wee
// email:	k.wee@digipen.edu
//
// brief:	Pause state for the game. This state is entered when the player presses the 'Esc' key during gameplay.
//
//---------------------------------------------------------

#include <stdio.h>
#include "utils.h"
#include "Pause.h"
#include "cprocessing.h"	
#include "ropegame.h"
#include "mainmenu.h"
#include "LevelSelect.h"


#define GREY_BG CP_Color_Create(100, 100, 100, 255)
#define BLACK_BG CP_Color_Create(0, 0, 0, 255)
#define REDDISH_PINK_BG CP_Color_Create(255, 125, 125, 255)

#define FONT_SIZE 75.0f
#define FONT_ASSET "Assets/Exo2-Regular.ttf"

#define BTN_WIDTH 325.0f
#define BTN_HEIGHT 175.0f
#define BTN_X (float)(CP_System_GetWindowWidth() / 2)
#define BTN_Y_RESUME (float)(CP_System_GetWindowHeight() / 4.25f)
#define BTN_Y_RESTART (float)(CP_System_GetWindowHeight() / 2.15f)
#define BTN_Y_EXIT (float)(CP_System_GetWindowHeight() / 1.45f)

static int is_paused = 0;
CP_Font myFont;


int Pause_IsPaused(void) {
	return is_paused;
}


void Pause_Init(void) {
	
	is_paused = 0;

	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);

}
void Pause_Update(void) {

	

	// If the player presses the 'Esc' key, toggle the pause state
	if (CP_Input_KeyTriggered(KEY_ESCAPE)) {
		is_paused = !is_paused;
	}

	if (!is_paused)
	{
		return;
	}

	

	if (IsAreaClicked(BTN_X, BTN_Y_RESUME, BTN_WIDTH, BTN_HEIGHT,CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		is_paused = 0;
		return;
	}

	if (IsAreaClicked(BTN_X, BTN_Y_RESTART, BTN_WIDTH, BTN_HEIGHT,CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		is_paused = 0;
		RopeGame_Init();
		return;
	}

	// Exit button
	if (IsAreaClicked(BTN_X, BTN_Y_EXIT, BTN_WIDTH, BTN_HEIGHT,CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(Level_Select_Init,Level_Select_Update,Level_Select_Exit);
		return;
	}

	

	// Draw buttons
	CP_Settings_RectMode(CP_POSITION_CENTER);
	CP_Settings_Fill(REDDISH_PINK_BG);

	CP_Graphics_DrawRect(BTN_X, BTN_Y_RESUME, BTN_WIDTH, BTN_HEIGHT);
	CP_Graphics_DrawRect(BTN_X, BTN_Y_RESTART, BTN_WIDTH, BTN_HEIGHT);
	CP_Graphics_DrawRect(BTN_X, BTN_Y_EXIT, BTN_WIDTH, BTN_HEIGHT);

	// Draw text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(FONT_SIZE);
	CP_Settings_Fill(BLACK_BG);
	CP_Font_DrawText("Resume", BTN_X, BTN_Y_RESUME);
	CP_Font_DrawText("Restart", BTN_X, BTN_Y_RESTART);
	CP_Font_DrawText("Exit", BTN_X, BTN_Y_EXIT);

}

void Pause_Exit(void) {
	CP_Font_Free(myFont);
}