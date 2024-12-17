#include "usart.h"

#define USART_PORT GPIOA //USART port
#define USART_AF 7 //USART alternating function
#define SYSTEM_CLK_FREQ 80000000 //80MHz MCU clock
#define BAUD_RATE 115200 //baud rate
#define NVIC_MASK 0x1F //mask bottom 5 bits
#define WHEEL_OUTLINE 4 //number of lines in wheel outline
#define TABLE_OUTLINE 16 //number of lines in table outline
#define BOTTOM_OUTLINE 10 //number of lines in bottom container outline

//escape codes
#define ESC "\x1B"
#define HIDE_CURSOR "[?25l"
#define UNDERLINE "[4m"
#define BOLD "[1m"
#define CLEAR_SCREEN "[2J"

#define RIGHT_1 "[1C"
#define RIGHT_2 "[2C"
#define RIGHT_3 "[3C"
#define RIGHT_5 "[5C"
#define RIGHT_6 "[6C"
#define RIGHT_14 "[14C"
#define RIGHT_31 "[31C"
#define RIGHT_50 "[50C"
#define RIGHT_60 "[60C"
#define RIGHT_63 "[63C"

#define LEFT_6 "[6D"
#define LEFT_8 "[8D"

#define DOWN_1 "[1B"
#define DOWN_2 "[2B"
#define DOWN_5 "[5B"
#define DOWN_12 "[12B"
#define DOWN_23 "[23B"
#define DOWN_32 "[32B"

#define UP_9 "[9A"

#define YELLOW "[33m"
#define PURPLE "[35m"
#define BLACK "[30m"
#define ORANGE "[38:5:202m"
#define GREEN "[32m"
#define BLUE "[34m"
#define RED "[31m"
#define CYAN "[96m"

const char *wheel_outline[] = { //outline for wheel
    "####",
    "	     -------------------|----|-------------------",
    "	         |    |    |    |    |    |    |    |    ",
    "	     -------------------|----|-------------------\n\n"
};

const char *table_outline[] = { //outline for table
    "   -------------------------------------------------------------",
    "   |            1 - 18           |           19 - 36           |",
    "---|-----------------------------|-----------------------------|------",
    "|  |    |    |    |    |    |    |    |    |    |    |    |    | 3rd |",
    "|  |----|----|----|----|----|----|----|----|----|----|----|----|-----|",
    "|--|    |    |    |    |    |    |    |    |    |    |    |    | 2nd |",
    "|  |----|----|----|----|----|----|----|----|----|----|----|----|-----|",
    "|  |    |    |    |    |    |    |    |    |    |    |    |    | 1st |",
    "---|-------------------|-------------------|-------------------|------",
    "   |       1 - 12      |      13 - 24      |      25 - 36      |",
    "   |-----------------------------------------------------------|",
    "   |     EVEN     |     RED      |    BLACK     |     ODD      |",
    "   -------------------------------------------------------------",
	"                                                        --------------",
	"                                                        |BET: $      |",
	"                                                        --------------"
};

const char *bottom_container_outline[] = { //outline for bottom container
    "---------------------------------------------    ---------------------",
    "| Straight: 35 to 1 | Double Street: 5 to 1 |    |     :   |   :     |",
    "| Split: 17 to 1    | Dozen: 2 to 1         |    |    :    |   :     |",
    "| Street: 11 to 1   | Column: 2 to 1        |    |    :    |  :      |",
    "| Basket: 11 to 1   | Red/Black: 1 to 1     |    |   :     |$1:      |",
    "| Corner: 8 to 1    | Odd/Even: 1 to 1      |    ---------------------",
    "| Top Line: 6 to 1  | Low/High: 1 to 1      |    |BALANCE: $         |",
    "---------------------------------------------    ---------------------",
    "----------------------------------------------------------------------\n",
    "----------------------------------------------------------------------"
};

//configure USART registers and pins
void USART_init(void) {
	//enable GPIOA clock
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	//configure GPIO pins
    //set PA2 and PA3 to AF, push-pull, very fast, no PUPD resistor
    USART_PORT->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    USART_PORT->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    USART_PORT->OTYPER &= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    USART_PORT->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
    USART_PORT->PUPDR &= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);
    //clear and set USART pins to alternate function 7 (USART2)
    USART_PORT->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    USART_PORT->AFR[0] |= (USART_AF << GPIO_AFRL_AFSEL2_Pos |
    					   USART_AF << GPIO_AFRL_AFSEL3_Pos);
	//enable USART2 clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
	//set word length to 1 start bit, 8 data bits, 1 stop bit, no parity bit
	USART2->CR1 &= ~(USART_CR1_M0 | USART_CR1_M1);
	USART2->CR2 &= ~USART_CR2_STOP;
	USART2->CR1 &= ~USART_CR1_PCE;
	//set baud rate (oversampling by 16)
	USART2->CR1 &= ~USART_CR1_OVER8;
	USART2->BRR = (uint32_t)(SYSTEM_CLK_FREQ/BAUD_RATE);
	//set TE bit to send an idle frame as first transmission
	//enable receiver and receiver interrupts
	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);
	NVIC->ISER[1] |= (1 << (USART2_IRQn & NVIC_MASK));
	//enable USART
	USART2->CR1 |= USART_CR1_UE;
}

