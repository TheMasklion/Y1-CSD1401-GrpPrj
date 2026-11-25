//---------------------------------------------------------
// file:	splash.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Splash screen game state. Fades in the DigiPen logo
//			and the copyright text over a dark background with a
//			soft red glow, holds them, fades out, and then goes
//			to the main menu. The logo is fully visible for at
//			least two seconds. A key press or click skips it once
//			that minimum time has passed.
//			Ported from the LockedOut project's splash screen.
//
// Copyright © 2026 DigiPen, All rights reserved.
//---------------------------------------------------------

#include <stdbool.h>

#include "cprocessing.h"
#include "mainmenu.h"     // Main_Menu_* (the next state)
#include "splash.h"

#define LOGO_PATH            "Assets/DigiPen_BLACK.png"
#define FONT_ASSET           "Assets/Exo2-Regular.ttf"
// "\xC2\xA9" is the (c) in a circle written as UTF-8 bytes, so it shows correctly whatever encoding this file is saved in
#define COPYRIGHT_TEXT       "All content \xC2\xA9 2026 DigiPen Institute of Technology Singapore. All Rights Reserved."

#define SPLASH_FADE_IN       0.6f    // seconds to fade in
#define SPLASH_HOLD          2.0f    // seconds the logo stays fully visible (this is the "at least two seconds")
#define SPLASH_FADE_OUT      0.6f    // seconds to fade out
#define SPLASH_TOTAL         (SPLASH_FADE_IN + SPLASH_HOLD + SPLASH_FADE_OUT)
#define SPLASH_SKIP_AFTER    2.0f    // a key press or click can skip the splash, but only after this many seconds

#define LOGO_WIDTH           640.0f  // logo width on screen at 1600x900 (the height keeps the logo's shape)
#define GLOW_RINGS           14      // stacked circles that make the soft red glow behind the logo

static CP_Image s_logo = NULL;
static CP_Font  s_font = NULL;                 // the splash loads its own font; the main menu loads its own in Main_Menu_Init
static float    s_w = 1600.0f, s_h = 900.0f;   // window size
static float    s_k = 1.0f;                    // UI scale: 1.0 at 1600x900, smaller for smaller windows
static float    s_time;                        // seconds since the splash started
static float    s_logoW, s_logoH;              // logo size on screen

// ---- small drawing helpers (kept local so utils.c stays untouched) ----

static void DrawFilledCircle(float cx, float cy, float radius, CP_Color c)
{
	CP_Settings_EllipseMode(CP_POSITION_CENTER);
	CP_Settings_NoStroke();
	CP_Settings_Fill(c);
	CP_Graphics_DrawCircle(cx, cy, radius * 2.0f);   // CP_Graphics_DrawCircle takes a diameter
}

static void DrawRectCentered(float cx, float cy, float w, float h, CP_Color c)
{
	CP_Settings_RectMode(CP_POSITION_CENTER);   // the same rect mode the rest of the game draws in
	CP_Settings_NoStroke();
	CP_Settings_Fill(c);
	CP_Graphics_DrawRect(cx, cy, w, h);
}

static void DrawCenteredText(const char* s, float x, float y, float size, CP_Color c)
{
	CP_Font_Set(s_font);
	CP_Settings_TextSize(size);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
	CP_Settings_Fill(c);
	CP_Font_DrawText(s, x, y);
}

static void GoToMainMenu(void)
{
	CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
}

// 0..1: how visible everything is right now (fade in, hold, fade out).
static float GetVisibility(void)
{
	float v = 1.0f;
	if (s_time < SPLASH_FADE_IN)                          v = s_time / SPLASH_FADE_IN;
	else if (s_time > SPLASH_FADE_IN + SPLASH_HOLD)       v = (SPLASH_TOTAL - s_time) / SPLASH_FADE_OUT;
	return CP_Math_ClampFloat(v, 0.0f, 1.0f);
}

