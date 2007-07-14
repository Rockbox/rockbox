#ifndef SERIAL_IMX31_H
#define SERIAL_IMX31_H

#include <stdarg.h>
#include <stdio.h>

int Tx_Rdy(void);
void Tx_Writec(const char c);
void dprintf(const char * str, ... );

#endif

