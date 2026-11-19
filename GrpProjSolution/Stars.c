//---------------------------------------------------------
// file:	Stars.c
// author:	Jayden Tan
// email:	jayden.tan@digipen.edu
//
// brief:	Draws a star image at the given coordinates. The image is loaded once and reused for all stars.
// Star image is 32x32 pixels, and the draw function takes the x and y coordinates for placement.
//---------------------------------------------------------


#include <cprocessing.h>
#include "Collision.h"
#include "PigCandy.h"
#include "Stars.h"

CP_Image star_img = NULL;

typedef struct
{
	int id;			// Unique identifier for the star
	int collected; // Flag to indicate if the star has been collected
} id;

id ID[3]; // Initiate 3 IDs for the 3 stars, each with a unique ID



Star YellowStar;
Star EmptyStar;
#define star_radius_in_game 50.0f // Define the radius for the star collision detection, which is half of the star image size (32x32 pixels)

int star_count;

void Stars_Init(void)
{
	star_count = 0; // Initialize the star count to 0

	if (star_img == NULL)
	{
		CP_Settings_ImageFilterMode(CP_IMAGE_FILTER_NEAREST);
		star_img = CP_Image_Load("./Assets/Stars.png");
	}

	//set YellowStar params with unique id set for each
	for (int i = 0; i < 3; i++)
	{
		ID[i] = (id)
		{
			.id = i + 1,			// Unique identifier for each star (1, 2, 3)
			.collected = 0			// Initialize the collected flag to 0 (not collected)
		};
	}

	YellowStar = (Star)
	{
		.sprite_left = 0.0f,
		.sprite_top = 0.0f,
		.sprite_right = (float)CP_Image_GetWidth(star_img) / 2.0f,
		.sprite_bottom = (float)CP_Image_GetHeight(star_img)
	};

	EmptyStar = (Star)
	{
		.sprite_left = (float)CP_Image_GetWidth(star_img) / 2.0f,
		.sprite_top = 0.0f,
		.sprite_right = (float)CP_Image_GetWidth(star_img),
		.sprite_bottom = (float)CP_Image_GetHeight(star_img)
	};
	
}

void Stars_Draw(int star_id, float x_star, float y_star)//modifiable x and y coordinates for placement of star
{
	int index = star_id - 1; // Convert star_id to index (0, 1, 2) in the struct array
	if (!ID[index].collected) //meaning not collected, draw the star image at the given coordinates
	{
		//draw the star image at the given coordinates, using the appropriate frame from the image based on the star_id
		CP_Image_DrawSubImage(star_img, x_star, y_star, 100.0f, 100.0f,
			YellowStar.sprite_left, YellowStar.sprite_top, YellowStar.sprite_right, YellowStar.sprite_bottom, 255);

		if (IsCircleOverlapping(x_star, y_star, star_radius_in_game, candy.x, candy.y, candy.r))
		{
			ID[index].collected = 1; // Set the star_collected flag to 1 and stop drawing the star
			star_count++;

		}
	}
}

void Star_Counter(void)
{
	int i; // Loop through the stars and draw the collected ones
	//draw yellow star for each star collected, up to 3 stars
	for (i = 1; i <= star_count; i++)
	{
		//draw yellow star
		CP_Image_DrawSubImage(star_img, 50.0f*i, 50.0f, 100.0f, 100.0f,
			YellowStar.sprite_left, YellowStar.sprite_top, YellowStar.sprite_right, YellowStar.sprite_bottom, 255);
	}
	
	for (; i <= 3; i++)
	//draw empty star
	CP_Image_DrawSubImage(star_img, 50.0f*i, 50.0f, 100.0f, 100.0f,
		EmptyStar.sprite_left, EmptyStar.sprite_top, EmptyStar.sprite_right, EmptyStar.sprite_bottom, 255);
}

void Stars_Exit(void)
{
	CP_Image_Free(star_img);
	star_img = NULL;
}

