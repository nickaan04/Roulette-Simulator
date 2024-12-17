#ifndef SRC_USART_H_
#define SRC_USART_H_
#include "spots.h"
#include "misc.h"
#include <stdio.h>

#define DOWN_35 "[35B"
#define LEFT_1 "[1D"
#define LEFT_2 "[2D"
#define TOP_LEFT "[H"
#define CLEAR_LINE "[2K"
#define FULLY_LEFT "[1G"
#define RESET_ATTRIBUTES "[0m"

void USART_init(void);
void USART_print_char(char);
void USART_print_string(char*);
void USART_ESC_Code(char*);
void USART_reset_screen(void);
void USART_start_screen(void);
void USART_print_table(Spot*);
Spot USART_print_wheel(const Spot*, uint32_t);
void USART_print_chips(Chips*, uint32_t);

#endif
