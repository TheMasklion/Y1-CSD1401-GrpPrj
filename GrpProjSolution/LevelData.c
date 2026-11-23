//---------------------------------------------------------
// file:	LevelData.c
// author:	Everyone
//
// brief:	The level table. See LevelData.h for what every field means.
//
//			How the pieces interact (so the routes below make sense):
//			- The candy hangs where every rope can reach it. Cut a rope and it swings on the others; cut the
//			  last one and it falls STRAIGHT DOWN from where it was (x stays), so the cut has to be timed.
//			- A star is collected when the candy comes within 50 px + candy radius (~69 px) of its centre.
//			- The bubble catches the free candy that touches it (bubble radius 35 + 19) and floats it up at
//			  150 px/s until it is clicked or reaches the top, then the candy falls again from there.
//			- The magnet is pressed and held. While held, the first time the FREE candy falls level with it
//			  (within 30 px in y) it slides the candy sideways at 300 px/s toward the magnet, at that height,
//			  and lets go when the candy touches it (or the mouse is released early): the candy then drops.
//
//			Difficulty curve:
//			Level 1 (tutorial) - one rope straight above the pig, no power-ups, the three stars in the drop path.
//			                   One cut collects everything; a hint on screen says what to do.
//			Level 2 (medium) - two ropes, NO bubble, smaller pig far off to the LEFT while the candy hangs on
//			                   the right. A straight drop misses: the magnet has to carry the candy over the
//			                   pig. Three stars need the swing: cut the right rope, let the candy swing out to
//			                   the star on the left, cut the main rope as it swings back to the right so it
//			                   falls through the star under the V, then hold the magnet to sweep it through
//			                   the last star and drop it on the pig.
//			Level 3 (hard)   - three ropes, smallest pig in the far RIGHT corner, bubble and magnet both
//			                   needed in order. Cut both left ropes so the candy swings right on the last one,
//			                   and cut that one while the candy is over the bubble (a ~100 px window): the
//			                   bubble lifts it through the high star, pops at the top, and the held magnet
//                   (placed higher than the candy ever swings, so it cannot shortcut the bubble)
//			                   catches the falling candy, sweeps it through the third star and drops it on
//			                   the pig. Miss the bubble and the candy is lost.
//			Level 4          - the original level 1 layout: three ropes, bubble and magnet, pig under the main
//			                   anchor. Needs a timed cut, the magnet, and letting go of it at the right moment.
//---------------------------------------------------------

#include "LevelData.h"

static const LevelData levels[LEVEL_COUNT] =
{
	// ---------------------------------------------------------------- Level 1: tutorial
	// One rope, no power-ups (both parked off screen). The candy hangs straight above the pig with the three
	// stars in a line between them: cut the rope and the candy falls through all three onto the pig.
	{
		.anchor_count = 1,
		.anchor = { { 800.0f, 60.0f } },
		.rope_length = 300.0f,
		.start_angle = 0.0f,			// hangs straight down at (800, 360), no swing
		.pig = { 800.0f, 850.0f },
		.pig_radius = 50.0f,
		.bubble = { -200.0f, -200.0f },
		.magnet = { -200.0f, -200.0f },
		.star = { { 800.0f, 470.0f }, { 800.0f, 580.0f }, { 800.0f, 690.0f } },
		.hint = "Tutorial: swipe across the rope to cut it",
	},

	// ---------------------------------------------------------------- Level 2: medium
	// Candy settles at the bottom of a V about (905, 282). Pig and magnet are on the left. Cut the right
	// rope and it swings on the main rope out to x ~ 580 (star 3), back to x ~ 770 (cut here for star 1),
	// then settles under the anchor at x ~ 700.
	{
		.anchor_count = 2,
		.anchor = { { 700.0f, 60.0f }, { 1100.0f, 60.0f } },
		.rope_length = 300.0f,
		.start_angle = -45.0f,		// negative: the candy starts on the right of the main anchor, inside the V
		.pig = { 450.0f, 850.0f },
		.pig_radius = 45.0f,
		.bubble = { -200.0f, -200.0f },	// no bubble in this level: parked off screen where the candy can never touch it
		.magnet = { 450.0f, 600.0f },
		.star =
		{
			{ 790.0f, 470.0f },		// under the first swing back to the right (x ~ 770): cut there and the drop passes it
			{ 620.0f, 600.0f },		// on the magnet's pull path, at the magnet's height
			{ 560.0f, 330.0f },		// far left of the first swing on the main rope once the right rope is cut (x ~ 580)
		},
		.hint = "Tutorial: hold the magnet to pull the lollipop towards it",
	},

	// ---------------------------------------------------------------- Level 3: hard
	// Candy settles about (590, 295) on the two upper ropes; the low left rope starts slack. With both left
	// ropes cut it swings right on the (800, 80) rope out to x ~ 930 once, then never past ~850 again, so
	// the bubble at x = 910 (catch window +-54 px) has to be hit on that first swing.
	{
		.anchor_count = 3,
		.anchor = { { 400.0f, 60.0f }, { 800.0f, 80.0f }, { 330.0f, 300.0f } },
		.rope_length = 300.0f,
		.start_angle = -45.0f,
		.pig = { 1300.0f, 850.0f },
		.pig_radius = 40.0f,
		.bubble = { 910.0f, 600.0f },
		.magnet = { 1300.0f, 250.0f },	// above every height the swinging candy can be cut from: only the
														// fall out of the popped bubble can be caught, so the bubble is a must
		.star =
		{
			{ 910.0f, 450.0f },		// above the bubble: collected on the drop into it
			{ 910.0f, 150.0f },		// high up: collected on the bubble ride
			{ 1100.0f, 250.0f },	// at the magnet's height, between the bubble column and the magnet
		},
		.hint = "Tutorial: Bubble will cause the lollipop to float",
	},

	// ---------------------------------------------------------------- Level 4: the original level 1 layout
	// Three ropes, bubble and magnet. The candy rests about (725, 270). Cut the left rope and it swings right to
	// x ~ 900 on the other two; cut both there and it drops through the star at (900, 550); the held magnet
	// pulls it left into the bubble, which lifts it through the top star and pops; let go of the magnet by
	// then or it grabs the candy again on the way down, which falls through the last star onto the pig.
	{
		.anchor_count = 3,
		.anchor = { { 800.0f, 60.0f }, { 438.0f, 172.0f }, { 988.0f, 122.0f } },	// candy start (588, 272) - (150, 100) and + (400, -150)
		.rope_length = 300.0f,
		.start_angle = 45.0f,
		.pig = { 800.0f, 850.0f },
		.pig_radius = 50.0f,
		.bubble = { 800.0f, 562.5f },
		.magnet = { 320.0f, 562.5f },
		.star = { { 900.0f, 550.0f }, { 800.0f, 700.0f }, { 800.0f, 50.0f } },
	},
};

const LevelData* Level_Get(int level)
{
	if (level < 1)
		level = 1;
	if (level > LEVEL_COUNT)
		level = LEVEL_COUNT;
	return &levels[level - 1];
}
