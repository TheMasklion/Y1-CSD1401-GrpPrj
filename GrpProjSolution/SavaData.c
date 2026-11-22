#include <stdio.h>
#include <cprocessing.h>
#include "Stars.h"
#include <stdlib.h>
#include "LevelData.h" // LEVEL_COUNT sizes the star array

#define star_offset 100.0f
#define starting_x x - star_offset

extern int current_level;
extern int total_levels;
extern Star YellowStar, EmptyStar;
extern CP_Image star_img;


//Check creation and existence of save data, create if not found
int InitialiseSaveData() {

	FILE* file = NULL;
	//check if file exists
	if (fopen_s(&file, "./SaveData.txt", "r") != 0 || file == NULL){

		//File doesn't exist, create it and write the number of levels
		if (fopen_s(&file, "./SaveData.txt", "w") != 0 || file == NULL) {
			printf("Error creating SaveData.txt\n");
			return -1; // Error code for file creation failure
		}

		//Create save file
		fprintf(file, "//Save Data//\n");

		//write Level 1 to Level n to the file, each on a new line
		for (int i = 1; i <= total_levels; i++) {
			fprintf(file, "Level %d - 0\n", i);
		}

		
	}
	fclose(file);
	return 0; // Success
}

//Returns Star value for specified level
int RetrieveStars(int num) {

	FILE* file = NULL;
	if (fopen_s(&file, "./SaveData.txt", "r") != 0 || file == NULL) {
		printf("Error opening SavaData.txt\n");
		return 0;
	}

	char buffer[100];

	//skip the first line //Save Data//
	fgets(buffer, sizeof(buffer), file);

	int levels, stars;
	while(fscanf_s(file, " Level %d - %d", &levels, &stars) == 2) {
		if (levels == num) {
			fclose(file);
			return stars;
		}
	}
	fclose(file);
	return 0;
}

//draws stars, init the star variables before drawing
void DrawSaveDataStars(float x, float y, int num) {

	//to initalise yellowstar and emptystar variables
	LoadStarSprite();

	int stars = RetrieveStars(num);
	int i;
	for (i = 0; i < stars; i++)
	{
		//draw yellow star
		CP_Image_DrawSubImage(star_img, starting_x + (100.0f * i), y, 100.0f, 100.0f,
			YellowStar.sprite_left, YellowStar.sprite_top, YellowStar.sprite_right, YellowStar.sprite_bottom, 255);
	}

	for (; i < 3; i++)
		//draw empty star
		CP_Image_DrawSubImage(star_img, starting_x + (100.0f * i), y, 100.0f, 100.0f,
			EmptyStar.sprite_left, EmptyStar.sprite_top, EmptyStar.sprite_right, EmptyStar.sprite_bottom, 255);
}


void UpdateSave(int level, int star_count)
{
	FILE* file = NULL;

	// Open file for reading
	if (fopen_s(&file, "./SaveData.txt", "r") != 0 || file == NULL) {
		printf("Error opening SaveData.txt\n");
		return;
	}

	int levels, stars;
	int savedStars[LEVEL_COUNT];

	if (level < 1 || level > total_levels)
		return;

	// Initialise array
	for (int i = 0; i < total_levels; i++) {
		savedStars[i] = 0;
	}

	// Skip first line //Save Data//
	char buffer[100];
	fgets(buffer, sizeof(buffer), file);

	// Read existing save data
	while (fscanf_s(file, " Level %d - %d", &levels, &stars) == 2) {
		if (levels >= 1 && levels <= total_levels) {
			savedStars[levels - 1] = stars;
		}
	}

	fclose(file);

	// Update current level's stars
	//overwrite only if star_count is bigger than saved 
	if (star_count > savedStars[level - 1])
		{
		savedStars[level - 1] = star_count;
		}

	// Reopen file and overwrite with updated data
	if (fopen_s(&file, "./SaveData.txt", "w") != 0 || file == NULL) {
		printf("Error opening SaveData.txt\n");
		return;
	}

	fprintf(file, "//Save Data//\n");

	for (int i = 0; i < total_levels; i++) {
		fprintf(file, "Level %d - %d\n", i + 1, savedStars[i]);
	}

	fclose(file);
}