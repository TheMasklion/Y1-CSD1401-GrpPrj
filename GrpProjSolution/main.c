//---------------------------------------------------------
// file:	main.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Main entry point for the sample project
//			of the CProcessing library
//
// Copyright © 2026 DigiPen, All rights reserved.
//---------------------------------------------------------

#include "cprocessing.h"
#include "mainmenu.h"
#include "splash.h"


// main() the starting point for the program
// CP_Engine_SetNextGameState() tells CProcessing which functions to use for init, update and exit
// CP_Engine_Run() is the core function that starts the simulation
int main(void)
{
	CP_Engine_SetNextGameState(Splash_Init, Splash_Update, Splash_Exit);	// DigiPen splash screen first; it moves on to the main menu by itself
	CP_System_SetWindowSize(1600, 900);
	//CP_System_Fullscreen(); //fullscreen but we need to fix the stars, to use screen positioning instead of +- pixels Jayden
	CP_Engine_Run(0);
	return 0;
}
