//---------------------------------------------------------
// file:	collision.c
// author:	Gasper Ng
// email:	NG.S@digipen.edu
//
// brief:	Reusable collision tests. Circle vs circle for now.
//---------------------------------------------------------

#include "collision.h"

// Two circles overlap when the distance between their centres is no more than the sum of their radii.
// Both sides are squared so no square root is needed (sqrt is slow, and squaring keeps the answer the same
// because both sides are never negative).
int IsCircleOverlapping(float x1, float y1, float r1, float x2, float y2, float r2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float distance_squared = dx * dx + dy * dy;
	float radius_sum = r1 + r2;

	// <= so circles that only touch count as overlapping
	if (distance_squared <= radius_sum * radius_sum)
		return 1;
	return 0;
}