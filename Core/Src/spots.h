#ifndef SRC_SPOTS_H_
#define SRC_SPOTS_H_
#include "stm32l4xx_hal.h"

#define ARR_SIZE 38 //total spots
#define SPLIT_SIZE 2 //number of spots in a split
#define ST_SIZE 3 //number of spots in a street
#define BASKET_SIZE 3 //number of spots in a basket
#define CORNER_SIZE 4 //number of spots in a corner
#define DUB_ST_SIZE 6 //number of spots in a double street
#define DOZ_COL_SIZE 12 //number of spots in a dozen/column

//structure to represent spot
typedef struct {
    char color[6]; //spot color
    char number[3]; //spot number
} Spot;

extern const Spot wheel_arr[ARR_SIZE];
extern const Spot base_table_arr[ARR_SIZE];
extern const char *split_bets[][SPLIT_SIZE];
extern const char *street_bets[][ST_SIZE];
extern const char *basket_bets[][BASKET_SIZE];
extern const char *corner_bets[][CORNER_SIZE];
extern const char *top_line[];
extern const char *double_street_bets[][DUB_ST_SIZE];
extern const char *dozen_bets[][DOZ_COL_SIZE];
extern const char *column_bets[][DOZ_COL_SIZE];
extern const char *red[];
extern const char *black[];
extern const char *odds[];
extern const char *evens[];
extern const char *low_half[];
extern const char *high_half[];

#endif