//transmit character
void USART_print_char(char input) {
	//wait until TXE is set
	while (!(USART2->ISR & USART_ISR_TXE));
	//transmit character
	USART2->TDR = input;
}

//transmit a string of characters
void USART_print_string(char* input) {
	//continue to transmit characters until reaching end of string
    while (*input != '\0') {
    	USART_print_char(*input);
        input++;
    }
}

//print ESC character, then print desired ESC code
void USART_ESC_Code(char* code) {
	USART_print_string(ESC);
	USART_print_string(code);
}

//reset terminal screen
void USART_reset_screen(void) {
	USART_ESC_Code(CLEAR_SCREEN);
	USART_ESC_Code(TOP_LEFT);
	USART_ESC_Code(RESET_ATTRIBUTES);
	USART_ESC_Code(HIDE_CURSOR);
}

//print line (of table/wheel outline)
void USART_print_line(char* line) {
	USART_print_string(line);
	USART_ESC_Code(DOWN_1);
	USART_ESC_Code(FULLY_LEFT);
}

//print inputted section line by line
void USART_print_section(const char **lines, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        USART_print_line(lines[i]);
    }
}

//print start screen
void USART_start_screen(void) {
	//reset screen
	USART_reset_screen();
    //print title
    USART_ESC_Code(RIGHT_31);
    USART_ESC_Code(BOLD);
    USART_ESC_Code(UNDERLINE);
    USART_print_string("ROULETTE\n\n\n");
    USART_ESC_Code(RESET_ATTRIBUTES);
    USART_ESC_Code(LEFT_6);
    //print wheel outline
    USART_print_section(wheel_outline, WHEEL_OUTLINE);
    //print table outline
    USART_print_section(table_outline, TABLE_OUTLINE);
    //print bottom container outline
    USART_ESC_Code(BOLD);
    USART_print_string("             BETTING PAYOUTS                             CHIPS\n");
    USART_ESC_Code(FULLY_LEFT);
    USART_ESC_Code(RESET_ATTRIBUTES);
    USART_print_section(bottom_container_outline, BOTTOM_OUTLINE);
	//print colored chip values
	USART_ESC_Code(UP_9);
	USART_ESC_Code(FULLY_LEFT);
	USART_ESC_Code(RIGHT_50);
	USART_ESC_Code(YELLOW);
	USART_print_string("$1000");
	USART_ESC_Code(GREEN);
	USART_ESC_Code(RIGHT_5);
	USART_print_string("$25\n");

	USART_ESC_Code(FULLY_LEFT);
	USART_ESC_Code(RIGHT_50);
	USART_ESC_Code(PURPLE);
	USART_print_string("$500");
	USART_ESC_Code(BLUE);
	USART_ESC_Code(RIGHT_6);
	USART_print_string("$10\n");

	USART_ESC_Code(FULLY_LEFT);
	USART_ESC_Code(RIGHT_50);
	USART_ESC_Code(BLACK);
	USART_print_string("$100");
	USART_ESC_Code(RED);
	USART_ESC_Code(RIGHT_6);
	USART_print_string("$5\n");

	USART_ESC_Code(FULLY_LEFT);
	USART_ESC_Code(RIGHT_50);
	USART_ESC_Code(ORANGE);
	USART_print_string("$50");
	USART_ESC_Code(RESET_ATTRIBUTES);
}

//print given table spots in correct locations with appropriate colors
void USART_print_table(Spot *table_arr) {
	USART_ESC_Code(TOP_LEFT);
	USART_ESC_Code(DOWN_12);
	USART_ESC_Code(RIGHT_1);
	USART_ESC_Code(BOLD);
    //print "00"
    if (strcmp(table_arr[0].color, "cyan") == 0) {
        USART_ESC_Code(CYAN);
    } else {
        USART_ESC_Code(GREEN);
    }
	USART_print_string(table_arr[0].number);
	USART_ESC_Code(RIGHT_2);
    //print columns
    for (uint8_t i = 2; i <= ARR_SIZE - 1; i++) {
    	//choose color
        if (strcmp(table_arr[i].color, "red") == 0) {
            USART_ESC_Code(RED);
        } else if (strcmp(table_arr[i].color, "black") == 0) {
        	USART_ESC_Code(BLACK);
        } else if (strcmp(table_arr[i].color, "cyan") == 0) {
            USART_ESC_Code(CYAN);
        }
        USART_print_string(table_arr[i].number);
    	USART_ESC_Code(RIGHT_3);
    	//adjust at end of row
    	if (i == 13 || i == 25) {
    		USART_ESC_Code(FULLY_LEFT);
    		USART_ESC_Code(DOWN_2);
    		USART_ESC_Code(RIGHT_5);
    	}
    }
	USART_ESC_Code(FULLY_LEFT);
	USART_ESC_Code(RIGHT_1);
	//print "0"
    if (strcmp(table_arr[1].color, "cyan") == 0) {
        USART_ESC_Code(CYAN);
    } else {
        USART_ESC_Code(GREEN);
    }
	USART_print_string(table_arr[1].number);
	USART_ESC_Code(RESET_ATTRIBUTES);
}

