// file:	ropecut.c
// author:	Kavian Wee
// email:	k.wee@digipen.edu
//
// Overview: see ropecut.h for the five steps of a cut and the hook-up.
//
//			Nothing here reaches into rope.c. A cut changes point_count, copies hinges and edits
//			prev_pos (which is how Verlet stores velocity) through the public VerletRope struct, and
//			the loose pieces are stepped and drawn with VerletRope_Update / VerletRope_Draw unchanged.
//
//			Geometry: a blade (p, q) hits a rope segment (a, b) when the two segments properly cross
//			(the end points of each lie on opposite sides of the other, tested with cross products),
//			or when they come within blade_radius of each other. Two segments that do not cross are
//			closest at one of their four end points, so four point-to-segment distances cover that.

#include "ropecut.h"

#define ROPECUT_MAX_DT			(1.0f / 30.0f)	// same clamp as rope.c, so the snap is scaled like the rope's own step
#define ROPECUT_MIN_DT			(1.0f / 240.0f)
#define ROPECUT_EPSILON			0.0001f			// lengths below this have no direction
#define ROPECUT_MAX_BLADE_SPEED	3000.0f			// px/s: blade velocity used for the push and the fray is capped here
#define ROPECUT_MAX_KICK		14.0f			// px per frame: cap on the push the blade gives the cut ends
#define ROPECUT_RECOIL_SPREAD	3				// hinges on each side of the cut that share the snap (1, 2/3, 1/3)
#define ROPECUT_FIBER_DRAG		3.0f			// per second: air drag on the fibres
#define ROPECUT_FIBER_MIN_SPEED	60.0f			// px/s
#define ROPECUT_FIBER_MAX_SPEED	240.0f
#define ROPECUT_FIBER_MIN_LIFE	0.3f			// seconds
#define ROPECUT_FIBER_MAX_LIFE	0.65f
#define ROPECUT_FIBER_MIN_LEN	3.0f			// px
#define ROPECUT_FIBER_MAX_LEN	9.0f
#define ROPECUT_FIBER_MAX_SPIN	720.0f			// degrees per second
#define ROPECUT_FIBER_SNAP		0.35f			// fraction of the recoil speed the fibres leave with
#define ROPECUT_FIBER_PUSH		0.3f			// fraction of the blade velocity the fibres leave with
#define ROPECUT_SLASH_WEIGHT	3.0f			// px, at full brightness
#define ROPECUT_SLASH_R			255
#define ROPECUT_SLASH_G			255
#define ROPECUT_SLASH_B			255
#define ROPECUT_DEFAULT_GRAVITY	CP_Vector_Set(0.0f, 980.0f)	// for the fibres if there is no rope to copy from

// ------------------------------------------------------------------ geometry

// Distance from point p to segment ab.
static float Cut_PointSegmentDistance(CP_Vector p, CP_Vector a, CP_Vector b)
{
	CP_Vector ab = CP_Vector_Subtract(b, a);
	CP_Vector ap = CP_Vector_Subtract(p, a);
	float len2 = CP_Vector_DotProduct(ab, ab);
	float t;

	if (len2 < ROPECUT_EPSILON)
		return CP_Vector_Distance(p, a);
	t = CP_Math_ClampFloat(CP_Vector_DotProduct(ap, ab) / len2, 0.0f, 1.0f);	// nearest point along ab
	return CP_Vector_Distance(p, CP_Vector_Add(a, CP_Vector_Scale(ab, t)));
}

// 1 if blade pq properly crosses segment ab. *t_out = where on ab (0 at a, 1 at b).
static int Cut_SegmentsCross(CP_Vector p, CP_Vector q, CP_Vector a, CP_Vector b, float* t_out)
{
	CP_Vector pq = CP_Vector_Subtract(q, p);
	CP_Vector ab = CP_Vector_Subtract(b, a);
	float d1 = CP_Vector_CrossProduct(pq, CP_Vector_Subtract(a, p));	// side of the blade a is on
	float d2 = CP_Vector_CrossProduct(pq, CP_Vector_Subtract(b, p));	// side of the blade b is on
	float d3 = CP_Vector_CrossProduct(ab, CP_Vector_Subtract(p, a));	// side of the segment p is on
	float d4 = CP_Vector_CrossProduct(ab, CP_Vector_Subtract(q, a));	// side of the segment q is on

	if (d1 * d2 < 0.0f && d3 * d4 < 0.0f)
	{
		*t_out = d1 / (d1 - d2);	// a and b are on opposite sides, so d1 != d2
		return 1;
	}
	return 0;
}

