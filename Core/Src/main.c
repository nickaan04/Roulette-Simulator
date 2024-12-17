#include "main.h"
#include "usart.h"
#include <stdbool.h>
#include <stdlib.h>

void SystemClock_Config(void);
void handle_single_array_bet(const char **, uint8_t);
void handle_double_array_bet(char *, const char **, uint8_t, uint8_t);

#define SINGLE_ARR_SIZE 18
#define NUM_SPLITS 61 //number of possible split bets
#define NUM_STREETS 12 //number of possible street bets
#define NUM_BASKETS 3 //number of possible basket bets
#define NUM_CORNERS 22 //number of possible corner bets
#define TOP_LINE_SIZE 5
#define NUM_DUB_ST 11 //number of possible double street bets
#define NUM_DOZ_COL 3 //number of possible dozen/column bets
#define DEL 2000 //1 second in milliseconds

//game states
typedef enum {
    INIT_ST,
	TRADE_ST,
    BET_TYPE_ST,
	TABLE_UPDATE_ST,
	BET_MONEY_ST,
    SPIN_ST,
    RESULT_ST,
    END_ST
} GameState;

volatile GameState current_state = INIT_ST; //current state of the game
volatile char usart_input_buffer[20]; //buffer for USART input
volatile uint8_t usart_input_index = 0; //index for USART buffer
volatile bool input_ready = false; //flag to indicate input is complete

//player data
Chips player_chips = {
    .yellow = 0, //$1000 chips
    .purple = 1, //$500 chips
    .black = 5, //$100 chips
    .orange = 10, //$50 chips
    .green = 12, //$25 chips
    .blue = 10, //$10 chips
    .red = 16, //$5 chips
    .white = 20 //$1 chips
};

volatile uint32_t bet_amount = 0; //current bet amount
volatile char bet_type[20]; //type of bet

//game data
volatile uint32_t winning_index = 0; //winning number index
volatile char winning_numbers[ARR_SIZE][3]; //buffer to store winning spots
volatile uint8_t winning_numbers_count = 0; //count of winning numbers in the buffer
volatile uint8_t spin_iterations = 0; //number of completed wheel iterations
volatile uint8_t spin_index = 0; //current wheel index during spinning
volatile bool spin_complete = false; //flag to indicate the spin is done

