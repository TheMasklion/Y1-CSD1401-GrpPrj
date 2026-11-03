//---------------------------------------------------------
// file:	LevelSelect.c
// author:	Jayden Tan
// email:	jayden.tan@digipen.edu
//
// brief:	Level Select creates a level selector using rectangles and texts, to form buttons.
//			Utilizing utils.c functions onclick to interact with the shapes.
//
//---------------------------------------------------------

#include "utils.h"
#include "ropegame.h"	// ROPE: the rope game state (rope on the candy, fixed length)
#include "LevelSelect.h"//Level Select > goes to this after clicking Play
#include "mainmenu.h"//Go back to main menu from backarrow top left
#include "cprocessing.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//I keep almost the same styling for consistency
#define GREY_BG CP_Color_Create(100, 100, 100, 255)
#define BLACK_BG CP_Color_Create(0, 0, 0, 255)
#define REDDISH_PINK_BG CP_Color_Create(255, 125, 125, 255)
#define WHITE_BG CP_Color_Create(255, 255, 255, 255)

#define FONT_SIZE 75.0f
#define FONT_ASSET "Assets/Exo2-Regular.ttf"

//Limit of how many buttons we can create on screen : 16 per page
#define MAX_COLUMNS 4
#define MAX_ROWS 4
//columns and rows we need for calculation
int columns, rows;
float BTN_X, BTN_Y;
int col, row;
/* columns/rows vs col/row: former is the 'ceiling' of the grid based on the number of levels we have. Latter calculates how many rows there are as we are
in a specific column through the loop 
e.g 15:
- minimum grid we need to use is 4x4, columns and rows specify this boundary calculated beforehand
- col 1-3 are filled with 4 row, col 4 is filled only with 3 row, col and row will calculate along the way
*/

#define BTN_WIDTH 300.0f
#define BTN_HEIGHT 100.0f
#define BTN_Y_MARGIN 300.0f
#define BTN_X_SPACE (float)CP_System_GetWindowWidth() //Given width space to fill the buttons
#define BTN_Y_SPACE (float)CP_System_GetWindowHeight() - BTN_Y_MARGIN //Given height space to fill the buttons
//we +1 because both ends of a button have a space <space><button><space> THIS CALCULATES <space> ONLY
#define X_GAP (float)(BTN_X_SPACE - (BTN_WIDTH * MAX_COLUMNS)) / (MAX_COLUMNS + 1)
#define Y_GAP (float)(BTN_Y_SPACE - (BTN_HEIGHT * MAX_ROWS)) / (MAX_ROWS + 1)

#define levels 1 //levels to be created, this value can change dynamically when we have level creator

CP_Font myFont;

void Level_Select_Init(void)
{
	myFont = CP_Font_Load(FONT_ASSET);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);

	//First we map a grid; e.g 3 buttons use a 1x4 grid, 19 buttons use a 5x4 grid etc.
	columns = levels; //calculate columns needed 
	if (columns > MAX_COLUMNS)
		columns = MAX_COLUMNS; //limit columns if exceed, we will then calculate the rows to fit 

	rows = (int)ceil((float)levels / columns); //calculate rows needed by dividing the levels by columns and rounding up (to fill all numbers) so this time we get 5
	if (rows > MAX_ROWS)
		rows = MAX_ROWS; //limit rows
	
}


void Level_Select_Update(void) 
{
	CP_Graphics_ClearBackground(GREY_BG);

	//Header text
	CP_Settings_TextSize(125.0f);
	CP_Settings_Fill(WHITE_BG); //text colour
	CP_Font_DrawText("Select level", CP_System_GetWindowWidth() / 2.0f, BTN_Y_MARGIN / 2.0f);

	CP_Font_Set(myFont);
	CP_Settings_TextSize(FONT_SIZE);
	
	
	//Level Button
	for (int i = 0; i < levels; i++)
	{
		/*Level buttons----------------------------------------------------------------------------------------------------------------------*/
		CP_Settings_Fill(REDDISH_PINK_BG); //Button colour

		//Determine which row and column we are currently in for the button; e.g button 1 row 0 column 0, button 2 row 1 column 2...
		col = i % columns; //gives remainder of i or 0 when divided by columns(4)
		row = i / columns; //gives quotient if divisible by columns(4), means we are on to Wanother row if so

		//we add BTN/2.0f because the rectangles draws from the center and we must account for half its dimensions to find its edge 
		BTN_X = X_GAP + (BTN_WIDTH / 2.0f) + ((BTN_WIDTH + X_GAP) * col);//x-coordinate is the <space>x<button><space>x2<button><space>
		BTN_Y = BTN_Y_MARGIN + (BTN_HEIGHT / 2.0f) + ((BTN_HEIGHT + Y_GAP) * row) ;//same for y but add in the margin
		CP_Graphics_DrawRect(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT);

		/*Level text----------------------------------------------------------------------------------------------------------------------*/
		CP_Settings_Fill(BLACK_BG); //text colour
		char levelText[20];
		sprintf_s(levelText, sizeof(levelText), "Level %d", i + 1);
		CP_Font_DrawText(levelText, BTN_X, BTN_Y); //Level Button Text
	}

	//Checking which level is selected, then load according to the i value returned
	//float area_center_x, float area_center_y, float area_width, float area_height, float click_x, float click_y, int i
	if (CP_Input_MouseClicked()) 
	{
		//run the test for each button
		for (int i = 0; i < levels; i++) 
		{
			//we need to get the x and y coordinates of each button
			//Determine which row and column we are currently in for the button; e.g button 1 row 0 column 0, button 2 row 1 column 2...
			col = i % columns; //gives remainder of i or 0 when divided by columns(4)
			row = i / columns; //gives quotient if divisible by columns(4), means we are on to Wanother row if so
			BTN_X = X_GAP + (BTN_WIDTH / 2.0f) + ((BTN_WIDTH + X_GAP) * col);//x-coordinate is the <space><x><button><space><x2><button><space>
			BTN_Y = BTN_Y_MARGIN + (BTN_HEIGHT / 2.0f) + ((BTN_HEIGHT + Y_GAP) * row);//same for y but add in the margin

			//then run the test, additionally parse i to compare each button number
			switch (LevelClicked(BTN_X, BTN_Y, BTN_WIDTH, BTN_HEIGHT, CP_Input_GetMouseX(), CP_Input_GetMouseY(),i)) 
			{
				//IMPORTANT WE START FROM CASE 1 SO ITS i+1, 0 is reserved for false meaning not clicked
				default:
				case 0:
				break;
				case 1:
					CP_Engine_SetNextGameState(RopeGame_Init, RopeGame_Update, RopeGame_Exit);	// ROPE: Play opens the rope state (was Game_Init, Game_Update, Game_Exit)
					break;
	
			}
		}
	}

	if (CP_Input_KeyTriggered(KEY_Q))
		CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);

	

	
}

void Level_Select_Exit(void)
{
	CP_Font_Free(myFont);
}