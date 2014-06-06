/*
 * MB hardware functions
 *
 *  Created on: 5 лист. 2013
 *      Author: drvlas
 */
#include "MB_header.h"

#define EUSCI_A_UART_CLOCKSOURCE_SMCLK					UCSSEL__SMCLK		/* These defines are fetched from MSP430 driverlib headers	*/
#define EUSCI_A_UART_DEGLITCH_TIME_200ns				(UCGLIT0 + UCGLIT1)
#define EUSCI_A_UART_LSB_FIRST							0x00
#define EUSCI_A_UART_MODE								UCMODE_0
#define EUSCI_A_UART_ONE_STOP_BIT						0x00
#define EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION	0x01
#define EUSCI_A_UART_RECEIVE_INTERRUPT					UCRXIE
#define EUSCI_A_UART_TRANSMIT_INTERRUPT					UCTXIE
#define EUSCI_A_UART_RECEIVE_ERRONEOUSCHAR_INTERRUPT	UCRXEIE
#define EUSCI_A_UART_BREAKCHAR_INTERRUPT				UCBRKIE
#define EUSCI_A_UART_STARTBIT_INTERRUPT					UCSTTIE
#define EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT		UCTXCPTIE
#define TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE				CCIE
#define TIMER_A_CLOCKSOURCE_DIVIDER_1					0x01
#define TIMER_A_CLOCKSOURCE_SMCLK						TASSEL__SMCLK
#define TIMER_A_UP_MODE									MC_1

#define	FSMCLK_HZ	(7372800UL/2)				/* Look FQUARZ_HZ in main application */

const
uint16_t NBR[] = {	FSMCLK_HZ/4800/16,			// 48 to write in UCABRW register
					FSMCLK_HZ/9600/16,			// 24
					FSMCLK_HZ/19200/16,			// 12
					FSMCLK_HZ/38400/16};		// 6. BTW, for 115K it will be 2
const
uint16_t Nt35[] = { FSMCLK_HZ*38.5/4800-1,		// t3.5 is 11 bits * 3.5 = 38.5 clocks
					FSMCLK_HZ*38.5/9600-1,		// 14783: 4010 us
					FSMCLK_HZ*38.5/19200-1,		// 7391: 2005 us
					6450};						// 1750 us
const
uint16_t Nt25[] = { FSMCLK_HZ*27.5/4800-1,		// t2.5 is 11 bits * 2.5
					FSMCLK_HZ*27.5/9600-1,
					FSMCLK_HZ*27.5/19200-1,
					FSMCLK_HZ*11/38400+2765};	// 750 us + 1 symbol @ 38400 baud


uint16_t
MBHWinit( uint16_t baudrate_code) {

	/*
	 * Timer А1
	 * Used to catch events t1.5 and t3.5
	 * Source: SMCLK, divided by 1
	 * UP mode, interrupt at CCR0 compare
	 */

   	TA1CTL = ((TIMER_A_CLOCKSOURCE_DIVIDER_1 - 1) << 6);
   	TA1EX0 = TAIDEX_0;								// Усе це разом визначило дільник = 1
   	TA1CTL |= TIMER_A_CLOCKSOURCE_SMCLK;
   	TA1CCTL0 |= TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;	// CCIE bit = 1
   	TA1CTL |= TIMER_A_UP_MODE;
   	TA1CCR0  = 0;									// Look datasheet, 11.2.2 Starting the Timer: it's STOP

   	/*
   	 * eUSCI as UART for MODBUS
   	 * Clock: SMCLK, divided by 1. It equals to 3686400 Hz in my project
   	 * The baudrate code sets one of 4 values: 4800, 9600, 19200, 38400
   	 * These values may be changed - look NBR[] table
   	 * All depended constants are precompiled values from tables
   	 * The recommended eUSCI_A initialization/reconfiguration process is:
   	 * 1. Set UCSWRST (BIS.B #UCSWRST,&UCAxCTL1).
   	 * 2. Initialize all eUSCI_A registers with UCSWRST = 1 (including UCAxCTL1).
   	 * 3. Configure ports.
   	 * 4. Clear UCSWRST via software (BIC.B #UCSWRST,&UCAxCTL1).
   	 * Enable interrupts (optional) via UCRXIE or UCTXIE.
   	 */

   	UCA0CTLW0 |= UCSWRST;					// Disable the USCI during configuration
   	UCA0CTLW0 = (
   		UCPEN							|	// Enable Parity
   		UCPAR							|	// Even Parity
   		EUSCI_A_UART_LSB_FIRST			|	// UCMSB = 0, default
   		EUSCI_A_UART_MODE				|	// UCSYNC = UC7BIT = UCMODE = 0
   		EUSCI_A_UART_ONE_STOP_BIT 		|	// UCSPB = 0, default
   		EUSCI_A_UART_CLOCKSOURCE_SMCLK		// UCSSEL0 = 2
   			);								// UCRXEIE = UCBRKIE = 0, ignoring bad bytes received
   											// UCDORM = 0, resetDormant,
   											// UCTXADDR = UCTXBRK = 0, no special modes
   	UCA0BRW   = NBR[baudrate_code];			// З обраним кварцем тут ціле число
   	UCA0MCTLW = ((0 << 8) + (0 << 4) +		// No modulation with 7372800 Hz quartz
   			EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION );
	UCA0CTLW1 = EUSCI_A_UART_DEGLITCH_TIME_200ns;

	P2SEL0 &= ~(1<<0);						// P2.0 is secondary OUT function: TxD
	P2SEL1 |=  (1<<0);
	P2DIR  |=  (1<<0);

	P2SEL0 &= ~(1<<1);						// P2.1 is secondary INP function: RxD
	P2SEL1 |=  (1<<1);
	P1DIR  &= ~(1<<1);

	P2SEL0 &= ~(1<<2);						// P2.2 is RTS signal: "1" to transmit
	P2SEL1 &= ~(1<<2);
	P2DIR  |=  (1<<2);

	UCA0CTLW0 &= ~UCSWRST;					// Enable the USCI after configuration

	UCA0IE = EUSCI_A_UART_TRANSMIT_INTERRUPT |
			 EUSCI_A_UART_RECEIVE_INTERRUPT	 |
			 EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT;

	TA1CCR0 = Nt35[baudrate_code];			// Start Timer t35

	return 	Nt25[baudrate_code];			// Timer value @ t2.5 after it's reset
}

