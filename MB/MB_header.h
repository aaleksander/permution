/*
 * MB header
 *
 *  Created on: 26 жовт. 2013
 *      Author: drvlas
 */

/* ******************************************************	Modbus Library using notes. drvlas, 2013

1. ******************************************************	Library Use Scope

- Slave
- RTU MODBUS only
- MSP430FR5726IPW as a target MCU. Other MCUs may be easily used instead, look Adjustment
- 03 and 16 Modbus functions only. User can add other functions


2. ******************************************************	Library Files & Functions

MB_header.h - common header, "platform-independent" part of includes and defines (except of #include <msp430.h>)

	- include part
	- configuration defines (look Adjustment)
	- Modbus function codes, exceptions, states, events, data structure

MB_parsing.c - main Modbus functions. They are almost "platform-independent"

	- MBinit() - initiates hardware and State Machine. Passes settings from main application to the Modbus stack
	- MBserve() -  constantly called from main application
	- TIMER1_A0_ISR_HOOK - ISR to measure time laps between incoming symbols. Catches t3.5 and helps to catch t1.5
	- USCI_A0_ISR_HOOK - UART ISR. It's common if Transmit a Byte or Receive a Byte happens (for all MSP430FR57xx)
									So it has to be reviewed if UART interrupts are different
	- MBparsing() - parses a received frame, checks syntax and prepares a response. Uses several functions inside
									this file and an external one - CRC calculation

MB_crc.c - may be used AS IS, may be substituted by another implementation

MB_hardware.h - totally depends on target MCU

	It's included in C-files and contains MACROs defined so that the text of MB_parsing.c becomes "platform-independent"
	It has to be totally revised if the MCU and/or pinout are changed

MB.hardware.c - totally and intendedly depends on target MCU

	- MBHWinit() - timer and UART init. Uses a lot of defines from MSP430 "driverlib" to make all configuration
					more or less explicit.
	- NBR[], Nt35[], Nt25[] - tables of baud rate dependent codes for timer and UART init. They contain only 4 numbers
					which may be changed according to user-chosen baud rates. The size of tables may be changed, but
					look in MBinit() and change a mask: "br_indx & 3"


3. *****************************************************	Application Interface

	The program that uses this Modbus library has to contain:

1) MODBUS definitions

typedef struct {
	uint16_t *pin;
	uint16_t *pwk;
	uint8_t  regsnumb;
	uint8_t  br_indx;
	uint8_t  id_indx;
	uint8_t  showmode;
} mb_struct;

#define	LASTINDEX	50	 (Or any chosen value)

2) Variables

mb_struct	mbs;
uint16_t	ParsWk[LASTINDEX];
uint16_t	ParsIn[LASTINDEX];

3) Initializing of "mbs"

mbs.pin = ParsIn;
mbs.pwk = ParsWk;
mbs.regsnumb = LASTINDEX;
br_indx = 0...3;
id_indx - 0...3;
showmode = 0;		// Look carefully #define SET_ST(s) in MB_hardware.h if you want to show MBState on MCU pins

4) Calls of 2 functions

MBinit( &mbs) - at least once - to pass parameters to a Modbus stack and initialize hardware
MBserve() - somewhere in a main loop of a program. It finishes the work of the Modbus stack from the last MBserve() call


4. *****************************************************	Adjustment

1) PARSE_IN_INT - in MB_header.h
			If defined it causes all parsing to be performed in TIMER1_A0_ISR_HOOK. It allows the quickest response
			of the Slave to a request: timer t3.5 interrupt immediately begins parsing and responding. The only
			disadvantage: the MCU is in ISR during all parsing. In my program it is 124 us of time.
			You may decide to yourself whether it is really bad.

2) Modbus functions list. There are several function codes in MB_header.h
			To add a function you have to modify switch(MBBuff[1]) in FrameParse()

3) MCU clock source (frequency) is used (and defined) in MB_hardware.c only:	#define	FSMCLK_HZ 	(7372800UL/2)

4) Baud rate defines NBR[], Nt35[], Nt25[] tables as mentioned above

5) Slave ID my be 1 of 4 values defined in IDTbl[] in MB_parsing.c
			The values may be changed directly in the table.

6) MCU. To adjust the Library to another target MCU it is absolutely necessary to look up
	- MB_hardware.h
	- MB_hardware.c
	- MB_parsing.c - both ISRs. It may happen that only slight syntax corrections are needed. But if UART interrupts
					for received and transmitted symbol are separate or the "last bit transmitted" interrupt is absent
					- the text has to be modified accordingly
	 And it's a good idea to change #include <msp430.h> in MB_header.h :)


5. *****************************************************	Main application example

int main(void) {
	...
 	vInit();
 	mbs.showmode = 1;
 	mbs.regsnumb = LASTINDEX;
 	mbs.pin = ParsIn;
 	mbs.pwk = ParsWk;
    mbs.br_indx  = CHK_PIN( BR0_PORT, BR0_PIN)? 1:0;
    mbs.br_indx |= CHK_PIN( BR1_PORT, BR1_PIN)? 2:0;
	mbs.id_indx  = CHK_PIN( SLVID0_PORT, SLVID0_PIN)? 1:0;
	mbs.id_indx |= CHK_PIN( SLVID1_PORT, SLVID1_PIN)? 2:0;

 	MBinit( &mbs);
    __bis_SR_register(GIE);

    do {
    	WDTCTL = WDTPW + WDTCNTCL;
    	if( MBserve()) MBtmo = MBTIMEOUT;	// Reset program Modbus watchdog
    	vMeasureService();
    	vProcessInpBuf();					// Process ParsIn. Ane nes from Modbus?
    } while( 1);
}


*/



