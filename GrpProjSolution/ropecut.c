/**
@file    ropecut.c
@author  Kavian Wee
@email   k.wee@digipen.edu
@brief   Implementation of rope cutting mechanics, physics state, and lifecycle management.
*/

#include "ropecut.h"

/** @brief Maximum delta time step to prevent physics instability (1/30s). */
#define ROPECUT_MAX_DT 0.0333f

/** @brief Epsilon value for floating-point distance comparisons. */
#define ROPECUT_EPSILON 0.0001f

// ================================================================== 
// Geometry (Simple Distance)
// ==================================================================

static float Cut_DistancePointToSegment(CP_Vector p, CP_Vector a, CP_Vector b)
{
	// Create vectors for the swipe path (ab) and point relative to swipe start (ap)
	CP_Vector ab = CP_Vector_Subtract(b, a);
	CP_Vector ap = CP_Vector_Subtract(p, a);

	// Get the squared length of the swipe to avoid an expensive sqrt() call yet
	float len2 = CP_Vector_DotProduct(ab, ab);
	float t;

	// If the mouse wasn't dragged (just clicked), return straight distance to the click
	if (len2 < ROPECUT_EPSILON)
		return CP_Vector_Distance(p, a);

	// Vector projection: finding where the hinge drops perpendicularly onto the swipe line.
	// Clamped between 0.0 and 1.0 to ensure the point stays on the actual physical segment.
	t = CP_Math_ClampFloat(CP_Vector_DotProduct(ap, ab) / len2, 0.0f, 1.0f);

	// Calculate the exact closest point on the line, and return the distance to it
	return CP_Vector_Distance(p, CP_Vector_Add(a, CP_Vector_Scale(ab, t)));
}

static int Cut_FindHinge(const VerletRope* rope, CP_Vector p, CP_Vector q, float radius)
{
	int n = rope->point_count;
	// Start loop at index 1 if the first point is pinned to the ceiling
	int k_min = rope->points[0].pinned ? 1 : 0;
	int i;

	// Need at least two points to make a cuttable segment
	if (n < 2) return -1;

	// Iterate through all joints (stopping before the very last point, which holds the candy)
	for (i = k_min; i < n - 1; ++i)
	{
		// If the swipe passed closer to this hinge than the blade's thickness
		if (Cut_DistancePointToSegment(rope->points[i].pos, p, q) <= radius)
		{
			return i; // collision detected. Return the index of the severed hinge.
		}
	}
	return -1;
}

// ================================================================== 
// Cutting Logic
// ==================================================================

static RopeCutPiece* Cut_NewPiece(RopeCutter* c, const VerletRope* src, int first, int last, int keep_pin, int has_candy)
{
	RopeCutPiece* piece;
	int i;

	// Abort if we hit the 32-piece limit or if the requested indices are invalid
	if (c->loose_count >= ROPECUT_MAX_LOOSE || last < first)
		return NULL;

	// Grab the next available pre-allocated slot in the array
	piece = &c->loose[c->loose_count++];

	// Inherit physics settings from the source rope
	piece->rope.point_count = last - first + 1;
	piece->rope.segment_length = src->segment_length;
	piece->rope.gravity = src->gravity;
	piece->rope.damping = c->loose_damping; // Apply higher air resistance so it falls slower
	piece->rope.iterations = src->iterations;

	// Copy over the specific joints for this sliced segment
	for (i = 0; i < piece->rope.point_count; ++i)
	{
		piece->rope.points[i] = src->points[first + i];

		// Unpin the points so they fall, unless instructed to keep the top point anchored
		if (!(keep_pin && i == 0))
			piece->rope.points[i].pinned = 0;
	}

	// Reset lifecycle trackers
	piece->has_candy = has_candy;
	piece->vanishing = 0;
	piece->vanish_left = 0.0f;
	piece->cooldown = c->recut_cooldown; // Brief invincibility to prevent double-cuts
	return piece;
}

static void Cut_RemovePiece(RopeCutter* c, int j)
{
	// "Swap and Pop" deletion: Move the very last active piece in the array into the 
	// slot we want to delete, then shrink the count. This prevents empty gaps in the array
	// and is much faster than shifting every element down.
	c->loose[j] = c->loose[c->loose_count - 1];
	--c->loose_count;
}



