//---------------------------------------------------------
// file:	LevelData.h
// author:	Everyone
//
// brief:	One table entry per level: where the anchors, the pig, the power-ups and the stars sit.
//			ropegame.c reads the entry for current_level in RopeGame_Init, so adding a level is one more
//			entry in LevelData.c (and LEVEL_COUNT + 1), nothing else.
//
//			All positions are pixels in the 1600 x 900 window.
//---------------------------------------------------------

#pragma once

#define LEVEL_COUNT			4		// levels in the table (LevelSelect.c reads this for its buttons)
#define LEVEL_MAX_ANCHORS	4		// same as ROPERIG_MAX_ROPES: the game's own rope plus up to three added by the rig
#define LEVEL_STARS			3		// stars per level (Stars.c draws and counts exactly three)

typedef struct
{
	float x;
	float y;
} LevelPoint;

typedef struct
{
	int anchor_count;						// 1..LEVEL_MAX_ANCHORS ropes, every one holding the candy
	LevelPoint anchor[LEVEL_MAX_ANCHORS];	// anchor[0] is the main rope: the candy starts one rope length from it at start_angle
	float rope_length;						// rest length of every rope, pixels
	float start_angle;						// degrees away from hanging straight down that the candy starts at (+ = left of the anchor, - = right)

	LevelPoint pig;							// centre of the pig
	float pig_radius;						// smaller pig = smaller landing zone

	LevelPoint bubble;						// every level has one bubble and one magnet: park one off screen to leave it out
	LevelPoint magnet;

	LevelPoint star[LEVEL_STARS];			// where the three stars are drawn (collected when the candy touches them)

	const char* hint;						// one line shown at the top while playing (the tutorial), NULL for none
} LevelData;

// The table entry for a level number from the level select (1-based). Out-of-range numbers clamp,
// so current_level == 0 (the game state entered without a level select) plays level 1.
const LevelData* Level_Get(int level);
