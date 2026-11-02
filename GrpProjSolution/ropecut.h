/**
@file        ropecut.h
@author      Kavian Wee
@email       k.wee@digipen.edu
@brief       Rope cutting mechanics and state management for physics-based puzzles.
  
This module handles the intersection math, physics state, and lifecycle of split Verlet rope segments. It manages the transition of ropes from anchored
states to independent loose pieces, handles fade-out animations, and tracks the state of attached payloads (candy).
 */

#ifndef ROPECUT_H
#define ROPECUT_H

#include "cprocessing.h"
#include "rope.h" /* Assumed dependency for VerletRope struct */

 /** @brief Maximum number of loose rope pieces allowed on screen simultaneously. */
#define ROPECUT_MAX_LOOSE 32

/** @brief Margin outside the window boundaries before a piece is culled. */
#define ROPECUT_OFFSCREEN_MARGIN 100.0f

/** @brief Default thickness of the swipe blade collision radius. */
#define ROPECUT_DEFAULT_BLADE_RADIUS 15.0f

/** @brief Default physics damping applied to severed rope pieces. */
#define ROPECUT_DEFAULT_LOOSE_DAMPING 0.98f

/** @brief Cooldown time in seconds before a newly cut piece can be cut again. */
#define ROPECUT_DEFAULT_RECUT_COOLDOWN 0.1f

/** @brief Duration in seconds for a disconnected rope stub to fade out and vanish. */
#define ROPECUT_DEFAULT_VANISH_TIME 0.5f

/**
 * @enum RopeCutCandyState
 * @brief Represents the current physical state and attachment of the payload.
 */
typedef enum {
    ROPECUT_CANDY_ATTACHED, //Payload is bound to the main anchored rope
    ROPECUT_CANDY_FALLING,  //Payload is attached to a severed, falling rope piece
    ROPECUT_CANDY_GONE      //Payload has fallen off-screen or been destroyed
} RopeCutCandyState;

/**
 * @struct RopeCutPiece
 * @brief Represents an independent, severed segment of a rope.
 */
typedef struct {
    VerletRope rope;     //The physical rope data for this segment.
    int has_candy;       //Flag indicating if the payload is attached to this piece.
    int vanishing;       //Flag indicating if this piece is currently fading out.
    float vanish_left;   //Remaining time in seconds before the piece is culled.
    float cooldown;      //Remaining time in seconds before this piece can be cut again.
} RopeCutPiece;

/** 
@struct RopeCutter
@brief The core manager for rope cutting interactions and loose piece tracking.
 */
typedef struct {
    VerletRope* attached;                   //Pointer to the primary anchored rope.
    int candy_attached;                     //Flag indicating if the candy is on the main rope.
    float attached_cooldown;                //Recut cooldown for the main rope.
    CP_Vector candy_pos;                    //Last known position of the candy.

    int loose_count;                        //Current number of active loose pieces.
    RopeCutPiece loose[ROPECUT_MAX_LOOSE];  //Array tracking all severed rope segments.

    int dragging;                           //Flag indicating if the user is currently swiping.
    CP_Vector drag_prev;                    //Previous frame's mouse position for swipe logic.

    float kill_y;                           //Bottom boundary for culling pieces.
    float kill_x_min;                       //Left boundary for culling pieces.
    float kill_x_max;                       //Right boundary for culling pieces.

    int vanish_on;                          //Toggle for whether stubs should fade out.
    float blade_radius;                     //Hitbox radius for mouse swipe intersections.      
    float loose_damping;                    //Physics damping applied to severed pieces.
    float recut_cooldown;                   //Base recut cooldown applied to new pieces.
    float vanish_time;                      //Base duration for vanish fade-out animations.
} RopeCutter;

/**
@brief Initializes the rope cutter manager with default configurations.

@param c Pointer to the RopeCutter instance to initialize.
@param rope Pointer to the primary VerletRope to track.
 */
void RopeCutter_Init(RopeCutter* c, VerletRope* rope);

/**
@brief Polls mouse input to detect and process swipe gestures.

@param c Pointer to the active RopeCutter instance.
@return int The total number of successful cuts made this frame.
 */
int RopeCutter_HandleMouse(RopeCutter* c);

/**
 @brief Calculates intersections between a line segment and all active ropes.
 
 @param c Pointer to the active RopeCutter instance.
 @param from The starting coordinates of the swipe segment.
 @param to The ending coordinates of the swipe segment.
 @return int The total number of successful cuts made.
 */
int RopeCutter_Cut(RopeCutter* c, CP_Vector from, CP_Vector to);

/**
 @brief Advances the physics simulation for all loose segments and manages culling.
 
 @param c Pointer to the active RopeCutter instance.
 @param dt Delta time in seconds since the last frame.
 */
void RopeCutter_Update(RopeCutter* c, float dt);

/**
@brief Renders all loose rope segments to the screen.
 
@param c Pointer to the active RopeCutter instance.
@param rope_color The CP_Color to tint the ropes (alpha is dynamically adjusted for vanishing pieces).
@param thickness The stroke thickness for drawing the rope segments.
 */
void RopeCutter_Draw(const RopeCutter* c, CP_Color rope_color, float thickness);

/**
 @brief Renders the individual hinge points of all loose rope segments.
 
 @param c Pointer to the active RopeCutter instance.
 @param color The CP_Color to tint the points.
 @param diameter The diameter of the rendered points.
 */
void RopeCutter_DrawPoints(const RopeCutter* c, CP_Color color, float diameter);

/**
 @brief Retrieves the current spatial coordinates of the payload/candy.
 
 @param c Pointer to the active RopeCutter instance.
 @return CP_Vector The absolute (X, Y) coordinates of the payload.
 */
CP_Vector RopeCutter_GetCandy(const RopeCutter* c);

/**
 @brief Retrieves the current attachment state of the payload/candy.
 
 @param c Pointer to the active RopeCutter instance.
 @return RopeCutCandyState The enumerated state of the payload.
 */
RopeCutCandyState RopeCutter_GetCandyState(const RopeCutter* c);

/**
 @brief Gets the total number of severed loose pieces currently active.
 
 @param c Pointer to the active RopeCutter instance.
 @return int The count of active loose pieces.
 */
int RopeCutter_GetLooseCount(const RopeCutter* c);

#endif /* ROPECUT_H */