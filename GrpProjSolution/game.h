#pragma once

void Game_Init(void);

void Game_Update(void);

<<<<<<< Updated upstream
void Game_Exit(void);

void DrawPlayTriangle(float cx, float cy, float degrees);
=======
void Game_Init(void);		// builds anchor, rope, candy and pig for the current window size
void Game_Update(void);		// one frame: physics, then drawing
void Game_Exit(void);		// nothing to free h
>>>>>>> Stashed changes