int main(void) {
	HAL_Init();
	SystemClock_Config();
	//initialize LEDs, USART, RNG, TIM2, and print/populate start screen
	LED_init();
	USART_init();
	RNG_init();
	TIM2_init();
	USART_start_screen();
	USART_print_wheel(wheel_arr, 0);
	USART_print_table(base_table_arr);
	USART_print_chips(&player_chips, bet_amount);
	__enable_irq(); //enable interrupts globally

	while (1) { //infinte program flow
		switch (current_state) { //state machine
			case INIT_ST: //start point of game
				//print start message
				USART_ESC_Code(TOP_LEFT);
				USART_ESC_Code(DOWN_35);
				USART_ESC_Code(CLEAR_LINE);
				USART_print_string("Welcome to Roulette! Press Enter to begin.");

				while (!input_ready); //wait for user input
				input_ready = false; //reset input flag

				USART_ESC_Code(CLEAR_LINE);
				USART_ESC_Code(FULLY_LEFT);
				USART_print_string("Starting game...");
				HAL_Delay(DEL); //2 second delay

				current_state = TRADE_ST; //transition to trading state
				break;

			case TRADE_ST: //handle chip trade in logic
				while(1) {
					//ask user if they want to trade in chips
					USART_ESC_Code(CLEAR_LINE);
					USART_ESC_Code(FULLY_LEFT);
					USART_print_string("Trade in chips? (yes/no) --> ");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

					//transition to betting type state if the answer is not "yes"
					if (strcmp(usart_input_buffer, "yes") != 0) {
						current_state = BET_TYPE_ST; //transition to betting type state
						break;
					}
			        //ask the user for the type of chip to trade in
			        uint32_t chip_value_in = 0;
			        uint32_t chip_value_out = 0;
			        uint32_t chip_quantity_in = 0;
			        uint32_t possible_out = 0;
			        uint32_t *chip_ptr_in = NULL;
			        uint32_t *chip_ptr_out = NULL;

			        //prompt for the chip to trade in
			        while (1) {
			        	USART_ESC_Code(CLEAR_LINE);
			            USART_ESC_Code(FULLY_LEFT);
			            USART_print_string("Enter chip value to trade in --> ");

						while (!input_ready); //wait for user input
						input_ready = false; //reset input flag

						//validate and record chip value
			            chip_value_in = atoi(usart_input_buffer);
			            //disallow trading in white chips
			            if (chip_value_in == WHITE_VAL) {
		                    USART_ESC_Code(CLEAR_LINE);
		                    USART_ESC_Code(FULLY_LEFT);
		                    USART_print_string("Cannot trade in $1 chips! Please enter a higher chip value.");
		                    HAL_Delay(DEL); //2 second delay
		                    continue; //retry for valid input
			            }
			            //assign pointer to corresponding chip value in player chips
			            chip_ptr_in = get_chip_pointer(chip_value_in, &player_chips);
			            //handle an invalid input
			            if (chip_ptr_in == NULL) {
		                    USART_ESC_Code(CLEAR_LINE);
		                    USART_ESC_Code(FULLY_LEFT);
		                    USART_print_string("Invalid chip value! Please enter a valid chip value.");
		                    HAL_Delay(DEL); //2 second delay
		                    continue; //retry for valid input
			            } else if (*chip_ptr_in == 0) { //check if out of those chips
		                    USART_ESC_Code(CLEAR_LINE);
		                    USART_ESC_Code(FULLY_LEFT);
							USART_print_string("You are out of ");
							snprintf(usart_input_buffer, sizeof(usart_input_buffer), "$%lu", chip_value_in);
							USART_print_string(usart_input_buffer);
							USART_print_string(" chips! Please enter a different chip value.");
		                    HAL_Delay(DEL); //2 second delay
		                    continue; //retry for valid input
			            }
			            break;
			        }

			        //ask the user how many of these chips to trade in
					while (1) {
						USART_ESC_Code(CLEAR_LINE);
						USART_ESC_Code(FULLY_LEFT);
						USART_print_string("Enter quantity of ");
						snprintf(usart_input_buffer, sizeof(usart_input_buffer), "$%lu", chip_value_in);
						USART_print_string(usart_input_buffer);
						USART_print_string(" chips to trade in --> ");

						while (!input_ready); //wait for user input
						input_ready = false; //reset input flag

						//validate chip quantity
						chip_quantity_in = atoi(usart_input_buffer);
						if (chip_quantity_in > *chip_ptr_in) {
							USART_ESC_Code(CLEAR_LINE);
							USART_ESC_Code(FULLY_LEFT);
							USART_print_string("Not enough chips to trade in! Please enter a lower quantity.");
							HAL_Delay(DEL); //2 second delay
							continue;
						}
						break;
					}

			        //ask the user for the chip value they want in return
			        while (1) {
			            USART_ESC_Code(CLEAR_LINE);
			            USART_ESC_Code(FULLY_LEFT);
			            USART_print_string("Enter chip value you want in return --> ");

						while (!input_ready); //wait for user input
						input_ready = false; //reset input flag

						//validate and record chip value
			            chip_value_out = atoi(usart_input_buffer);
			            //validate that the chip value out is lower than chip value in
			            if (chip_value_out >= chip_value_in) {
			                USART_ESC_Code(CLEAR_LINE);
			                USART_ESC_Code(FULLY_LEFT);
			                USART_print_string("Chip value must be lower than trade-in chip value! Try again.");
			                HAL_Delay(DEL); //2 second delay
			                continue;
			            }
			            //assign pointer to corresponding chip value being traded in for
			            chip_ptr_out = get_chip_pointer(chip_value_out, &player_chips);
			            //handle invalid input
			            if (chip_ptr_out == NULL) {
		                    USART_ESC_Code(CLEAR_LINE);
		                    USART_ESC_Code(FULLY_LEFT);
		                    USART_print_string("Invalid chip value! Please enter a valid chip value.");
		                    HAL_Delay(DEL); //2 second delay
		                    continue; //retry for valid input
			            }
			            //validate that the lower value fits evenly into the higher value
			            if (((chip_value_in * chip_quantity_in) % chip_value_out) != 0) {
			                USART_ESC_Code(CLEAR_LINE);
			                USART_ESC_Code(FULLY_LEFT);
			                USART_print_string("Trade-in value must be divisible by the desired value! Try again.");
			                HAL_Delay(DEL); //2 second delay
			                continue;
			            }
			            break;
			        }
			        //calculate the possible number of lower chips
			        possible_out = (chip_quantity_in * chip_value_in) / chip_value_out;
			        //perform the transaction
			        *chip_ptr_in -= chip_quantity_in;
			        *chip_ptr_out += possible_out;

			        //update the display
			        USART_print_chips(&player_chips, bet_amount);
			        //notify the user of the transaction
					USART_ESC_Code(TOP_LEFT);
					USART_ESC_Code(DOWN_35);
			        USART_ESC_Code(CLEAR_LINE);
			        USART_ESC_Code(FULLY_LEFT);
			        USART_print_string("Trade in complete! You traded ");
			        snprintf(usart_input_buffer, sizeof(usart_input_buffer), "%lu", chip_quantity_in);
			        USART_print_string(usart_input_buffer);
			        USART_print_string(" $");
			        snprintf(usart_input_buffer, sizeof(usart_input_buffer), "%lu", chip_value_in);
			        USART_print_string(usart_input_buffer);
			        USART_print_string(" chips for ");
			        snprintf(usart_input_buffer, sizeof(usart_input_buffer), "%lu", possible_out);
			        USART_print_string(usart_input_buffer);
			        USART_print_string(" $");
			        snprintf(usart_input_buffer, sizeof(usart_input_buffer), "%lu", chip_value_out);
			        USART_print_string(usart_input_buffer);
			        USART_print_string(" chips.");
			        HAL_Delay(2 * DEL); //4 second delay
			    }

			case BET_TYPE_ST: //determine the type of bet the user wants
				//ask for the type of bet
				USART_ESC_Code(CLEAR_LINE);
				USART_ESC_Code(FULLY_LEFT);
				USART_print_string("Choose your bet type (from above) --> ");

				while (!input_ready); //wait for user input
				input_ready = false; //reset input flag

				//store the chosen bet type in a global variable for later use
				strncpy(bet_type, usart_input_buffer, sizeof(bet_type) - 1);
				bet_type[sizeof(bet_type) - 1] = '\0'; //ensure null termination

				if (strcmp(bet_type, "Straight") == 0) { //straight bet
					//ask for the number
					USART_ESC_Code(CLEAR_LINE);
					USART_ESC_Code(FULLY_LEFT);
					USART_print_string("Enter number (00-36) --> ");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

					//validate the number and update the winning numbers array
					if (strcmp(usart_input_buffer, "00") == 0 ||
					   (atoi(usart_input_buffer) >= 0 && atoi(usart_input_buffer) <= 36)) {
						winning_numbers_count = 0;
						strncpy(winning_numbers[winning_numbers_count++], usart_input_buffer, 3);

						current_state = TABLE_UPDATE_ST; //move to betting amount state
					} else {
						//invalid number
						USART_ESC_Code(CLEAR_LINE);
						USART_ESC_Code(FULLY_LEFT);
						USART_print_string("Invalid bet! Number you entered does not exist on the wheel.");
						HAL_Delay(DEL); //2 second delay

						current_state = BET_TYPE_ST; //remain in betting type state
					}
				} else if (strcmp(bet_type, "Split") == 0) { //split bet
					handle_double_array_bet("split", split_bets, NUM_SPLITS, SPLIT_SIZE);
				} else if (strcmp(bet_type, "Street") == 0) { //street bet
					handle_double_array_bet("street", street_bets, NUM_STREETS, ST_SIZE);
				} else if (strcmp(bet_type, "Basket") == 0) { //basket bet
					handle_double_array_bet("basket", basket_bets, NUM_BASKETS, BASKET_SIZE);
				} else if (strcmp(bet_type, "Corner") == 0) { //corner bet
					handle_double_array_bet("corner", corner_bets, NUM_CORNERS, CORNER_SIZE);
				} else if (strcmp(bet_type, "Top Line") == 0) { //top line bet
					handle_single_array_bet(top_line, TOP_LINE_SIZE);
				} else if (strcmp(bet_type, "Double Street") == 0) { //double street bet
					handle_double_array_bet("double street", double_street_bets, NUM_DUB_ST, DUB_ST_SIZE);
				} else if (strcmp(bet_type, "Dozen") == 0) { //dozen bet
					handle_double_array_bet("dozen", dozen_bets, NUM_DOZ_COL, DOZ_COL_SIZE);
				} else if (strcmp(bet_type, "Column") == 0) { //column bet
					handle_double_array_bet("column", column_bets, NUM_DOZ_COL, DOZ_COL_SIZE);
				} else if (strcmp(bet_type, "Red") == 0 || strcmp(bet_type, "Black") == 0) { //color bet
					const char **selected_color = (strcmp(bet_type, "Red") == 0)
													? red
													: black;
					handle_single_array_bet(selected_color, SINGLE_ARR_SIZE);
				} else if (strcmp(bet_type, "Odd") == 0 || strcmp(bet_type, "Even") == 0) { //odd/even bet
				  const char **selected_parity = (strcmp(bet_type, "Odd") == 0)
													? odds
													: evens;
				  handle_single_array_bet(selected_parity, SINGLE_ARR_SIZE);
				} else if (strcmp(bet_type, "Low") == 0 || strcmp(bet_type, "High") == 0) { //high/low bet
				  const char **selected_range = (strcmp(bet_type, "Low") == 0)
													? low_half
													: high_half;
				  handle_single_array_bet(selected_range, SINGLE_ARR_SIZE);
				} else {
					//invalid bet type
					USART_ESC_Code(CLEAR_LINE);
					USART_ESC_Code(FULLY_LEFT);
					USART_print_string("Invalid bet type! Choose one from the list above.");
					HAL_Delay(DEL); //2 second delay

					current_state = BET_TYPE_ST; //remain in betting type state
				}
				break;

			case TABLE_UPDATE_ST:
				//create a dynamic array to copy the base table spots
				Spot dynamic_table[ARR_SIZE];
				memcpy(dynamic_table, base_table_arr, sizeof(base_table_arr));

				//highlight the winning spots in the dynamic array
				for (uint8_t i = 0; i < winning_numbers_count; i++) {
				  for (uint8_t j = 0; j < ARR_SIZE; j++) {
					  if (strcmp(winning_numbers[i], dynamic_table[j].number + (dynamic_table[j].number[0] == ' ' ? 1 : 0)) == 0) {
						  strcpy(dynamic_table[j].color, "cyan"); //highlight winning spot
						  }
				  }
				}
				//print the updated table
				USART_print_table(dynamic_table);

				current_state = BET_MONEY_ST; //transition to betting money state
				break;

			case BET_MONEY_ST:
				//variables to track bet info
				uint32_t chip_value = 0;
				uint32_t chip_quantity = 0;
				uint32_t total_bet = 0;

				//run continuously while in this state
				while (1) {
					//ask for chip value or "done"
					USART_ESC_Code(TOP_LEFT);
					USART_ESC_Code(DOWN_35);
					USART_ESC_Code(CLEAR_LINE);
					USART_print_string("Enter chip value to bet or 'done' --> ");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

					//check if the user is done betting
					if (strcmp(usart_input_buffer, "done") == 0) {
						if (total_bet == 0) {
		                    USART_ESC_Code(CLEAR_LINE);
		                    USART_ESC_Code(FULLY_LEFT);
		                    USART_print_string("You must bet before spinning the wheel!");
		                    HAL_Delay(DEL); //2 second delay
		                    continue; //retry for valid input
						}
						break;
					}

					//validate and record chip value
					chip_value = atoi(usart_input_buffer);
					uint32_t *chip_ptr = NULL;

					chip_ptr = get_chip_pointer(chip_value, &player_chips);

		            if (chip_ptr == NULL) {
	                    USART_ESC_Code(CLEAR_LINE);
	                    USART_ESC_Code(FULLY_LEFT);
	                    USART_print_string("Invalid chip value! Please enter a valid chip value.");
	                    HAL_Delay(DEL); //2 second delay
	                    continue; //retry for valid input
		            }

					//ask for the quantity of the selected chip
					USART_ESC_Code(CLEAR_LINE);
					USART_ESC_Code(FULLY_LEFT);
					USART_print_string("Enter quantity of chips --> ");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

					//validate chip quantity
					chip_quantity = atoi(usart_input_buffer);
					if (chip_quantity > *chip_ptr) {
						//invalid chip quantity
						USART_ESC_Code(CLEAR_LINE);
						USART_ESC_Code(FULLY_LEFT);
						USART_print_string("Not enough chips! Please enter a lower quantity or different value.");
						HAL_Delay(DEL); //2 second delay
						continue;
					}
					//calculate the total bet and update chip counts
					total_bet += chip_value * chip_quantity;
					*chip_ptr -= chip_quantity;
					//update chip and balance display
					USART_print_chips(&player_chips, total_bet);
				}
				//update global bet amount
				bet_amount = total_bet;

				current_state = SPIN_ST; //transition to spin state
				break;

			case SPIN_ST:
				//prompt user to press enter to spin the wheel
				USART_ESC_Code(CLEAR_LINE);
				USART_ESC_Code(FULLY_LEFT);
				USART_print_string("Press Enter to spin the wheel...");

				while (!input_ready); //wait for user input
				input_ready = false; //reset input flag

				//message while spinning
				USART_ESC_Code(CLEAR_LINE);
				USART_ESC_Code(FULLY_LEFT);
				USART_print_string("Spinning...");
				//get winning index using RNG
				winning_index = RNG_get_random_number() % ARR_SIZE;
				//reset spinning variables
				spin_iterations = 0;
				spin_index = 0;
				spin_complete = false;

				GPIOC->ODR |= YELLOW_PIN; //turn on yellow LED to alternate with blue
				//enable the timer to start spinning
				TIM2->CR1 |= TIM_CR1_CEN; //start timer
				TIM2->DIER |= TIM_DIER_UIE; //enable update interrupt
				TIM2->SR &= ~TIM_SR_UIF; //clear update flag

				//wait for spin to complete
				while (!spin_complete);

				GPIOC->ODR &= ~(YELLOW_PIN | BLUE_PIN); //turn off LEDs
				//disable the timer to stop spinning
				TIM2->CR1 &= ~TIM_CR1_CEN; //stop timer
				TIM2->DIER &= ~TIM_DIER_UIE; //disable update interrupt
				TIM2->SR &= ~TIM_SR_UIF; //clear update flag

				current_state = RESULT_ST; //transition to result state
				break;

			case RESULT_ST:
				//reset the table to unhighlighted values
				Spot unhighlighted_table[ARR_SIZE];
				memcpy(unhighlighted_table, base_table_arr, sizeof(base_table_arr));
				USART_print_table(unhighlighted_table);
				//retrieve the winning spot
				Spot winning_spot = wheel_arr[winning_index];
				//check if the winning spot is in the winning numbers array
				bool user_won = false;
				for (uint8_t i = 0; i < winning_numbers_count; i++) {
				  if (strcmp(winning_numbers[i], winning_spot.number + (winning_spot.number[0] == ' ' ? 1 : 0)) == 0) {
					  user_won = true;
					  break;
				  }
				}
				//calculate winnings or losses
				uint32_t net_gain = 0; //net gain or loss
				if (user_won) {
					//calculate winnings based on bet type odds
					uint32_t winnings = bet_amount * calculate_odds(bet_type);
					net_gain = winnings; //net gain
					distribute_chips(winnings, &player_chips); //distribute winnings back as chips
				} else {
					net_gain = bet_amount; //net loss
				}
				//prepare result message
				char result_message[25];
				if (user_won) {
				  snprintf(result_message, sizeof(result_message), "You won $%ld! ", (net_gain - bet_amount));
				} else {
				  snprintf(result_message, sizeof(result_message), "You lost $%ld. ", net_gain);
				}
				//reset bet amount
				bet_amount = 0;
				//update the chips and balance display
				USART_print_chips(&player_chips, bet_amount);
				//navigate to message section
				USART_ESC_Code(TOP_LEFT);
				USART_ESC_Code(DOWN_35);
				USART_ESC_Code(CLEAR_LINE);
				USART_ESC_Code(FULLY_LEFT);
				USART_ESC_Code(RESET_ATTRIBUTES);
				//inform the user of the result
				if (user_won) {
					USART_print_string("Congratulations! ");
					GPIOC->ODR |= LED_PINS;
				} else {
					USART_print_string("Better luck next time! ");
					GPIOB->ODR |= LED_PINS;
				}
				USART_print_string(result_message);
				HAL_Delay(2.5 * DEL); //5 second delay

				current_state = END_ST; //transition to end state

			case END_ST:
			    //reset bet-related variables
			    memset(bet_type, 0, sizeof(bet_type));     // clear the bet type
			    memset(winning_numbers, 0, sizeof(winning_numbers)); // clear the winning numbers array
			    winning_numbers_count = 0;                //reset winning numbers count
			    //check if the player is out of chips
			    if (calculate_total_balance(player_chips) == 0) {
			        //inform the user that they are out of chips
			        USART_ESC_Code(TOP_LEFT);
			        USART_ESC_Code(DOWN_35);
			        USART_ESC_Code(CLEAR_LINE);
			        USART_ESC_Code(FULLY_LEFT);
			        USART_print_string("You are out of chips! Type 'reset' to start over --> ");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

					//if user want to reset
			        if (strcmp(usart_input_buffer, "reset") == 0) {
			            //reset the player's chips to the initial state
			            player_chips = (Chips) {
			                .yellow = 0, //$1000 chips
			                .purple = 1, //$500 chips
			                .black = 5, //$100 chips
			                .orange = 10, //$50 chips
			                .green = 12, //$25 chips
			                .blue = 10, //$10 chips
			                .red = 16, //$5 chips
			                .white = 20 //$1 chips
			            };
			            //update the chips and balance display
			            USART_print_chips(&player_chips, bet_amount);

			            current_state = INIT_ST; //transition back to initial state
			        } else {
			            //invalid input
			            USART_ESC_Code(CLEAR_LINE);
			            USART_ESC_Code(FULLY_LEFT);
			            USART_print_string("Invalid input! Type 'reset' to start over.");

			            current_state = END_ST; //remain in end state
			        }
			    } else {
			        //inform the user that they can play again
			        USART_print_string("Press Enter to play again!");

					while (!input_ready); //wait for user input
					input_ready = false; //reset input flag

			        current_state = TRADE_ST; //transition to trade state
			    }
				//turn off LEDs
				GPIOB->ODR &= ~LED_PINS;
				GPIOC->ODR &= ~LED_PINS;
			    break;
		}
	}
}

