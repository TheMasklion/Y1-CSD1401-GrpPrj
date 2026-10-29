//---------------------------------------------------------
// file:	collision.h
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Reusable collision tests. Include this file from any state or object
//			(candy-pig, star-candy, ...) that needs to know if two hitboxes touch.
//---------------------------------------------------------

#pragma once

// Returns 1 if two circles overlap or just touch, 0 if they do not.
// Positions are the circle CENTRES, and r1 / r2 are RADII (not diameters, unlike CP_Graphics_DrawCircle).
int IsCircleOverlapping(float x1, float y1, float r1, float x2, float y2, float r2);