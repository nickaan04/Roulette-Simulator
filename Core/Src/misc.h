#ifndef SRC_MISC_H_
#define SRC_MISC_H_
#include "stm32l4xx_hal.h"
#include <string.h>

#define YELLOW_VAL 1000 //yellow chips are $1000
#define PURPLE_VAL 500 //purple chips are $500
#define BLACK_VAL 100 //black chips are $100
#define ORANGE_VAL 50 //orange chips are $50
#define GREEN_VAL 25 //green chips are $25
#define BLUE_VAL 10 //blue chips are $10
#define RED_VAL 5 //red chips are $5
#define WHITE_VAL 1 //white chips are $1
#define POSSIBLE_CHIPS 8 //number of different chips
#define LED_PINS (GPIO_ODR_OD5 | GPIO_ODR_OD6 | GPIO_ODR_OD7 | GPIO_ODR_OD8)
#define YELLOW_PIN GPIO_ODR_OD2
#define BLUE_PIN GPIO_ODR_OD3

typedef struct {
    uint32_t yellow; //number of $1000 chips
    uint32_t purple; //number of $500 chips
    uint32_t black; //number of $100 chips
    uint32_t orange; //number of $50 chips
    uint32_t green; //number of $25 chips
    uint32_t blue; //number of $10 chips
    uint32_t red; //number of $5 chips
    uint32_t white; //number of $1 chips
} Chips;

void TIM2_init(void);
void LED_init(void);
void RNG_init(void);
uint32_t RNG_get_random_number(void);
uint32_t calculate_total_balance(Chips);
uint32_t calculate_odds(const char *);
void distribute_chips(uint32_t, Chips *);
uint32_t *get_chip_pointer(uint32_t, Chips *);

#endif
