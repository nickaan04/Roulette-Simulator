#include "misc.h"

#define RNG_MULT 24 //clock configuration multiplier
#define ARR_VAL 2105263 //full wheel spin in 1 second

//configure TIM2
void TIM2_init(void){
	//turn on TIM2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	//set timer to count in up mode
	TIM2->CR1 &= ~TIM_CR1_DIR;
	//set ARR to 1s
	TIM2->ARR = ARR_VAL - 1;
	//enable TIM2 in NVIC
	NVIC->ISER[0] = (1 << TIM2_IRQn);
}

//configure LEDs to output, push-pull, very fast, no pull up/pull down resistor
void LED_init(void) {
	//turn on GPIOB and GPIOC clock
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN);
	//red LEDS
	GPIOB->MODER &= ~(GPIO_MODER_MODE5 | GPIO_MODER_MODE6 |
				   GPIO_MODER_MODE7 | GPIO_MODER_MODE8);
	GPIOB->MODER |= ((1 << GPIO_MODER_MODE5_Pos) | (1 << GPIO_MODER_MODE6_Pos) |
				  (1 << GPIO_MODER_MODE7_Pos) | (1 << GPIO_MODER_MODE8_Pos));
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 |
				    GPIO_OTYPER_OT7 | GPIO_OTYPER_OT8);
	GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 |
				    GPIO_OSPEEDR_OSPEED7 | GPIO_OSPEEDR_OSPEED8);
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 |
				   GPIO_PUPDR_PUPD7 | GPIO_PUPDR_PUPD8);
	//green LEDS (and 1 yellow, 1 blue)
	GPIOC->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3 |
					  GPIO_MODER_MODE5 | GPIO_MODER_MODE6 |
				      GPIO_MODER_MODE7 | GPIO_MODER_MODE8);
	GPIOC->MODER |= ((1 << GPIO_MODER_MODE2_Pos) | (1 << GPIO_MODER_MODE3_Pos) |
					(1 << GPIO_MODER_MODE5_Pos) | (1 << GPIO_MODER_MODE6_Pos) |
					(1 << GPIO_MODER_MODE7_Pos) | (1 << GPIO_MODER_MODE8_Pos));
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3 |
					GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6 |
				    GPIO_OTYPER_OT7 | GPIO_OTYPER_OT8);
	GPIOC->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3 |
					GPIO_OSPEEDR_OSPEED5 | GPIO_OSPEEDR_OSPEED6 |
				    GPIO_OSPEEDR_OSPEED7 | GPIO_OSPEEDR_OSPEED8);
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3 |
					GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6 |
				    GPIO_PUPDR_PUPD7 | GPIO_PUPDR_PUPD8);
	//setting all pins to start low (LEDs off)
	GPIOB->ODR &= ~LED_PINS;
	GPIOC->ODR &= ~(LED_PINS | YELLOW_PIN | BLUE_PIN);
}

//initialize the RNG with 48MHz clock
void RNG_init(void) {
	//enable PLLSAIQ with multiplier of 24
	RCC->CR &= ~RCC_CR_PLLSAI1ON;
	RCC->PLLSAI1CFGR &= ~RCC_PLLSAI1CFGR_PLLSAI1N_Msk;
	RCC->PLLSAI1CFGR |= (RNG_MULT << RCC_PLLSAI1CFGR_PLLSAI1N_Pos);
	RCC->PLLSAI1CFGR |= RCC_PLLSAI1CFGR_PLLSAI1QEN;
	RCC->CR |= RCC_CR_PLLSAI1ON;
	//configure RNG to use PLLSAI1Q
    RCC->CCIPR &= ~RCC_CCIPR_CLK48SEL;
    RCC->CCIPR |= RCC_CCIPR_CLK48SEL_0;
    //enable RNG clock
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    //enable RNG
    RNG->CR |= RNG_CR_RNGEN;
}

//generate a random number
uint32_t RNG_get_random_number(void) {
	//check for error bits
	if (RNG->SR & (RNG_SR_SEIS | RNG_SR_CEIS)) {
		//clear error bits
	    RNG->SR &= ~(RNG_SR_CEIS | RNG_SR_SEIS);
	}
    //wait for the data ready flag
    while (!(RNG->SR & RNG_SR_DRDY));
    //return the random number
    return RNG->DR;
}

//calculate total balance based on number of chips
uint32_t calculate_total_balance(Chips chips) {
    uint32_t total_balance = (chips.yellow * YELLOW_VAL) +
                             (chips.purple * PURPLE_VAL) +
                             (chips.black * BLACK_VAL) +
                             (chips.orange * ORANGE_VAL) +
                             (chips.green * GREEN_VAL) +
                             (chips.blue * BLUE_VAL) +
                             (chips.red * RED_VAL) +
                             (chips.white * WHITE_VAL);
    return total_balance;
}

//determine odds/payout based on bet type
uint32_t calculate_odds(const char *bet_type) {
    if (strcmp(bet_type, "Straight") == 0) return 36;
    if (strcmp(bet_type, "Split") == 0) return 18;
    if (strcmp(bet_type, "Street") == 0) return 12;
    if (strcmp(bet_type, "Basket") == 0) return 12;
    if (strcmp(bet_type, "Corner") == 0) return 9;
    if (strcmp(bet_type, "Top Line") == 0) return 7;
    if (strcmp(bet_type, "Double Street") == 0) return 6;
    if (strcmp(bet_type, "Dozen") == 0 || strcmp(bet_type, "Column") == 0) return 3;
    if (strcmp(bet_type, "Red") == 0 || strcmp(bet_type, "Black") == 0 ||
        strcmp(bet_type, "Odd") == 0 || strcmp(bet_type, "Even") == 0 ||
        strcmp(bet_type, "Low") == 0 || strcmp(bet_type, "High") == 0) return 2;
    return 0; //default odds
}

//distribute winnings into chips, starting with highest chip amount
void distribute_chips(uint32_t amount, Chips *chips) {
    uint32_t chip_values[] = {YELLOW_VAL, PURPLE_VAL, BLACK_VAL, ORANGE_VAL, GREEN_VAL, BLUE_VAL, RED_VAL, WHITE_VAL};
    uint32_t *chip_counts[] = {&chips->yellow, &chips->purple, &chips->black, &chips->orange, &chips->green, &chips->blue, &chips->red, &chips->white};
    //distribute through all chip values if possible
    for (uint8_t i = 0; i < POSSIBLE_CHIPS; i++) {
        while (amount >= chip_values[i]) {
            (*chip_counts[i])++;
            amount -= chip_values[i];
        }
    }
}

//validate and get chip pointer for a given chip value
uint32_t* get_chip_pointer(uint32_t chip_value, Chips *chips) {
    switch (chip_value) {
        case YELLOW_VAL: return &chips->yellow;
        case PURPLE_VAL: return &chips->purple;
        case BLACK_VAL: return &chips->black;
        case ORANGE_VAL: return &chips->orange;
        case GREEN_VAL: return &chips->green;
        case BLUE_VAL: return &chips->blue;
        case RED_VAL: return &chips->red;
        case WHITE_VAL: return &chips->white;
        default: return NULL;
    }
}
