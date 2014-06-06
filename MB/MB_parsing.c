/*
 * MB main functions: get (interrupts), parse and response a Master's request
 *
 *  Created on: 26 жовт. 2013
 *      Author: drvlas
 */
#include "MB_header.h"

/*
 * Functions
 */
extern uint16_t	CRC16( uint8_t *, uint16_t);// look MB_crc.c
extern uint16_t MBHWinit( uint16_t);		// look MB_hardware.c

void			MBparsing( void);
inline eLogical	InvalidFrame( void);		// Actually it's a part of MBparsing() text
inline eLogical	FrameParse(void);			// Actually it's a part of MBparsing() text
inline void		BeginResponse( void);		// Actually it's a part of MBparsing() text

/*
 * Variables, that are changed in interrupts
 */
volatile uint8_t	MBIndx;					// MBBuff[] index
volatile eMBState	MBState;				// MB State Machine state (enumerated)
volatile eMBEvents	ER_frame_bad = 			// Any kind of error, detected during frame receiving
							EV_NOEVENT;
volatile uint8_t	MBBuff[MB_FRAME_MAX];	// Input-output Modbus buffer

/*
 * Static variables, used to pass data among several functions
 */
uint8_t		ResponseSize;					// Set in FrameParse(), used in transmit
eLogical	RequestParsed;					// A new MB request is parsed (and responded if need be)
uint16_t	Nt25_code;						// Set in MBTimer_init() and used to catch a flow break t1.5 event

/*
 * Almost a "copy" of the mbs-structure (declared in main application). Data from "mbs" is latched and is being used locally
 */
uint16_t	*RegsInp;						// (Constant) pointer to the begin of ParsIn[]
uint16_t	*RegsOut;						// (Constant) pointer to the begin of ParsWk[]
uint8_t		RegsTotal;						// Size of ParsIn[] and ParsWk[] (to check data address)
uint8_t		SlaveID;						// Keeps Slave ID (address)
uint8_t		ShowState;						// Used in SET_ST macro. Makes MCU to output MBState to 3 port pins

const
uint8_t 	IDTbl[] = { 100, 3, 2, 1};		// I use Slave addresses 100, 3, 2, 1. You may change it, bro!

/*
 * Modbus hardware and variables initializing. Must be called each time the baud rate or Slave Id are changed
 */
void
MBinit( mb_struct *mbp) {
	SlaveID = IDTbl[mbp->id_indx & 3];		// Used in MBparsing
	RegsInp = mbp->pin;						// Pointer to register array to receive Modbus data
	RegsOut = mbp->pwk;						// Pointer to register array to be send to Modbus
	RegsTotal = mbp->regsnumb;				// Size of arrays
	ShowState = mbp->showmode;				// Output MBState at MCU pins (look SET_ST(s) in MB_hardware.h)
	Nt25_code = MBHWinit(mbp->br_indx & 3);	// Look NBR[] and other tables in MB_hardware.c
	SET_ST(STATE_IDLE);						// Initial state of MB state machine
}

/*
 * Must be called somewhere in a main loop of application. Each time the MBserve() returns 1 the main program
 * 	knows - one more Modbus request is received and parsed. This may be used in some sort of "program watchdog"
 * 	to detect the absence of a Master on line (or wrong communication parameters or communication errors)
 *
 * If "PARSE_IN_INT" is defined, the MBserve() deals with RequestParsed only. The real request parsing is done
 * 	inside TIMER1_A0_ISR_HOOK Interrupt Service Routine. This means the Modbus stack functions totally in the
 * 	interrupts. Quickest response! But during the parsing of a request the MCU stays in TIMER1_A0_ISR_HOOK for
 * 	some time (have to be measured...) - which is a "mauvais ton" in coding
 */
uint8_t
MBserve( void) {						// returns 1 if a new frame is parsed
#ifndef	PARSE_IN_INT
	if( STATE_PARS == MBState)
		MBparsing();					// else it`s called in USCI_A0_ISR_HOOK
#endif
	if( RequestParsed) {				// Might be set in MBparsing()
		RequestParsed = FALSE;
		return 1;						// A frame was received and parsed
	}
	else
		return 0;
}

/*
 *  ======== Timer1_A3 Interrupt Service Routine ========
 */
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR_HOOK(void) {		// t3.5 hunting
	if( STATE_RCVE == MBState) {				// If we are receiving, it's the end event: t3.5
		SET_ST(STATE_PARS);						// Begin parsing of a frame. Here or in MBserve()
		STOP_TIMER;								// Less surprises in life...
#ifdef	PARSE_IN_INT
		MBparsing();							// Parse & response. Look "RequestParsed" also
#endif
	}
}