// 1 if the blade pq crosses segment ab or passes within radius of it.
static int Cut_BladeHitsSegment(CP_Vector p, CP_Vector q, CP_Vector a, CP_Vector b, float radius, float* t_out)
{
	if (Cut_SegmentsCross(p, q, a, b, t_out))
		return 1;
	if (Cut_PointSegmentDistance(p, a, b) <= radius || Cut_PointSegmentDistance(q, a, b) <= radius ||
		Cut_PointSegmentDistance(a, p, q) <= radius || Cut_PointSegmentDistance(b, p, q) <= radius)
	{
		*t_out = 0.5f;
		return 1;
	}
	return 0;
}

// Index k of the first segment (hinges k, k+1) of rope that the blade hits, or -1.
// A cut at k leaves hinges 0..k and k+1..n-1. A lone hinge draws nothing, so a free piece is never
// cut down to a lone head hinge (k_min = 1; the pinned anchor may stand alone: whole rope drops),
// and a lone tail hinge is dropped outright by Cut_Split unless it is the candy. A 2-hinge free
// piece therefore cannot be cut (k_min > k_max) and returns -1.
static int Cut_FindSegment(const VerletRope* rope, CP_Vector p, CP_Vector q, float radius, float* t_out)
{
	int n = rope->point_count;
	int k_min = rope->points[0].pinned ? 0 : 1;
	int k_max = n - 2;
	int k;

	if (n < 2 || k_min > k_max)
		return -1;
	for (k = 0; k < n - 1; ++k)
	{
		if (Cut_BladeHitsSegment(p, q, rope->points[k].pos, rope->points[k + 1].pos, radius, t_out))
		{
			if (k < k_min || k > k_max)
			{
				k = CP_Math_ClampInt(k, k_min, k_max);	// the blade touched the end of the piece: cut one segment in
				*t_out = 0.5f;
			}
			return k;
		}
	}
	return -1;
}

// End-to-end distance / rest length, 0..1. A hanging or swinging rope is ~1, a slack one less.
static float Cut_Tautness(const VerletRope* rope)
{
	float rest = rope->segment_length * (float)(rope->point_count - 1);
	float span;

	if (rope->point_count < 2 || rest < ROPECUT_EPSILON)
		return 0.0f;
	span = CP_Vector_Distance(rope->points[0].pos, rope->points[rope->point_count - 1].pos);
	return CP_Math_ClampFloat(span / rest, 0.0f, 1.0f);
}

// ------------------------------------------------------------------ effects

static void Cut_AddSlash(RopeCutter* c, CP_Vector from, CP_Vector to)
{
	RopeCutSlash* s = &c->slashes[c->slash_next];
	c->slash_next = (c->slash_next + 1) % ROPECUT_MAX_SLASHES;
	s->from = from;
	s->to = to;
	s->life = ROPECUT_SLASH_LIFE;
}

// A free fibre slot: append, or recycle the one closest to dying.
static RopeCutFiber* Cut_NewFiber(RopeCutter* c)
{
	int j;
	int oldest = 0;

	if (c->fiber_count < ROPECUT_MAX_FIBERS)
		return &c->fibers[c->fiber_count++];
	for (j = 1; j < ROPECUT_MAX_FIBERS; ++j)
	{
		if (c->fibers[j].life < c->fibers[oldest].life)
			oldest = j;
	}
	return &c->fibers[oldest];
}

