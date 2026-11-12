#include <cprocessing.h>
#include "utils.h"
#include "ropegame.h"
#include "LevelSelect.h"
#include "gamestate.h"

CP_Font myFont;
#define FONT_ASSET "Assets/Exo2-Regular.ttf"

//I keep almost the same styling for consistency
#define GREY_BG CP_Color_Create(100, 100, 100, 255)
#define BLACK_BG CP_Color_Create(0, 0, 0, 255)
#define REDDISH_PINK_BG CP_Color_Create(255, 125, 125, 255)
#define WHITE_BG CP_Color_Create(255, 255, 255, 255)
#define RED_BG CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG CP_Color_Create(0, 255, 0, 255)

//Screen coordinates
#define center_screen_y CP_System_GetWindowHeight() / 2.0f
#define retry_y center_screen_y - 50.0f
#define levelselect_y center_screen_y + 150.0f


void GameOver_Screen(void)
{
	//Load font, align text to center, so the "Game Over" text is centered on the screen
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_RectMode(CP_POSITION_CENTER);

	//gray area screen
	CP_Settings_Fill(GREY_BG);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, center_screen_y, CP_System_GetWindowWidth() * (2.0f / 3.0f), CP_System_GetWindowHeight() * (2.0f / 3.0f));

	//Game Over text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(125.0f);
	CP_Settings_Fill(RED_BG); //text colour
	CP_Font_DrawText("Game Over", CP_System_GetWindowWidth() / 2.0f, center_screen_y - 200.0f);

	//Retry and Level Select Buttons
	CP_Settings_Fill(REDDISH_PINK_BG);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, retry_y, 300.0f, 100.0f);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, levelselect_y, 300.0f, 100.0f);

	//Retry and Level Select Text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(60.0f);
	CP_Settings_Fill(BLACK_BG); //text colour
	CP_Font_DrawText("Retry", CP_System_GetWindowWidth() / 2.0f, retry_y);
	CP_Font_DrawText("Level Select", CP_System_GetWindowWidth() / 2.0f, levelselect_y);

	//Checking if the buttons are clicked
	//float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y
	if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, retry_y, 300.0f, 100.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		RopeGame_Init();// Retry
	}

	if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, levelselect_y, 300.0f, 100.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(Level_Select_Init, Level_Select_Update, Level_Select_Exit);	// Level Select
	}
}

void GameWin_Screen(void)
{
	//Load font, align text to center, so the "You Win!" text is centered on the screen
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_RectMode(CP_POSITION_CENTER);
	//gray area screen
	CP_Settings_Fill(GREY_BG);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, center_screen_y, CP_System_GetWindowWidth() * (2.0f / 3.0f), CP_System_GetWindowHeight() * (2.0f / 3.0f));
	//You Win! text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(125.0f);
	CP_Settings_Fill(GREEN_BG); //text colour
	CP_Font_DrawText("You Win!", CP_System_GetWindowWidth() / 2.0f, center_screen_y - 200.0f);
	//Retry and Level Select Buttons
	CP_Settings_Fill(REDDISH_PINK_BG);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, retry_y, 300.0f, 100.0f);
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, levelselect_y, 300.0f, 100.0f);
	//Retry and Level Select Text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(60.0f);
	CP_Settings_Fill(BLACK_BG); //text colour
	CP_Font_DrawText("Retry", CP_System_GetWindowWidth() / 2.0f, retry_y);
	CP_Font_DrawText("Level Select", CP_System_GetWindowWidth() / 2.0f, levelselect_y);
	//Checking if the buttons are clicked
	if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, retry_y, 300.0f, 100.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		RopeGame_Init();// Retry
	}
	if (CP_Input_MouseClicked() && IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, levelselect_y, 300.0f, 100.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(Level_Select_Init, Level_Select_Update, Level_Select_Exit);	// Level Select
	}
}