/*
 *  ======== eUSCI_A0 Interrupt Service Routine ========
 */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR_HOOK(void) {

	uint16_t cnt;

	switch(__even_in_range(UCA0IV,18)) {		// All UART happenings cause 1 interrupt

	case RX_INT:				// Vector 2: UCRXIFG, a char received
		cnt = TIMER_COUNT;
		RESET_TIMER;							// Timer reset anyway: received symbol means NO SILENCE
		if( STATE_RCVE == MBState) {
			if( cnt > Nt25_code) {				// t1.5 from the last 02 interrupt to begin of this symbol
				ER_frame_bad = EV_HAPPEND;		// This error will be processed later
				break;							// Nothing more to do in RECEIVE state
			}
			if( MBIndx > MB_FRAME_MAX-1) {		// Last acceptable position is 255 = MB_FRAME_MAX-1
				ER_frame_bad = EV_HAPPEND;
				break;							// Ignore big frames
			}
			MBBuff[MBIndx++] = BYTE_RECEIVED;	// MAIN DOING: New byte to buffer
		}
		else {									// 1-st symbol come!
			MBBuff[0] = BYTE_RECEIVED;			// Put it to buffer
			MBIndx = 1;							// "Clear" the rest of buffer
			ER_frame_bad = EV_NOEVENT;			// New buffer, no old events
			TO_RECEIVE;							// RS-485 driver - to receive mode
			SET_ST(STATE_RCVE);					// MBMachine: begin of receiving the request
			break;
		}

	case TX_INT: 				// Vector 4: UCTXIFG, a char transmitted

		if( STATE_SEND == MBState) {
			if( MBIndx <= ResponseSize) {
				BYTE_TO_SEND = MBBuff[MBIndx++];// MAIN DOING: sending of the next byte
				break;
			}
			SET_ST(STATE_SENT); 				// Frame sent. Wait for the last bit to be sent
			break;
		}
		break;									// Ignore interrupt if no STATE_SEND

	case LASTBIT_INT:			// Vector 8: UCTXCPTIFG, last bit transmitted

		if( STATE_SENT == MBState) {
			TO_RECEIVE;							// RS-485 driver - to receive mode
			SET_ST(STATE_IDLE);
		}
		break;
	default: break;
	}
}


void
MBparsing( void) {
	if( InvalidFrame()) {
		SET_ST(STATE_IDLE);			// Wait new frame. T35 is got earlier
		return;
	}
	RequestParsed = TRUE;				// Flag for MBserve()
	if( FrameParse()) {				// Need response?
		BeginResponse();			// Send 1-st byte
		SET_ST(STATE_SEND);			// To avoid entering STATE_PARS case once more
	}
	else {							// The request was broadcast
		SET_ST(STATE_IDLE);			// Wait new request. T35 is got already
	}
}

eLogical
InvalidFrame( void) {
	uint8_t	PDU_len;
	if( EV_HAPPEND == ER_frame_bad) {
		return TRUE;						// Any byte error or buffer overflow during RCVE state
	}
	if( MBIndx < MB_FRAME_MIN) {
		return TRUE;						// Frame is too small
	}
	PDU_len = MBIndx;						// Including Checksum. Then CRC calculated must be ZERO
	if( CRC16( (uint8_t*)MBBuff, PDU_len)) {
		return TRUE;						// CRC != Checksum
	}
	if( SlaveID != MBBuff[0]) {
		return TRUE;						// No mbs.ring of another's frame
	}
	return FALSE;
}