//print wheel in wheel outline
Spot USART_print_wheel(const Spot *wheel_arr, uint32_t index) {
	//number of spots to display in the row
    const uint8_t display_count = 9;
    const uint8_t half_window = display_count / 2;
    //calculate the starting index for the circular array
    uint8_t start_index = (index - half_window + ARR_SIZE) % ARR_SIZE;
    //navigate to outline
    USART_ESC_Code(TOP_LEFT);
    USART_ESC_Code(DOWN_5);
    USART_ESC_Code(RIGHT_14);
    USART_ESC_Code(BOLD);
    //variable to hold middle "winning" spot
    Spot winning_spot = wheel_arr[(start_index + half_window) % ARR_SIZE];
    //print the 9 spots
    for (uint8_t i = 0; i < display_count; i++) {
        //compute the current index in the circular array
    	uint8_t current_index = (start_index + i) % ARR_SIZE;
        //set color for the spot
        if (strcmp(wheel_arr[current_index].color, "red") == 0) {
        	USART_ESC_Code(RED);
        } else if (strcmp(wheel_arr[current_index].color, "black") == 0) {
        	USART_ESC_Code(BLACK);
        } else if (strcmp(wheel_arr[current_index].color, "green") == 0) {
        	USART_ESC_Code(GREEN);
        }
        //print the number
        USART_print_string(wheel_arr[current_index].number);
        USART_ESC_Code(RIGHT_3);
    }
    return winning_spot; //return winning spot
}

//print inputted balance
void USART_print_balance(uint32_t balance) {
    char balance_str[12]; //character buffer
    //navigate to balance position
    USART_ESC_Code(TOP_LEFT);
    USART_ESC_Code(RESET_ATTRIBUTES);
    USART_ESC_Code(DOWN_32);
    USART_ESC_Code(RIGHT_60);
    //clear spots by overwriting with spaces
    for (uint8_t i = 0; i < 8; i++) {
        USART_print_char(' ');
    }
    //move cursor back to the start of the cleared area
    USART_ESC_Code(LEFT_8);
    //convert balance to string and print
    snprintf(balance_str, sizeof(balance_str), "%lu", balance);
    USART_print_string(balance_str);
}

//print inputted bet
void USART_print_bet(uint32_t bet) {
    char bet_str[12]; //character buffer
    //naviagte to bet position
    USART_ESC_Code(TOP_LEFT);
    USART_ESC_Code(RESET_ATTRIBUTES);
    USART_ESC_Code(DOWN_23);
    USART_ESC_Code(RIGHT_63);
    //clear spots by overwriting with spaces
    for (uint8_t i = 0; i < 6; i++) {
        USART_print_char(' ');
    }
    //move cursor back to the start of the cleared area
    USART_ESC_Code(LEFT_6);
    //convert bet to string and print
    snprintf(bet_str, sizeof(bet_str), "%lu", bet);
    USART_print_string(bet_str);
}

//print inputted chips
void USART_print_chips(Chips *chips, uint32_t bet) {
    char chip_count_str[4]; //buffer to store chip counts
    USART_ESC_Code(RESET_ATTRIBUTES);
    //clear the area and print chip values
    void USART_clear_and_print(uint8_t down, uint8_t right, uint32_t value) {
        //navigate to position
        USART_ESC_Code(TOP_LEFT);
        for (uint8_t i = 0; i < down; i++) {
            USART_ESC_Code(DOWN_1);
        }
        for (uint8_t i = 0; i < right; i++) {
            USART_ESC_Code(RIGHT_1);
        }
        //clear area (overwrite with spaces)
        for (uint8_t i = 0; i < 3; i++) {
            USART_print_char(' ');
        }
        //move back to the start of the cleared area
        for (uint8_t i = 0; i < 3; i++) {
            USART_ESC_Code(LEFT_1);
        }
        //print the value
        snprintf(chip_count_str, sizeof(chip_count_str), "%lu", value);
        USART_print_string(chip_count_str);
    }
    //print chips in corresponding spots
    USART_clear_and_print(27, 56, chips->yellow); //$1000 chips
    USART_clear_and_print(28, 55, chips->purple); //$500 chips
    USART_clear_and_print(29, 55, chips->black); //$100 chips
    USART_clear_and_print(30, 54, chips->orange); //$50 chips
    USART_clear_and_print(27, 64, chips->green); //$25 chips
    USART_clear_and_print(28, 64, chips->blue); //$10 chips
    USART_clear_and_print(29, 63, chips->red); //$5 chips
    USART_clear_and_print(30, 63, chips->white); //$1 chips
    //update total balance and bet
    uint32_t total_balance = calculate_total_balance(*chips);
    USART_print_bet(bet);
    USART_print_balance(total_balance);
}
