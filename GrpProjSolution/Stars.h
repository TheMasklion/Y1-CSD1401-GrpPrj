//Structure of a star, containing its in-game width and height, as well as the width and height of the frame in the image.
//header guards to prevent multiple inclusions of the header file for the star structure and related functions.
#ifndef STARS_H
#define STARS_H
typedef struct
{
	float sprite_left;
	float sprite_top;
	float sprite_right;
	float sprite_bottom;


} Star;

extern Star YellowStar;
extern Star EmptyStar;
#endif // !STARS_H



void Stars_Init(void);
void Stars_Draw(int star_id, float x_star, float y_star);
void Stars_Exit(void);

void Star_Counter(void);