eLogical
FrameParse( void) {							// Returns TRUE if a response is needed
	uint16_t 	RegIndx, RegNmb, RegLast, i;
	uint8_t		BytesN;
	eMBExcep	Exception;					// If a Modbus exception happens - we put the var in MBBuff[2]

	eLogical NeedResponse = TRUE;
	if( MBBuff[0] == MB_ADDRESS_BROADCAST) {
		NeedResponse = FALSE;				// We parse the request but we don'n give a response
	}

	switch( MBBuff[1]) {					// Function code

	case MB_FUNC_READ_HOLDING_REGISTER:
					//		  03	0...LASTINDEX-1      0...125
					// addr  func    AddrHi  AddrLo  QuantHi  QuantLo  CRC CRC
					//  0	  1			2		3		4		5		 6	7
		if( 8 == MBIndx) {								// In this function MBIndx == 8.
			RegIndx = (MBBuff[2]<<8) | (MBBuff[3]&0xFF);
			RegNmb  = (MBBuff[4]<<8) | (MBBuff[5]&0xFF);
			RegLast = RegIndx + RegNmb;
			if( (RegIndx > RegsTotal-1) ||
				(RegLast > RegsTotal-1)) {
				Exception = MBE_ILLEGAL_DATA_ADDRESS;
				break;
			}
														// Make response. MBBuff[0] and MBBuff[1] are ready
			MBBuff[2] = RegNmb << 1;
			MBIndx = 3;
	        while( RegIndx < RegLast ) {
	        	MBBuff[MBIndx++] = (uint8_t)(*(RegsOut+RegIndx) >> 8)  ;
	        	MBBuff[MBIndx++] = (uint8_t)(*(RegsOut+RegIndx) & 0xFF);
	        	++RegIndx;
	        }
	        Exception = MBE_NONE;						// OK, make CRC to MBBuff[MBIndx] and response is ready
	        break;
		}
		Exception = MBE_ILLEGAL_DATA_VALUE;	// PDU length incorrect
		break;

	case MB_FUNC_WRITE_MULTIPLE_REGISTERS:
					//		  16	0...LASTINDEX-1      0...125	 0...250
					// addr  func    AddrHi  AddrLo  QuantHi  QuantLo  Bytes  RG1Hi RG1LO ... CRC CRC
					//  0	  1			2		3		4		5		 6		7	  8	  ...

		if( MBIndx > 10) {								// The must: MBIndx == 11, 13, 15, ...
			RegIndx = (MBBuff[2]<<8) | (MBBuff[3]&0xFF);
			RegNmb  = (MBBuff[4]<<8) | (MBBuff[5]&0xFF);
			RegLast = RegIndx + RegNmb;
			BytesN	= MBBuff[6];
			if( (RegIndx > RegsTotal-1) ||
				(RegLast > RegsTotal-1)) {
				Exception = MBE_ILLEGAL_DATA_ADDRESS;
				break;
			}
			if( BytesN != (RegNmb << 1) ||
				MBIndx != (9+BytesN) ) {				// 1 reg - MBIndx=11, 2 regs - 13,... 5 regs - 19, etc.
				Exception = MBE_ILLEGAL_DATA_VALUE;
				break;
			}
			i = 7;										// Registers' values are from MBBuff[7] and more
			while( RegIndx < RegLast) {
				*(RegsInp+RegIndx)  = MBBuff[i++]<<8;	// High, then Low byte
				*(RegsInp+RegIndx) |= MBBuff[i++]	;	// ... are packed in a WORD
				++RegIndx;
			}
			MBIndx = 6;									// MBBuff[0] to MBBuff[5] are ready (unchanged)
			Exception = MBE_NONE;						// ...and MBIndx is a length of response
			break;
		}
		Exception = MBE_ILLEGAL_DATA_VALUE;				// PDU length incorrect
		break;

	default:
		Exception = MBE_ILLEGAL_FUNCTION;
		break;
	}

	/*
	 * At this point the "MBIndx" is a length of the response (w/o CRC bytes)
	 * MBIndx is variable if MB_FUNC_READ_HOLDING_REGISTER, 6 if MB_FUNC_WRITE_MULTIPLE_REGISTERS)
	 * but if there's some exception - it will be shortened to 3 in both cases
	 */

	if( Exception != MBE_NONE) {						// Any exception?
		MBBuff[1] |= MB_FUNC_ERROR;						// Add 0x80 to function code
		MBBuff[2] =  Exception;							// Exception code
		MBIndx = 3;										// Length of response is fixed if exception
	}

	i = CRC16( (uint8_t*)MBBuff, MBIndx);				// MBBuff is a pointer, MBIndx is a size
	MBBuff[MBIndx++] = i & 0xFF;						// CRC: Lo then Hi
	MBBuff[MBIndx  ] = i >> 8;
	ResponseSize = MBIndx;
	return NeedResponse? TRUE:FALSE;
}

inline void
BeginResponse( void) {
	TO_TRANSMT;								// RS-485 driver - to send mode
	BYTE_TO_SEND = MBBuff[0];				// 1-st byte to send - in Tx UART register
	MBIndx = 1;
}