// Fibres burst out of the cut point. Half leave with each cut end (along the rope, away from the
// cut), all of them get a random direction and a share of the blade's motion.
static void Cut_SpawnFibers(RopeCutter* c, CP_Vector at, CP_Vector along, CP_Vector blade_vel)
{
	int i;
	for (i = 0; i < ROPECUT_FIBERS_PER_CUT; ++i)
	{
		RopeCutFiber* f = Cut_NewFiber(c);
		float angle = CP_Random_RangeFloat(0.0f, 360.0f);
		float speed = CP_Random_RangeFloat(ROPECUT_FIBER_MIN_SPEED, ROPECUT_FIBER_MAX_SPEED);
		float side = (i % 2 == 0) ? 1.0f : -1.0f;
		CP_Vector dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(angle), CP_Vector_Set(1.0f, 0.0f));
		CP_Vector snap = CP_Vector_Scale(along, side * c->recoil * ROPECUT_FIBER_SNAP);
		CP_Vector push = CP_Vector_Scale(blade_vel, ROPECUT_FIBER_PUSH);

		f->pos = CP_Vector_Add(at, CP_Vector_Scale(along, side * CP_Random_RangeFloat(0.0f, 4.0f)));
		f->vel = CP_Vector_Add(CP_Vector_Scale(dir, speed), CP_Vector_Add(snap, push));
		f->angle = angle;
		f->spin = CP_Random_RangeFloat(-ROPECUT_FIBER_MAX_SPIN, ROPECUT_FIBER_MAX_SPIN);
		f->length = CP_Random_RangeFloat(ROPECUT_FIBER_MIN_LEN, ROPECUT_FIBER_MAX_LEN);
		f->max_life = CP_Random_RangeFloat(ROPECUT_FIBER_MIN_LIFE, ROPECUT_FIBER_MAX_LIFE);
		f->life = f->max_life;
	}
}

// ------------------------------------------------------------------ the cut itself

// Verlet keeps velocity as pos - prev_pos (px per frame), so a velocity change is written into prev_pos.
static void Cut_AddVelocity(RopePoint* p, CP_Vector dv)
{
	p->prev_pos = CP_Vector_Subtract(p->prev_pos, dv);
}

// Tension release. The hinges next to the cut snap away from it along the rope (the upper end
// towards the anchor, the lower end away from it) with a speed that scales with how taut the rope
// was, and both get a push in the direction the blade moved. The snap is spread over a few hinges
// on each side with a falling share, which makes the whip travel visibly up the rope.
static void Cut_Recoil(RopeCutter* c, VerletRope* upper, VerletRope* lower, CP_Vector along, float tautness, CP_Vector blade_vel)
{
	float dt = c->last_dt;
	float snap = c->recoil * tautness * dt;					// px per frame
	CP_Vector kick = CP_Vector_Scale(blade_vel, c->blade_push * dt);
	float kick_len = CP_Vector_Length(kick);
	int i;

	if (kick_len > ROPECUT_MAX_KICK)
		kick = CP_Vector_Scale(kick, ROPECUT_MAX_KICK / kick_len);

	for (i = 0; i < ROPECUT_RECOIL_SPREAD; ++i)
	{
		float share = 1.0f - (float)i / (float)ROPECUT_RECOIL_SPREAD;	// 1, 2/3, 1/3
		int up = upper->point_count - 1 - i;	// the upper piece's cut end, then the hinges above it
		int lo = i;								// the lower piece's cut end, then the hinges below it

		if (up >= 0 && !upper->points[up].pinned)
			Cut_AddVelocity(&upper->points[up], CP_Vector_Add(CP_Vector_Scale(along, -snap * share), CP_Vector_Scale(kick, share)));
		if (lo < lower->point_count && !lower->points[lo].pinned)
			Cut_AddVelocity(&lower->points[lo], CP_Vector_Add(CP_Vector_Scale(along, snap * share), CP_Vector_Scale(kick, share)));
	}
}

// Copies hinges first..last of src into a new loose piece: same rest length, gravity and solver
// settings as the rope they came from, a touch more damping. Each hinge keeps its pos and prev_pos,
// so it keeps the velocity it had on the swing. keep_pin leaves hinge 0's pinned flag as it is (a
// fading stub still hangs from the anchor); otherwise every hinge is free. NULL if there is no room.
static RopeCutPiece* Cut_NewPiece(RopeCutter* c, const VerletRope* src, int first, int last, int keep_pin, int has_candy)
{
	RopeCutPiece* piece;
	int i;

	if (c->loose_count >= ROPECUT_MAX_LOOSE || last < first)
		return NULL;
	piece = &c->loose[c->loose_count++];
	piece->rope.point_count = last - first + 1;
	piece->rope.segment_length = src->segment_length;
	piece->rope.gravity = src->gravity;
	piece->rope.damping = c->loose_damping;
	piece->rope.iterations = src->iterations;
	for (i = 0; i < piece->rope.point_count; ++i)
	{
		piece->rope.points[i] = src->points[first + i];
		if (!(keep_pin && i == 0))
			piece->rope.points[i].pinned = 0;
	}
	piece->has_candy = has_candy;
	piece->landed = 0;
	piece->vanishing = 0;
	piece->vanish_left = 0.0f;
	piece->age = 0.0f;
	piece->cooldown = c->recut_cooldown;	// the blade that made it is still right here next frame
	return piece;
}