static int Cut_Split(RopeCutter* c, VerletRope* src, int k, int src_has_candy)
{
	RopeCutPiece* tail;
	int n = src->point_count;
	int needed = c->vanish_on ? 3 : 1; // We might need up to 3 new slots if vanishing is active

	// Check if there is enough room in the array before attempting to split
	if (c->loose_count + needed > ROPECUT_MAX_LOOSE)
		return 0;

	// Create the bottom half of the cut rope (the "tail" that falls away)
	tail = Cut_NewPiece(c, src, k + 1, n - 1, 0, src_has_candy);

	// Shorten the original rope array so it now ends exactly at the cut point
	src->point_count = k + 1;

	// Handle the cosmetic fade-out effects
	if (c->vanish_on)
	{
		int tail_index = (int)(tail - c->loose); // Calculate where 'tail' is in the array

		// If the bottom half was holding the candy...
		if (src_has_candy)
		{
			// ...slice the candy off entirely so it falls on its own.
			Cut_NewPiece(c, &tail->rope, tail->rope.point_count - 1, tail->rope.point_count - 1, 0, 1);
			tail->rope.point_count -= 1; // Remove the candy joint from the tail piece
			tail->has_candy = 0;
		}

		// Trigger the fade-out animation for the now-empty tail piece
		tail->vanishing = 1;
		tail->vanish_left = c->vanish_time;

		// If the top half is still attached to the ceiling, make the little dangling stub fade out too
		if (src->points[0].pinned && k >= 1)
		{
			RopeCutPiece* stub = Cut_NewPiece(c, src, 0, k, 1, 0);
			stub->vanishing = 1;
			stub->vanish_left = c->vanish_time;
		}

		// The anchored rope is now just a single invisible pin point
		src->point_count = 1;

		// Cleanup: If the tail piece was so short it's effectively nothing, delete it immediately
		if (tail->rope.point_count < 2)
			Cut_RemovePiece(c, tail_index);
	}
	// If vanishing is off, just clean up useless tiny segments
	else if (tail->rope.point_count < 2 && !tail->has_candy)
	{
		Cut_RemovePiece(c, (int)(tail - c->loose));
	}
	return 1;
}

// ================================================================== 
// Public Functions
// ==================================================================

void RopeCutter_Init(RopeCutter* c, VerletRope* rope)
{
	if (c == NULL) return;

	c->attached = rope;
	c->candy_attached = (rope != NULL);
	c->attached_cooldown = 0.0f;
	c->candy_pos = VerletRope_GetEnd(rope); // Track where the payload starts

	c->loose_count = 0;
	c->dragging = 0;
	c->drag_prev = CP_Vector_Zero();

	// Define the "kill box" slightly outside the window.
	// If loose ropes fall past these coordinates, they are deleted to save memory.
	c->kill_y = (float)CP_System_GetWindowHeight() + ROPECUT_OFFSCREEN_MARGIN;
	c->kill_x_min = -ROPECUT_OFFSCREEN_MARGIN;
	c->kill_x_max = (float)CP_System_GetWindowWidth() + ROPECUT_OFFSCREEN_MARGIN;

	// Load default configurable values
	c->vanish_on = 1;
	c->blade_radius = ROPECUT_DEFAULT_BLADE_RADIUS;
	c->loose_damping = ROPECUT_DEFAULT_LOOSE_DAMPING;
	c->recut_cooldown = ROPECUT_DEFAULT_RECUT_COOLDOWN;
	c->vanish_time = ROPECUT_DEFAULT_VANISH_TIME;
}

int RopeCutter_HandleMouse(RopeCutter* c)
{
	CP_Vector now;
	CP_Vector from;

	if (c == NULL) return 0;

	// If the player releases the mouse, stop tracking the drag
	if (!CP_Input_MouseDown(MOUSE_BUTTON_LEFT))
	{
		c->dragging = 0;
		return 0;
	}

	// Capture current mouse coordinates
	now = CP_Vector_Set(CP_Input_GetMouseX(), CP_Input_GetMouseY());

	// If this is the very first frame of the click, just record the start position
	if (!c->dragging)
	{
		c->dragging = 1;
		c->drag_prev = now;
		return 0;
	}

	from = c->drag_prev;
	c->drag_prev = now;

	// Only process a cut if the mouse actually moved a measurable distance this frame
	if (CP_Vector_Distance(from, now) < ROPECUT_EPSILON)
		return 0;

	// Dispatch the line segment (last frame's position to this frame's position) to the collision logic
	return RopeCutter_Cut(c, from, now);
}

int RopeCutter_Cut(RopeCutter* c, CP_Vector from, CP_Vector to)
{
	int cuts = 0, before, j, k;

	if (c == NULL) return 0;

	// Cache the current piece count, so we don't accidentally test against 
	// new pieces we spawn *during* this loop.
	before = c->loose_count;

	// Test against the main anchored rope
	if (c->attached != NULL && c->attached_cooldown <= 0.0f)
	{
		k = Cut_FindHinge(c->attached, from, to, c->blade_radius);
		if (k >= 0 && Cut_Split(c, c->attached, k, c->candy_attached))
		{
			c->candy_attached = 0; // Payload has been detached from the main rope
			c->attached_cooldown = c->recut_cooldown;
			++cuts;
		}
	}

	// Test against all active severed pieces
	for (j = 0; j < before; ++j)
	{
		RopeCutPiece* piece = &c->loose[j];

		// Skip pieces that are currently invincible or already fading out
		if (piece->cooldown > 0.0f || piece->vanishing) continue;

		k = Cut_FindHinge(&piece->rope, from, to, c->blade_radius);
		if (k >= 0 && Cut_Split(c, &piece->rope, k, piece->has_candy))
		{
			piece->has_candy = 0;
			piece->cooldown = c->recut_cooldown;
			++cuts;
		}
	}
	return cuts; // Return total number of ropes sliced this frame
}