void Splash_Init(void)
{
	s_font = CP_Font_Load(FONT_ASSET);   // the copyright text needs the font

	s_w = (float)CP_System_GetWindowWidth();
	s_h = (float)CP_System_GetWindowHeight();
	s_k = (s_w / 1600.0f < s_h / 900.0f) ? s_w / 1600.0f : s_h / 900.0f;
	s_time = 0.0f;

	if (!s_logo) s_logo = CP_Image_Load(LOGO_PATH);
	if (s_logo)
	{
		s_logoW = LOGO_WIDTH * s_k;
		s_logoH = s_logoW * (float)CP_Image_GetHeight(s_logo) / (float)CP_Image_GetWidth(s_logo);
	}
	else
	{
		s_logoW = 0.0f;   // logo file not found: the splash still shows the text, then moves on
		s_logoH = 0.0f;
	}
}

void Splash_Update(void)
{
	float dt = CP_System_GetDt();
	if (dt > 0.05f) dt = 0.05f;
	s_time += dt;

	// ---- leave: when the time is up, or (after the minimum time) when the player skips ----
	bool skip = CP_Input_KeyTriggered(KEY_ESCAPE) || CP_Input_KeyTriggered(KEY_SPACE) ||
	            CP_Input_KeyTriggered(KEY_ENTER) || CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT);
	if (s_time >= SPLASH_TOTAL || (skip && s_time >= SPLASH_SKIP_AFTER))
	{
		GoToMainMenu();
		return;
	}

	// ---- draw ----
	float visible = GetVisibility();
	int   alpha = (int)(255.0f * visible);
	float cx = s_w * 0.5f;
	float cy = s_h * 0.44f;   // logo centre: a little above the middle of the screen

	// background: near-black with a soft red glow behind the logo (big faint circles stacked up)
	CP_Graphics_ClearBackground(CP_Color_Create(10, 12, 10, 255));
	for (int i = 0; i < GLOW_RINGS; ++i)
	{
		float radius = (600.0f - 40.0f * (float)i) * s_k;
		DrawFilledCircle(cx, cy, radius, CP_Color_Create(150, 20, 25, (int)(7.0f * visible)));
	}

	// logo
	if (s_logo)
	{
		CP_Settings_ImageMode(CP_POSITION_CENTER);
		CP_Image_Draw(s_logo, cx, cy, s_logoW, s_logoH, alpha);
	}
	else
	{
		DrawCenteredText("DigiPen Institute of Technology Singapore", cx, cy, 48.0f * s_k, CP_Color_Create(235, 235, 235, alpha));
	}

	// thin red line and the copyright text at the bottom
	DrawRectCentered(cx, s_h - 95.0f * s_k, 900.0f * s_k, 2.0f * s_k, CP_Color_Create(140, 30, 30, alpha));
	DrawCenteredText(COPYRIGHT_TEXT, cx, s_h - 62.0f * s_k, 22.0f * s_k, CP_Color_Create(210, 210, 210, alpha));

	// once skipping is allowed, tell the player they can press SPACE
	if (s_time >= SPLASH_SKIP_AFTER)
	{
		DrawCenteredText("Press SPACE to skip", cx, s_h - 26.0f * s_k, 18.0f * s_k, CP_Color_Create(150, 150, 150, (int)(alpha * 0.8f)));
	}
}

void Splash_Exit(void)
{
	if (s_logo)
	{
		CP_Image_Free(s_logo);
		s_logo = NULL;
	}
	if (s_font)
	{
		CP_Font_Free(s_font);
		s_font = NULL;
	}

	// Put the drawing settings back to what the game expects when it starts. The splash turns the
	// stroke off while it draws, and the menu / level select rely on the start-up state: a 3 px black
	// outline, rects drawn from their centre, images drawn from their centre. (Checked against the
	// untouched build: with any other rect mode the menu and level select buttons move.)
	CP_Settings_Stroke(CP_Color_Create(0, 0, 0, 255));
	CP_Settings_StrokeWeight(3.0f);
	CP_Settings_RectMode(CP_POSITION_CENTER);
	CP_Settings_EllipseMode(CP_POSITION_CENTER);
	CP_Settings_ImageMode(CP_POSITION_CENTER);
}