static void Cut_StartVanish(const RopeCutter* c, RopeCutPiece* piece)
{
	piece->vanishing = 1;
	piece->vanish_left = c->vanish_time;
}

// Removes loose piece j by moving the last piece into its slot (order does not matter).
static void Cut_RemovePiece(RopeCutter* c, int j)
{
	c->loose[j] = c->loose[c->loose_count - 1];
	--c->loose_count;
}

// Cuts src at segment k (between hinges k and k+1). t is where on the segment the blade hit (for
// the fray burst). Returns 0 if there is no room for the new pieces (the rope is then left whole).
//
//   keep mode (vanish_on = 0): hinges k+1..n-1 become a loose piece, src keeps hinges 0..k.
//   vanish mode (default):     the same, then the candy is split off into a piece of its own, the
//                              loose rope and the stub 0..k are set fading, and src is left with
//                              just its anchor hinge (draws nothing, cannot be cut again).
static int Cut_Split(RopeCutter* c, VerletRope* src, int k, float t, int src_has_candy, CP_Vector blade_vel)
{
	RopeCutPiece* tail;
	CP_Vector a = src->points[k].pos;
	CP_Vector b = src->points[k + 1].pos;
	CP_Vector along = CP_Vector_Subtract(b, a);				// down the rope, through the cut
	float length = CP_Vector_Length(along);
	float tautness = Cut_Tautness(src);						// measured before the split
	CP_Vector cut_point = CP_Vector_Add(a, CP_Vector_Scale(along, t));
	int n = src->point_count;
	int needed = c->vanish_on ? 3 : 1;						// vanish mode: loose rope, candy, stub

	if (c->loose_count + needed > ROPECUT_MAX_LOOSE)
		return 0;
	along = (length < ROPECUT_EPSILON) ? CP_Vector_Set(0.0f, 1.0f) : CP_Vector_Scale(along, 1.0f / length);

	// 1. the hinges below the cut move into a loose piece; the source keeps hinges 0..k.
	//    VerletRope_Update's reach check uses point_count, so the shorter rope has the shorter reach.
	tail = Cut_NewPiece(c, src, k + 1, n - 1, 0, src_has_candy);
	src->point_count = k + 1;

	// 2. recoil on both new ends
	Cut_Recoil(c, src, &tail->rope, along, tautness, blade_vel);

	// 3. fray
	Cut_SpawnFibers(c, cut_point, along, blade_vel);

	// 4. vanish mode: the rope is finished, only the candy goes on
	if (c->vanish_on)
	{
		int tail_index = (int)(tail - c->loose);

		if (src_has_candy)
		{
			// the candy becomes a piece of its own (pos and prev_pos copied: it keeps its momentum)
			Cut_NewPiece(c, &tail->rope, tail->rope.point_count - 1, tail->rope.point_count - 1, 0, 1);
			tail->rope.point_count -= 1;
			tail->has_candy = 0;
		}
		Cut_StartVanish(c, tail);

		if (src->points[0].pinned && k >= 1)
		{
			// the stub keeps hanging from the anchor (hinge 0 stays pinned) while it fades
			RopeCutPiece* stub = Cut_NewPiece(c, src, 0, k, 1, 0);
			Cut_StartVanish(c, stub);
		}
		src->point_count = 1;	// the game's rope is now just its anchor hinge

		if (tail->rope.point_count < 2)
			Cut_RemovePiece(c, tail_index);	// a lone rope hinge draws nothing: gone at once
	}
	else if (tail->rope.point_count < 2 && !tail->has_candy)
	{
		// keep mode, cut at the very end of a candy-less rope: the lone hinge draws nothing, so it is
		// simply gone (the fray marks where it went) instead of lingering as an invisible piece
		Cut_RemovePiece(c, (int)(tail - c->loose));
	}
	return 1;
}