void RopeCutter_Update(RopeCutter* c, float dt)
{
	int j;
	int i;
	int offscreen;

	if (c == NULL || dt <= 0.0f) return;

	// Cap the delta time to prevent physics explosions if the game lags
	if (dt > ROPECUT_MAX_DT) dt = ROPECUT_MAX_DT;

	// Decrement the main rope's invincibility timer
	if (c->attached_cooldown > 0.0f)
		c->attached_cooldown -= dt;

	// Iterate through all loose pieces (using a manual while-style loop to handle mid-loop deletions safely)
	for (j = 0; j < c->loose_count; )
	{
		RopeCutPiece* piece = &c->loose[j];

		// Step the Verlet physics simulation for this piece
		VerletRope_Update(&piece->rope, dt);

		// Decrement timers
		if (piece->cooldown > 0.0f) piece->cooldown -= dt;
		if (piece->vanishing) piece->vanish_left -= dt;

		// Check if the entire rope has fallen outside the kill box boundaries
		offscreen = 1;
		for (i = 0; i < piece->rope.point_count; ++i)
		{
			CP_Vector p = piece->rope.points[i].pos;
			if (p.y <= c->kill_y && p.x >= c->kill_x_min && p.x <= c->kill_x_max)
				offscreen = 0; // Found at least one joint still on screen
		}

		// Delete the piece if it finished fading out OR fell off screen
		if ((piece->vanishing && piece->vanish_left <= 0.0f) || offscreen)
		{
			// If this deleted piece held the payload, cache its final position
			if (piece->has_candy) c->candy_pos = VerletRope_GetEnd(&piece->rope);

			Cut_RemovePiece(c, j); // This swaps the last element into index 'j'
			// Notice we DO NOT increment 'j' here, because we need to process 
			// the newly swapped-in element on the next loop iteration.
		}
		else
		{
			++j; // Only advance to the next index if we didn't delete anything
		}
	}
}

void RopeCutter_Draw(const RopeCutter* c, CP_Color rope_color, float thickness)
{
	int j;
	if (c == NULL) return;

	for (j = 0; j < c->loose_count; ++j)
	{
		float alpha = 1.0f;

		// If the piece is dying, calculate its transparency as a percentage (0.0 to 1.0)
		if (c->loose[j].vanishing && c->vanish_time > 0.0f)
			alpha = CP_Math_ClampFloat(c->loose[j].vanish_left / c->vanish_time, 0.0f, 1.0f);

		// Blend the alpha value into the requested drawing color
		CP_Color fade = CP_Color_Create(rope_color.r, rope_color.g, rope_color.b, (int)((float)rope_color.a * alpha));
		VerletRope_Draw(&c->loose[j].rope, fade, thickness);
	}
}

void RopeCutter_DrawPoints(const RopeCutter* c, CP_Color color, float diameter)
{
	int j;
	if (c == NULL) return;

	for (j = 0; j < c->loose_count; ++j)
	{
		float alpha = 1.0f;

		// Identical alpha math for the hinge joints
		if (c->loose[j].vanishing && c->vanish_time > 0.0f)
			alpha = CP_Math_ClampFloat(c->loose[j].vanish_left / c->vanish_time, 0.0f, 1.0f);

		CP_Color fade = CP_Color_Create(color.r, color.g, color.b, (int)((float)color.a * alpha));
		VerletRope_DrawPoints(&c->loose[j].rope, fade, diameter);
	}
}

// Retrieves the absolute X/Y world position of the objective payload
CP_Vector RopeCutter_GetCandy(const RopeCutter* c)
{
	int j;
	if (c == NULL) return CP_Vector_Zero();

	// Check if it's still safe on the main rope
	if (c->candy_attached && c->attached != NULL) return VerletRope_GetEnd(c->attached);

	// Check if it's falling on a loose piece
	for (j = 0; j < c->loose_count; ++j)
	{
		if (c->loose[j].has_candy) return VerletRope_GetEnd(&c->loose[j].rope);
	}

	// If all ropes are gone, return its last known cached position
	return c->candy_pos;
}

// Reports the status of the payload to the main game loop (win/loss detection)
RopeCutCandyState RopeCutter_GetCandyState(const RopeCutter* c)
{
	int j;
	if (c == NULL) return ROPECUT_CANDY_GONE;
	if (c->candy_attached && c->attached != NULL) return ROPECUT_CANDY_ATTACHED;

	for (j = 0; j < c->loose_count; ++j)
	{
		if (c->loose[j].has_candy) return ROPECUT_CANDY_FALLING;
	}
	return ROPECUT_CANDY_GONE;
}

int RopeCutter_GetLooseCount(const RopeCutter* c)
{
	return (c == NULL) ? 0 : c->loose_count;
}