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
//#include "ropegame.h"	// ROPE: the rope game state (rope on the candy, fixed length)
#include "LevelSelect.h"//Level Select > goes to this after clicking Play
#include "mainmenu.h"
#include "cprocessing.h"
#include <stdio.h>
#include "SaveData.h" // Load/Create save data



#define GREY_BG CP_Color_Create(100, 100, 100, 255)
#define BLACK_BG CP_Color_Create(0, 0, 0, 255)
#define REDDISH_PINK_BG CP_Color_Create(255, 125, 125, 255)

#define FONT_SIZE 75.0f
#define FONT_ASSET "Assets/Exo2-Regular.ttf"

#define BTN_WIDTH 325.0f
#define BTN_HEIGHT 175.0f
#define BTN_X (float)(CP_System_GetWindowWidth() / 2)
#define BTN_Y_PLAY (float)(CP_System_GetWindowHeight() / 3.0f)	// moved down from / 4.25f to make room for the title
#define BTN_Y_EXIT (float)(CP_System_GetWindowHeight() / 1.45f)

// Title: "Lolli" in pink and "Pig" in green, like the logo
#define TITLE_FONT_SIZE 150.0f
#define TITLE_Y (float)(CP_System_GetWindowHeight() / 8.5f)
#define TITLE_SPLIT_OFFSET 30.0f	// "Lolli" is wider than "Pig", so the seam sits a little right of centre to keep the whole title centred
#define TITLE_SHADOW_OFFSET 5.0f
#define LOLLI_PINK CP_Color_Create(244, 162, 192, 255)
#define PIG_GREEN CP_Color_Create(106, 191, 64, 255)
#define TITLE_SHADOW CP_Color_Create(42, 27, 55, 255)

CP_Font myFont;
static CP_Image menu_bg;

// Draws the "LolliPig" title in two colours. CProcessing cannot measure text width, so "Lolli" is
// right-aligned and "Pig" left-aligned at the same seam x, which makes the two words sit flush.
static void DrawTitle(void)
{
	float seam_x = BTN_X + TITLE_SPLIT_OFFSET;

	CP_Font_Set(myFont);
	CP_Settings_TextSize(TITLE_FONT_SIZE);

	// "Lolli" ends at the seam
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_RIGHT, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(TITLE_SHADOW);
	CP_Font_DrawText("Lolli", seam_x + TITLE_SHADOW_OFFSET, TITLE_Y + TITLE_SHADOW_OFFSET);
	CP_Settings_Fill(LOLLI_PINK);
	CP_Font_DrawText("Lolli", seam_x, TITLE_Y);

	// "Pig" starts at the seam
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(TITLE_SHADOW);
	CP_Font_DrawText("Pig", seam_x + TITLE_SHADOW_OFFSET, TITLE_Y + TITLE_SHADOW_OFFSET);
	CP_Settings_Fill(PIG_GREEN);
	CP_Font_DrawText("Pig", seam_x, TITLE_Y);

	// Back to centred text for the buttons
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
}

void Main_Menu_Init(void)
{
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	menu_bg = CP_Image_Load("Assets/background.png");
	CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_LINEAR);	// smooth, because the game sets NEAREST for the pixel art

	InitialiseSaveData();
}

void Main_Menu_Update(void)
{
	//Drawing the main menu
	DrawBackground(menu_bg);
	DrawTitle();
	CP_Settings_Fill(REDDISH_PINK_BG); //Play Button colour
	CP_Graphics_DrawRect(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT); //Play Button
	CP_Graphics_DrawRect(BTN_X, BTN_Y_EXIT, BTN_WIDTH, BTN_HEIGHT); //Exit Button


	//Drawing the text for the buttons
	CP_Font_Set(myFont);
	CP_Settings_TextSize(FONT_SIZE);
	CP_Settings_Fill(BLACK_BG); //Play text colour
	CP_Font_DrawText("Play", BTN_X, BTN_Y_PLAY); //Play Button Text
	CP_Font_DrawText("Exit", BTN_X, BTN_Y_EXIT); //Exit Button Text)


	//Checking if the buttons are clicked
	//float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y
	//printf("%d",IsAreaClicked(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()));
	if (CP_Input_MouseClicked() && IsAreaClicked(BTN_X, BTN_Y_PLAY, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY()))
	{
		CP_Engine_SetNextGameState(Level_Select_Init, Level_Select_Update, Level_Select_Exit);	// ROPE: Play opens the level select state (was Game_Init, Game_Update, Game_Exit)
	}


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
	CP_Image_Free(menu_bg);
	menu_bg = NULL;
}