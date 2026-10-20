//
// This file showcases how to render dynamic text in CProcessing
// which is useful for displaying variables on screen.
// 
// This file assumes that:
// - You have Visual Studio 2022 set up correctly for
//   CProcessing as per the "Visual Studios Setup Guide" 
// - You have the file "Exo2-Regular.ttf" in your "Assets" folder, 
//   which should be a file in the TTF font format.
//

#include <stdio.h> // required for sprintf_s
#include "cprocessing.h"

CP_Font my_awesome_font;

void game_init(void) {
	// Loads "Assets/Exo2-Regular.ttf" into memory and gives back
	// a representation of the font to us (aka a 'handle' to the font).
	// 
	// We assign the 'handle' to my_awesome_font. 
	// You can then think of my_awesome_font as holding information
	// information and data about Exo2-Regular.ttf.
	//
	my_awesome_font = CP_Font_Load("Assets/Exo2-Regular.ttf");

	// Tells CProcessing to use my_awesome_font for it's future
	// text drawing operations.
	CP_Font_Set(my_awesome_font);

	// Tells CProcessing that when drawing text in the future (via CP_Font_DrawText),
	// the position given to it is the center of the text horizontally and vertically.
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);

	// Tells CProcessing that when drawing text in the future (via CP_Font_DrawText),
	// We use size 72 font height.
	CP_Settings_TextSize(72.f);
}

void game_update(void) {
	// Retrieve the X and Y position of the mouse
	float mouse_x = CP_Input_GetMouseX();
	float mouse_y = CP_Input_GetMouseY();

	// Create an array of characters (aka a string) that can store up to 256 characters.
	char buffer[256];

	// Fill the buffer with the text we want.
	// Notice that it uses a similar syntax as printf()!
	sprintf_s(buffer, sizeof(buffer), "mouse X: %.2f, mouse_y: %.2f", mouse_x, mouse_y);

	// Here on, we start rendering
	CP_Graphics_ClearBackground(CP_Color_Create(0, 0, 0, 255));

	// Tells CProcessing to use the white color for anything we are drawing on the screen
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));

	// Draw the text using the string stored in the buffer in the center of the screen.
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Font_DrawText(buffer, (float)CP_System_GetWindowWidth() / 2, (float)CP_System_GetWindowHeight() / 2);

}

void game_exit(void) {
	CP_Font_Free(my_awesome_font);
}

int main(void)
{
	CP_System_SetWindowSize(1600, 900);
	CP_Engine_SetNextGameState(game_init, game_update, game_exit);
	CP_Engine_Run(0);
	return 0;
}