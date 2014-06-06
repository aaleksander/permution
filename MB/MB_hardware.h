/*
 * MB_hardware.h
 *
 *  Created on: 4 груд. 2013
 *      Author: drvlas
 */

#ifndef MB_HARDWARE_H_
#define MB_HARDWARE_H_

#include "stdint.h"		/* Only int8_t, uint8_t, int16_t, uint16_t are needed	*/

/*
 * Interrupt vector of my MCU (MSP430), extracted from driverlib
 */
#define	RX_INT			2
#define	TX_INT			4
#define	FIRSTBIT_INT	6
#define	LASTBIT_INT		8

/*
 * Some sort of MACROs - to make functions' texts "hardware independent"
 */
#define	TO_TRANSMT	do { P2OUT  |=  (1<<2); } 	while(0)
#define	TO_RECEIVE	do { P2OUT  &= ~(1<<2); } 	while(0)
#define	STOP_TIMER	do { TA1CTL &= ~MC_1; } 	while(0)
#define	RESET_TIMER	do { TA1CTL |= TACLR; \
						 TA1CTL |= MC_1; }		while(0)
#define	TIMER_COUNT		TA1R
#define	BYTE_RECEIVED	UCA0RXBUF
#define	BYTE_TO_SEND	UCA0TXBUF

/*
 * If you don't intend to output MBState at MCU pins - use only "MBState = s"
 */
#define SET_ST(s)	do {										\
		if( ShowState) {										\
			if(1 & s) PJOUT |= (1<<0); else PJOUT &= ~(1<<0);	\
			if(2 & s) PJOUT |= (1<<1); else PJOUT &= ~(1<<1);	\
			if(4 & s) PJOUT |= (1<<2); else PJOUT &= ~(1<<2);	\
		}														\
		MBState = s; }	while(0)

#endif /* MB_HARDWARE_H_ */