//ISR for USART2
void USART2_IRQHandler(void) {
	//check if RXNE flag is set
    if (USART2->ISR & USART_ISR_RXNE) {
        char c = USART2->RDR; //read received character
        if (c == '\b' || c == 127) { //handle backspace
        	//move cursor back, print a space to 'erase', move back again
        	if (usart_input_index > 0) {
        		USART_ESC_Code(LEFT_1);
                USART_print_char(' ');
                USART_ESC_Code(LEFT_1);
                usart_input_index--; //remove last character from buffer
        	}
        } else if (c == '\n' || c == '\r') { //handle 'enter'
            //end of input
            usart_input_buffer[usart_input_index] = '\0'; //null-terminate string
            usart_input_index = 0; //reset buffer index
            input_ready = true;    //signal input is ready
        } else { //still typing
            //add character to buffer
            if (usart_input_index < sizeof(usart_input_buffer) - 1) {
                usart_input_buffer[usart_input_index++] = c;
                USART_print_char(c); //echo the character back
            }
        }
    }
}

//ISR for TIM2
void TIM2_IRQHandler(void) {
	//check if update flag is set
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; //clear update flag
        if (!spin_complete) {
            //simulate wheel spinning
            spin_index = (spin_index + 1) % ARR_SIZE;
            USART_print_wheel(wheel_arr, spin_index);
            //alternate yellow and blue LEDs at visible rate
            if (spin_index % 9 == 0) {
            	GPIOC->ODR ^= (YELLOW_PIN | BLUE_PIN);
            }
            //check if we completed the spin
            if (spin_iterations >= 5 && spin_index == winning_index) {
                spin_complete = true;
            }
            //increment iteration count if we completed a full spin
            if (spin_index == 0) {
                spin_iterations++;
            }
        }
    }
}

