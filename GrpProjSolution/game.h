#pragma once

#pragma once

#include "cprocessing.h"

void Game_Init(void);		// builds anchor, rope, candy and pig for the current window size
void Game_Update(void);		// one frame: physics, then drawing
void Game_Exit(void);		// nothing to free