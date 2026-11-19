//---------------------------------------------------------
// file:	LevelSelect.c
// author:	Jayden Tan
// email:	jayden.tan@digipen.edu
//
// brief:	GameOver and GameWin screen code. The game state is set to GameOver or GameWin in ropegame.c when the candy is lost or reaches the pig.
//			Utilizing utils.c functions onclick to interact with the shapes, Using Stars.c to display the stars collected on the GameOver screen. 
//			Extern variables are star_img and star_count from Stars.c, to access the star image and the number of stars collected in the level.
//---------------------------------------------------------

#include <cprocessing.h>
#include "utils.h"
#include "ropegame.h"
#include "LevelSelect.h"
#include "gamestate.h"
#include "Stars.h"

extern CP_Image star_img; //Access the star_img variable from Stars.c
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
#define retry_y center_screen_y + 100.0f
#define levelselect_y center_screen_y + 225.0f
#define star_y center_screen_y - 100.0f

#define star_offset_x 50.0f // Offset for the stars on the screen
#define star_starting_x CP_System_GetWindowWidth() / 2.0f - star_offset_x // Starting x position for the first star

extern int star_count; // Access the star_count variable from Stars.c
static CP_Image end_bg;

static void EndScreen_DrawPanel(void)
{
	float panel_w = CP_System_GetWindowWidth() * (2.0f / 3.0f);
	float panel_h = CP_System_GetWindowHeight() * (2.0f / 3.0f);

	if (end_bg == NULL)
		end_bg = CP_Image_Load("Assets/background.png");

	CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_LINEAR);	// smooth stripes
	DrawImageFill(end_bg, CP_System_GetWindowWidth() / 2.0f, center_screen_y, panel_w, panel_h);
	CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_NEAREST);	// back to what the game and the star sprites use
}

void GameOver_Screen(void)
{
	//Load font, align text to center, so the "Game Over" text is centered on the screen
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_RectMode(CP_POSITION_CENTER);

	//screen background panel
	EndScreen_DrawPanel();

	//Game Over text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(125.0f);
	CP_Settings_Fill(RED_BG); //text colour
	CP_Font_DrawText("Game Over", CP_System_GetWindowWidth() / 2.0f, center_screen_y - 200.0f);

	//stars collected text
	if (star_img == NULL)
	{
		CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_NEAREST);
		star_img = CP_Image_Load("./Assets/Stars.png");
	}
	int i; // Loop through the stars and draw the collected ones
	//draw yellow star for each star collected, up to 3 stars
	for (i = 1; i <= star_count; i++)
	{
		//draw yellow star
		CP_Image_DrawSubImage(star_img, star_starting_x + (i - 1) * star_offset_x, center_screen_y, 100.0f, 100.0f,
			YellowStar.sprite_left, YellowStar.sprite_top, YellowStar.sprite_right, YellowStar.sprite_bottom, 255);
	}

	for (; i <= 3; i++)
	{
		//draw empty star
		CP_Image_DrawSubImage(star_img, star_starting_x + (i - 1) * star_offset_x, center_screen_y, 100.0f, 100.0f,
			EmptyStar.sprite_left, EmptyStar.sprite_top, EmptyStar.sprite_right, EmptyStar.sprite_bottom, 255);
	}
		

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

	//screen background panel
	EndScreen_DrawPanel();

	//You Win! text
	CP_Font_Set(myFont);
	CP_Settings_TextSize(125.0f);
	CP_Settings_Fill(GREEN_BG); //text colour
	CP_Font_DrawText("You Win!", CP_System_GetWindowWidth() / 2.0f, center_screen_y - 200.0f);

	//stars collected text
	if (star_img == NULL)
	{
		CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_NEAREST);
		star_img = CP_Image_Load("./Assets/Stars.png");
	}
	int i; // Loop through the stars and draw the collected ones
	//draw yellow star for each star collected, up to 3 stars
	for (i = 1; i <= star_count; i++)
	{
		//draw yellow star
		CP_Image_DrawSubImage(star_img, star_starting_x + (i - 1) * star_offset_x, center_screen_y, 100.0f, 100.0f,
			YellowStar.sprite_left, YellowStar.sprite_top, YellowStar.sprite_right, YellowStar.sprite_bottom, 255);
	}

	for (; i <= 3; i++)
	{
		//draw empty star
		CP_Image_DrawSubImage(star_img, star_starting_x + (i - 1) * star_offset_x, center_screen_y, 100.0f, 100.0f,
			EmptyStar.sprite_left, EmptyStar.sprite_top, EmptyStar.sprite_right, EmptyStar.sprite_bottom, 255);
	}

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