// Floor contact for a loose piece, applied after VerletRope_Update. A hinge below the floor is put
// back on it; because Verlet keeps velocity as pos - prev_pos, the bounce and the friction are
// written into prev_pos: the downward speed comes back upwards scaled by floor_bounce, the sideways
// speed loses floor_friction of itself. The candy hinge rests on its rim (candy_radius above the
// floor), every other hinge on the floor line. The constraints that keep the segments at rest
// length run inside VerletRope_Update next frame, so the piece settles into a lying rope.
static void Cut_Floor(const RopeCutter* c, RopeCutPiece* piece)
{
	int last = piece->rope.point_count - 1;
	int i;

	for (i = 0; i <= last; ++i)
	{
		RopePoint* p = &piece->rope.points[i];
		float radius = (piece->has_candy && i == last) ? c->candy_radius : 0.0f;
		float limit = c->floor_y - radius;
		float vx;
		float vy;

		if (p->pos.y <= limit)
			continue;
		vx = p->pos.x - p->prev_pos.x;
		vy = p->pos.y - p->prev_pos.y;
		p->pos.y = limit;
		p->prev_pos.y = limit + vy * c->floor_bounce;
		p->prev_pos.x = p->pos.x - vx * (1.0f - c->floor_friction);
		piece->landed = 1;
	}
}

// The colour a piece is drawn in: the given colour, with its alpha scaled down while it fades.
static CP_Color Cut_PieceColor(const RopeCutter* c, const RopeCutPiece* piece, CP_Color color)
{
	float alpha = 1.0f;
	if (piece->vanishing && c->vanish_time > 0.0f)
		alpha = CP_Math_ClampFloat(piece->vanish_left / c->vanish_time, 0.0f, 1.0f);
	return CP_Color_Create(color.r, color.g, color.b, (int)((float)color.a * alpha));
}

// A loose piece is kept while any hinge of it could still be seen.
static int Cut_PieceOffscreen(const RopeCutter* c, const RopeCutPiece* piece)
{
	int i;
	for (i = 0; i < piece->rope.point_count; ++i)
	{
		CP_Vector p = piece->rope.points[i].pos;
		if (p.y <= c->kill_y && p.x >= c->kill_x_min && p.x <= c->kill_x_max)
			return 0;
	}
	return 1;
}

// ------------------------------------------------------------------ public

void RopeCutter_Init(RopeCutter* c, VerletRope* rope)
{
	int i;

	if (c == NULL)
		return;

	c->attached = rope;
	c->candy_attached = (rope != NULL);
	c->attached_cooldown = 0.0f;
	c->candy_pos = VerletRope_GetEnd(rope);

	c->loose_count = 0;
	c->fiber_count = 0;
	c->slash_next = 0;
	for (i = 0; i < ROPECUT_MAX_SLASHES; ++i)
		c->slashes[i].life = 0.0f;

	c->dragging = 0;
	c->drag_prev = CP_Vector_Zero();
	c->last_dt = 1.0f / 60.0f;

	c->kill_y = (float)CP_System_GetWindowHeight() + ROPECUT_OFFSCREEN_MARGIN;
	c->kill_x_min = -ROPECUT_OFFSCREEN_MARGIN;
	c->kill_x_max = (float)CP_System_GetWindowWidth() + ROPECUT_OFFSCREEN_MARGIN;

	c->vanish_on = 1;
	c->floor_on = 1;
	c->floor_y = (float)CP_System_GetWindowHeight();
	c->candy_radius = 0.0f;

	c->blade_radius = ROPECUT_DEFAULT_BLADE_RADIUS;
	c->recoil = ROPECUT_DEFAULT_RECOIL;
	c->blade_push = ROPECUT_DEFAULT_BLADE_PUSH;
	c->loose_damping = ROPECUT_DEFAULT_LOOSE_DAMPING;
	c->recut_cooldown = ROPECUT_DEFAULT_RECUT_COOLDOWN;
	c->floor_bounce = ROPECUT_DEFAULT_FLOOR_BOUNCE;
	c->floor_friction = ROPECUT_DEFAULT_FLOOR_FRICTION;
	c->vanish_time = ROPECUT_DEFAULT_VANISH_TIME;

	c->cuts_made = 0;
}

int RopeCutter_HandleMouse(RopeCutter* c)
{
	CP_Vector now;
	CP_Vector from;

	if (c == NULL)
		return 0;
	if (!CP_Input_MouseDown(MOUSE_BUTTON_LEFT))
	{
		c->dragging = 0;
		return 0;
	}
	now = CP_Vector_Set(CP_Input_GetMouseX(), CP_Input_GetMouseY());
	if (!c->dragging)
	{
		// press frame: the blade starts here, nothing to cut yet
		c->dragging = 1;
		c->drag_prev = now;
		return 0;
	}
	from = c->drag_prev;
	c->drag_prev = now;
	if (CP_Vector_Distance(from, now) < ROPECUT_EPSILON)
		return 0;		// held still: no blade motion, no cut
	return RopeCutter_Cut(c, from, now);
}

