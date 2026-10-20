//---------------------------------------------------------
// file:	game.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Game for solo proj 2, creates the game after the mainmenu. Uses utils.c for onclick function, to switch between circles.
//          math.h was included for sqrt to calculate distance. Used pythagoras & angles to calculate where the triangles are supposed to
//          be on the circles. Collision check for the boundary for AI and avoidance range. Q to exit back to mainmenu 
//
//---------------------------------------------------------


#include "utils.h" 
#include "game.h"
#include "mainmenu.h"
#include "cprocessing.h"
#include <math.h>
#include <stdio.h>

#define LIGHT_GREY_BG CP_Color_Create(200, 200, 200, 255)
#define WHITE_BG CP_Color_Create(255, 255, 255, 255)
#define BROWN_BG CP_Color_Create(120, 80, 40, 255)
#define RED_BG CP_Color_Create(255, 0, 0, 255)
#define GREEN_BG CP_Color_Create(0, 255, 0, 255)
#define DARK_GREY_BG CP_Color_Create(60, 60, 60, 255)

#define ANCHOR_RADIUS 12.0f
#define ANCHOR_Y 60.0f
#define ROPE_LENGTH 300.0f
#define CANDY_RADIUS 25.0f
#define PIG_RADIUS 50.0f


typedef struct
{
    float x;
    float y;
    float r;
} Candy;

typedef struct
{
    float x;
    float y;
    float r;
} Anchor;

typedef struct
{
    float length; // distance from the anchor to the candy
} Rope;

typedef struct
{
    float x;
    float y;
    float r;
} Pig;

Candy candy;
Anchor anchor;
Rope rope;
Pig pig;

void Game_Init(void)
{
    float screen_w = (float)CP_System_GetWindowWidth();
    float screen_h = (float)CP_System_GetWindowHeight();

    // anchor: top middle of the screen
    anchor.x = screen_w / 2.0f;
    anchor.y = ANCHOR_Y;
    anchor.r = ANCHOR_RADIUS;

    // rope: fixed length for now
    rope.length = ROPE_LENGTH;

    // candy: hangs straight below the anchor at the end of the rope
    candy.x = anchor.x;
    candy.y = anchor.y + rope.length;
    candy.r = CANDY_RADIUS;

    // pig: at the very bottom of the screen, directly below the candy
    pig.r = PIG_RADIUS;
    pig.x = candy.x;
    pig.y = screen_h - pig.r;
}

void Game_Update(void)
{	//background
    CP_Graphics_ClearBackground(LIGHT_GREY_BG);

    //rope (drawn first so the anchor and candy sit on top of it)
    CP_Settings_Stroke(BROWN_BG);
    CP_Settings_StrokeWeight(4.0f);
    CP_Graphics_DrawLine(anchor.x, anchor.y, candy.x, candy.y);

    //shapes get a thin outline
    CP_Settings_Stroke(DARK_GREY_BG);
    CP_Settings_StrokeWeight(2.0f);

    //anchor (DrawCircle takes the diameter, so radius * 2)
    CP_Settings_Fill(DARK_GREY_BG);
    CP_Graphics_DrawCircle(anchor.x, anchor.y, anchor.r * 2.0f);

    //candy
    CP_Settings_Fill(RED_BG);
    CP_Graphics_DrawCircle(candy.x, candy.y+=1, candy.r * 2.0f);

    //pig
    CP_Settings_Fill(GREEN_BG);
    CP_Graphics_DrawCircle(pig.x, pig.y, pig.r * 2.0f);

    //exit back to main menu
    if (CP_Input_KeyDown(KEY_Q)) //if the Q key is pressed, back to main menu
    {
        CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
    }
}

void Game_Exit(void)
{
    ExitProcess(0);
}