#include <msp430.h>
#include "MB_hardware.h"	// Hardware depended defines

#define	PARSE_IN_INT		/* Parse command inside TIMER1_A0_ISR_HOOK ISR. Quick response but 124 us in interrupt subroutine	*/

#define MB_FRAME_MIN     		4       /* Minimal size of a Modbus RTU frame	*/
#define MB_FRAME_MAX     		256     /* Maximal size of a Modbus RTU frame	*/
#define MB_ADDRESS_BROADCAST  	00		/* MBBuff[0] analysis					*/

#define MB_FUNC_NONE							00
#define MB_FUNC_READ_COILS						01
#define MB_FUNC_READ_DISCRETE_INPUTS			02
#define MB_FUNC_WRITE_SINGLE_COIL				05
#define MB_FUNC_WRITE_MULTIPLE_COILS			15
#define MB_FUNC_READ_HOLDING_REGISTER			03	/* implemented now	*/
#define MB_FUNC_READ_INPUT_REGISTER				04
#define MB_FUNC_WRITE_REGISTER					06
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS		16	/* implemented now	*/
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS	23
#define MB_FUNC_ERROR							0x80

typedef enum {
    MBE_NONE 					= 0x00,
    MBE_ILLEGAL_FUNCTION 		= 0x01,
    MBE_ILLEGAL_DATA_ADDRESS	= 0x02,
    MBE_ILLEGAL_DATA_VALUE		= 0x03
} eMBExcep;

typedef enum {
    STATE_IDLE,			// Ready to get a frame from Master
    STATE_RCVE,			// Frame is being received
    STATE_PARS,			// Frame is being parsed (may take some time)
    STATE_SEND,			// Response frame is being sent
    STATE_SENT			// Last byte sent to shift register. Waiting "Last Bit Sent" interrupt
} eMBState;

typedef enum {			// Actually only 1 variable uses this type: ER_frame_bad
	EV_NOEVENT,
    EV_HAPPEND
} eMBEvents;

typedef enum {			// Boolean
	FALSE,
	TRUE
} eLogical;

typedef struct {		// Main program passes interface data to Modbus stack.
	uint16_t *pin;		// Pointer to the begin of ParsIn array. Modbus writes data in the array
	uint16_t *pwk;		// Pointer to the begin of ParsWk array. Modbus takes data from the array
	uint8_t  regsnumb;	// Total number of registers (=pars), the size of ParsIn and ParsWk arrays
	uint8_t  br_indx;	// Index in tables of baudrate-depended values - MBUART_init()
	uint8_t  id_indx;	// Index in IDTbl[] - to get SlaveID and compare with MBBuff[0] in parsing
	uint8_t  showmode;	// If TRUE - Modbus stack shows current MBState at MCU pins
} mb_struct;