int RopeCutter_Cut(RopeCutter* c, CP_Vector from, CP_Vector to)
{
	int cuts = 0;
	int before;
	int j;
	int k;
	float t = 0.5f;
	float blade_speed;
	CP_Vector blade_vel;

	if (c == NULL)
		return 0;
	blade_vel = CP_Vector_Scale(CP_Vector_Subtract(to, from), 1.0f / c->last_dt);	// px/s
	blade_speed = CP_Vector_Length(blade_vel);
	if (blade_speed > ROPECUT_MAX_BLADE_SPEED)		// a blade given in one go (or a frame hitch) is not a hurricane
		blade_vel = CP_Vector_Scale(blade_vel, ROPECUT_MAX_BLADE_SPEED / blade_speed);
	Cut_AddSlash(c, from, to);
	before = c->loose_count;	// taken before any cut: a tail made by this stroke is not tested by it

	// the rope on the anchor
	if (c->attached != NULL && c->attached_cooldown <= 0.0f)
	{
		k = Cut_FindSegment(c->attached, from, to, c->blade_radius, &t);
		if (k >= 0 && Cut_Split(c, c->attached, k, t, c->candy_attached, blade_vel))
		{
			c->candy_attached = 0;	// the candy, if it was still on, went with the tail
			c->attached_cooldown = c->recut_cooldown;
			++cuts;
		}
	}

	// loose pieces already in the air can be cut again, once their cooldown is over. The cut ends
	// are pushed along with the blade, so without the cooldown a slow swipe would keep re-cutting
	// the same piece hinge by hinge on every frame.
	for (j = 0; j < before; ++j)
	{
		RopeCutPiece* piece = &c->loose[j];
		if (piece->cooldown > 0.0f || piece->vanishing)
			continue;
		k = Cut_FindSegment(&piece->rope, from, to, c->blade_radius, &t);
		if (k >= 0 && Cut_Split(c, &piece->rope, k, t, piece->has_candy, blade_vel))
		{
			piece->has_candy = 0;
			piece->cooldown = c->recut_cooldown;
			++cuts;
		}
	}

	c->cuts_made += cuts;
	return cuts;
}

void RopeCutter_Update(RopeCutter* c, float dt)
{
	CP_Vector gravity;
	int j;

	if (c == NULL || dt <= 0.0f)
		return;
	if (dt > ROPECUT_MAX_DT)
		dt = ROPECUT_MAX_DT;
	if (dt < ROPECUT_MIN_DT)
		dt = ROPECUT_MIN_DT;
	c->last_dt = dt;
	gravity = (c->attached != NULL) ? c->attached->gravity : ROPECUT_DEFAULT_GRAVITY;
	if (c->attached_cooldown > 0.0f)
		c->attached_cooldown -= dt;

	// loose pieces: the rope's own update, the floor, then drop the ones that have left the window
	for (j = 0; j < c->loose_count; )
	{
		RopeCutPiece* piece = &c->loose[j];
		VerletRope_Update(&piece->rope, dt);
		if (c->floor_on)
			Cut_Floor(c, piece);
		piece->age += dt;
		if (piece->cooldown > 0.0f)
			piece->cooldown -= dt;
		if (piece->vanishing)
			piece->vanish_left -= dt;
		if (piece->vanishing && piece->vanish_left <= 0.0f)
		{
			Cut_RemovePiece(c, j);	// faded out
		}
		else if (Cut_PieceOffscreen(c, piece))
		{
			if (piece->has_candy)
				c->candy_pos = VerletRope_GetEnd(&piece->rope);	// remembered for GetCandy once gone
			Cut_RemovePiece(c, j);
		}
		else
			++j;
	}

	// fibres: Euler step with gravity and drag, then drop the dead ones
	for (j = 0; j < c->fiber_count; )
	{
		RopeCutFiber* f = &c->fibers[j];
		f->vel = CP_Vector_Add(f->vel, CP_Vector_Scale(gravity, dt));
		f->vel = CP_Vector_Scale(f->vel, 1.0f / (1.0f + ROPECUT_FIBER_DRAG * dt));
		f->pos = CP_Vector_Add(f->pos, CP_Vector_Scale(f->vel, dt));
		f->angle += f->spin * dt;
		f->life -= dt;
		if (f->life <= 0.0f)
		{
			c->fibers[j] = c->fibers[c->fiber_count - 1];
			--c->fiber_count;
		}
		else
			++j;
	}

	// slash streaks fade
	for (j = 0; j < ROPECUT_MAX_SLASHES; ++j)
	{
		if (c->slashes[j].life > 0.0f)
			c->slashes[j].life -= dt;
	}
}