//handle bets that are stored in a single array
void handle_single_array_bet(const char **bet_array, uint8_t array_size) {
    //clear the winning numbers count
    winning_numbers_count = 0;
    //copy all numbers from the bet array to the winning_numbers array
    for (uint8_t i = 0; i < array_size; i++) {
        strncpy(winning_numbers[winning_numbers_count++], bet_array[i], 3);
    }

    current_state = TABLE_UPDATE_ST; //transition to table update state
}

//handle bets that are stored in a double array
void handle_double_array_bet(char *bet_name, const char **bet_array, uint8_t row_size, uint8_t col_size) {
    //ask the user for the index of the row in the double array
    USART_ESC_Code(CLEAR_LINE);
    USART_ESC_Code(FULLY_LEFT);
    USART_print_string("Enter ");
    USART_print_string(bet_name);
    USART_print_string(" number (refer to user manual or table) --> ");

	while (!input_ready); //wait for user input
	input_ready = false; //reset input flag

    //validate the input
    uint8_t index = atoi(usart_input_buffer) - 1; //convert to 0-based index
    if (index >= 0 && index < row_size) {
        //clear the winning numbers count
        winning_numbers_count = 0;
        //access the correct row
        const char **row = bet_array + (index * col_size);
        //copy all numbers from the selected row to the winning_numbers array
        for (uint8_t i = 0; i < col_size; i++) {
            strncpy(winning_numbers[winning_numbers_count++], row[i], 3);
        }

        current_state = TABLE_UPDATE_ST; //transition to table update state
    } else {
    	//invalid index
        USART_ESC_Code(CLEAR_LINE);
        USART_ESC_Code(FULLY_LEFT);
        USART_print_string("Invalid number! The number you entered does not exist on the table.");
        HAL_Delay(DEL); //2 second delay

        current_state = BET_TYPE_ST; //remain in the betting type state
    }
}

//80MHz MCU clock, 48MHz RNG clock
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}
void Error_Handler(void) {
  __disable_irq();
  while (1){}
}
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
