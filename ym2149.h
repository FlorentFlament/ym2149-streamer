#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

void set_bus_ctl(void);

void send_data(char addr, char data);
char read_data(char addr);

void set_registers(uint8_t *regs, uint16_t mask);