void RopeCutter_Draw(const RopeCutter* c, CP_Color rope_color, float thickness)
{
	int j;

	if (c == NULL)
		return;

	// loose rope, same look as the attached rope; a vanishing piece fades
	for (j = 0; j < c->loose_count; ++j)
		VerletRope_Draw(&c->loose[j].rope, Cut_PieceColor(c, &c->loose[j], rope_color), thickness);

	// fibres: short spinning lines in the rope colour, fading out
	CP_Settings_LineCapMode(CP_LINE_CAP_ROUND);
	CP_Settings_StrokeWeight(thickness * 0.5f);
	for (j = 0; j < c->fiber_count; ++j)
	{
		const RopeCutFiber* f = &c->fibers[j];
		float fade = CP_Math_ClampFloat(f->life / f->max_life, 0.0f, 1.0f);
		CP_Vector dir = CP_Vector_MatrixMultiply(CP_Matrix_Rotate(f->angle), CP_Vector_Set(1.0f, 0.0f));
		CP_Vector half = CP_Vector_Scale(dir, f->length * 0.5f);
		CP_Vector p0 = CP_Vector_Subtract(f->pos, half);
		CP_Vector p1 = CP_Vector_Add(f->pos, half);

		CP_Settings_Stroke(CP_Color_Create(rope_color.r, rope_color.g, rope_color.b, (int)(255.0f * fade)));
		CP_Graphics_DrawLine(p0.x, p0.y, p1.x, p1.y);
	}

	// blade streak: bright, thinning and fading
	for (j = 0; j < ROPECUT_MAX_SLASHES; ++j)
	{
		const RopeCutSlash* s = &c->slashes[j];
		float fade;

		if (s->life <= 0.0f)
			continue;
		fade = s->life / ROPECUT_SLASH_LIFE;
		CP_Settings_Stroke(CP_Color_Create(ROPECUT_SLASH_R, ROPECUT_SLASH_G, ROPECUT_SLASH_B, (int)(255.0f * fade)));
		CP_Settings_StrokeWeight(1.0f + ROPECUT_SLASH_WEIGHT * fade);
		CP_Graphics_DrawLine(s->from.x, s->from.y, s->to.x, s->to.y);
	}
	CP_Settings_LineCapMode(CP_LINE_CAP_BUTT);
}

void RopeCutter_DrawPoints(const RopeCutter* c, CP_Color color, float diameter)
{
	int j;

	if (c == NULL)
		return;
	for (j = 0; j < c->loose_count; ++j)
		VerletRope_DrawPoints(&c->loose[j].rope, Cut_PieceColor(c, &c->loose[j], color), diameter);
}

CP_Vector RopeCutter_GetCandy(const RopeCutter* c)
{
	int j;

	if (c == NULL)
		return CP_Vector_Zero();
	if (c->candy_attached && c->attached != NULL)
		return VerletRope_GetEnd(c->attached);
	for (j = 0; j < c->loose_count; ++j)
	{
		if (c->loose[j].has_candy)
			return VerletRope_GetEnd(&c->loose[j].rope);
	}
	return c->candy_pos;
}

RopeCutCandyState RopeCutter_GetCandyState(const RopeCutter* c)
{
	int j;

	if (c == NULL)
		return ROPECUT_CANDY_GONE;
	if (c->candy_attached && c->attached != NULL)
		return ROPECUT_CANDY_ATTACHED;
	for (j = 0; j < c->loose_count; ++j)
	{
		if (c->loose[j].has_candy)
			return c->loose[j].landed ? ROPECUT_CANDY_LANDED : ROPECUT_CANDY_FALLING;
	}
	return ROPECUT_CANDY_GONE;
}

int RopeCutter_GetLooseCount(const RopeCutter* c)
{
	return (c == NULL) ? 0 : c->loose_count;
}
