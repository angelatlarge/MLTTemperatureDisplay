// Copyright (c) 2012 All Right Reserved, Kirill Shklovsky
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/*	
	Cathodes
			SR595 pin	SR595 log	ULN in		ULN out
			pin 7		Q7			pin 1		pin 16	- LED5&6
			pin 6		Q6			pin 2		pin 15 	- LED1&2
			pin 5		Q5			pin 3		pin 14 	- LED3&4
			pin 4		Q4			pin 4		pin 13 	NC
			pin 3		Q3			pin 5		pin 12 	- LED3
			pin 2		Q2			pin 6		pin 11 	- LED2
			pin 1		Q1			pin 7		pin 10 	- LED1
			pin 15		Q0
						pin 5 GND |	pin  9 	- VCC
						
			LED1&2 = SR595 Q6
			LED3&4 = SR595 Q5
			LED5&6 = SR595 Q7
			7SEG1  = SR595 Q1
			7SEG2  = SR595 Q2
			7SEG3  = SR595 Q3
		
	Channel		Wire	What?
	2			Purple	RESET
	4			Blue	OE
	1			Black	output to SR595OE
	
*/

#define SPEEDUP8X
//~ #undef SPEEDUP8X
#ifdef SPEEDUP8X
#define F_CPU 16000000UL /* 8 MHz Internal Oscillator */
//~ #define F_CPU 8000000UL /* 8 MHz Internal Oscillator */
#else
#define F_CPU 2000000UL /* 1 MHz Internal Oscillator */
//~ #define F_CPU 1000000UL /* 1 MHz Internal Oscillator */
#endif
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include "sr595.h"
//~ #include <math.h>

#define READ_BTH

#define LEDSEG_A	(1<<5)	
#define LEDSEG_B	(1<<4)	
#define LEDSEG_C	(1<<2)	
#define LEDSEG_DP	(1<<3)	
#define LEDSEG_D	(1<<1)	
#define LEDSEG_E	(1<<0)	
#define LEDSEG_F	(1<<6)	
#define LEDSEG_G	(1<<7)	

#define kINDICATOR_COUNT 	6
#define IND0_R		LEDSEG_D	
#define IND0_G		LEDSEG_E	
#define IND0_B		LEDSEG_F	
#define IND1_R		LEDSEG_A	
#define IND1_G		LEDSEG_B	
#define IND1_B		LEDSEG_C	
uint8_t	aintIndicatorSegmentR[2] = {IND0_R, IND1_R};
uint8_t	aintIndicatorSegmentG[2] = {IND0_G, IND1_G};
uint8_t	aintIndicatorSegmentB[2] = {IND0_B, IND1_B};

//~ uint8_t aintIndicatorCathode[kINDICATOR_COUNT] = {6, 6, 5, 5, 7, 7, 1};
uint8_t aintIndicatorCathode[kINDICATOR_COUNT] = {6, 6, 5, 5, 7, 7};
// Font and digits for 7segment
// ... digits
#define LED7OUT_0		LEDSEG_A | LEDSEG_B | LEDSEG_C | LEDSEG_D | LEDSEG_E | LEDSEG_F
#define LED7OUT_1		LEDSEG_B | LEDSEG_C
#define LED7OUT_2		LEDSEG_A | LEDSEG_B | LEDSEG_G | LEDSEG_E | LEDSEG_D
#define LED7OUT_3		LEDSEG_A | LEDSEG_B | LEDSEG_G | LEDSEG_C | LEDSEG_D
#define LED7OUT_4		LEDSEG_F | LEDSEG_G | LEDSEG_B | LEDSEG_C
#define LED7OUT_5		LEDSEG_A | LEDSEG_F | LEDSEG_G | LEDSEG_C | LEDSEG_D
#define LED7OUT_6		LEDSEG_A | LEDSEG_F | LEDSEG_G | LEDSEG_E | LEDSEG_C | LEDSEG_D
#define LED7OUT_7		LEDSEG_A | LEDSEG_B | LEDSEG_C
#define LED7OUT_8		LEDSEG_A | LEDSEG_B | LEDSEG_C | LEDSEG_D | LEDSEG_E | LEDSEG_F | LEDSEG_G
#define LED7OUT_9		LEDSEG_A | LEDSEG_B | LEDSEG_C | LEDSEG_D | LEDSEG_F | LEDSEG_G
// ... letters
#define LED7OUT_A		LEDSEG_A | LEDSEG_F | LEDSEG_B | LEDSEG_G | LEDSEG_E | LEDSEG_C
#define LED7OUT_B		LEDSEG_F | LEDSEG_G | LEDSEG_E | LEDSEG_C | LEDSEG_D
#define LED7OUT_C		LEDSEG_A | LEDSEG_F | LEDSEG_E | LEDSEG_D
#define LED7OUT_D		LEDSEG_B | LEDSEG_G | LEDSEG_E | LEDSEG_C | LEDSEG_D
#define LED7OUT_E		LEDSEG_A | LEDSEG_F | LEDSEG_G | LEDSEG_E | LEDSEG_D
#define LED7OUT_F		LEDSEG_A | LEDSEG_F | LEDSEG_G | LEDSEG_E
// ... symbols
#define LED7OUT_DASH	LEDSEG_G
#define LED7OUT_ALL		0xFF

uint8_t aint7segdigits[] = {LED7OUT_0, LED7OUT_1,LED7OUT_2,LED7OUT_3,LED7OUT_4,LED7OUT_5,LED7OUT_6,LED7OUT_7,LED7OUT_8,LED7OUT_9};

/////////////////////////////////////////////////////////////////////
// SR595

#define SR74XX595_PORT	PORTD
#define SR74XX595_DDR	DDRD
#define SR74XX595_DS	02
#define SR74XX595_SHCP	05
#define SR74XX595_STCP0	04
#define SR74XX595_OE	07
#define SR74XX595_STCP1	06

#ifdef BTH_USE_PINKEY
#define BTH_PINKEY_DDR	DDRD
#define BTH_PINKEY_PORT	PORTD
#define BTH_PINKEY_PIN	1<<7
#endif
#define BTH_USE_POWER
#ifdef BTH_USE_POWER
#define BTH_POWER_DDR	DDRB
#define BTH_POWER_PORT	PORTB
#define BTH_POWER_PIN	1<<0
#endif 


#define DELAY_LONG	1000
#define DO_SHORT_DELAY	_delay_ms(250)
//~ #define DO_SHORT_DELAY	;
	
uint8_t STCP[2] = {SR74XX595_STCP0, SR74XX595_STCP1};
#undef PARALLEL_595
sr595 sr(
		2, 					// nCascadeCount
#		ifdef PARALLEL_595
		1, 					// fParallel
#		else PARALLEL_595
		0, 					// fParallel
#		endif PARALLEL_595
		&SR74XX595_PORT,        // ptrPort
		&SR74XX595_DDR,        // ptrDir
		SR74XX595_OE, 		// nOE
		1, 					// nInvertOE
		SR74XX595_DS, 		// nDS
		SR74XX595_SHCP, 	// nSHCP
		STCP				// anSTCP
		);

/////////////////////////////////////////////////////////////////////
// Regimes
#define kREGIME_DISPLAYVALUES	0
#define kREGIME_SETTIMER		1
#define kREGIME_TESTLEDS		2
volatile		uint8_t		nRegime = 0;

/////////////////////////////////////////////////////////////////////
// Keys control
#define TIMER1_OCR1A_FPU_DIV	351360


#define kMAX_KEYBOUNCE_CHECKS	6
#define INPUTS_DIR		DDRB
#define INPUTS_PORT		PORTB
#define INPUTS_PIN		PINB
#define	INPUT_BTNRIGHT		(01<<02)
#define	INPUT_BTNLEFT		(01<<04)
#define	INPUT_ENCODERLEFT	(01<<01)
#define	INPUT_ENCODERRIGHT	(01<<03)
#define	INPUT_ENCODERBTN	(01<<05)
#define INPUT_ALL		(INPUT_BTNRIGHT | INPUT_BTNLEFT | INPUT_ENCODERLEFT | INPUT_ENCODERRIGHT | INPUT_ENCODERBTN)
			uint8_t		aintDebounceState[kMAX_KEYBOUNCE_CHECKS];
			uint8_t		intKeyState;
			uint8_t		idxKeyState;
			uint8_t		nStopValueCycling;
#define	INPUT_IGNORE_ENCODER_OPPDIR_TICK_COUNT	24			
			uint8_t		nEncoderRotationIgnoreTickCount;
/////////////////////////////////////////////////////////////////////
// Other low-res timer stuff

// Value cycling
volatile	uint16_t	nValueCycleCount;							// Count of for the purposes of value cycling
#define KEYTIMER_MAX_VALUECYCLE	515									/* Update the display every 1.5 seconds
																		since the speed of the slow timer is independent of the CPU speed
																		this value need not be adjusted for FCPU 
																	*/

volatile	uint32_t	nTimingCount;								// Count for the purposes of displaying a timer
			uint16_t	nDisplayTimerValue; 
			uint8_t		nMinuteTimerDot; 
volatile	uint16_t 	nTimerSeconds; 
volatile	uint8_t 	nTimerMinutes;
			
			uint8_t		nCountUp = 1; 
#define COUNTDOWNTIMER_MAXMINUTES			120			
			
			uint16_t	nKeyPressCycleCount;						/* 	Counts how long a key has been pressed
																		zero when nothing is pressed */
			uint8_t		nIgnoreKeyRelease;			
#define kLONGPRESS_MAX_CYCLE_COUNT	500									/* Long press 
																		since the speed of the slow timer is independent of the CPU speed
																		this value need not be adjusted for FCPU 
																		*/

volatile	uint32_t	nBthUpdateCount;
#define KEYTIMER_MAX_BTHUPDATECOUNT	515									/* Update bluetooth every 1.5 seconds
																		since the speed of the slow timer is independent of the CPU speed
																		this value need not be adjusted for FCPU 
																	*/



/////////////////////////////////////////////////////////////////////
// Led control


#define kDISPVALUE_COUNT				4
#define kIDXDISPVALUE_SETTING			kDISPVALUE_COUNT
#define kIDXDISPVALUE_TIMER				0
#define kDISPVALUE_NOVALUEAVAILABLE		0xFFFF
#define kDISPVALUE_DIGITCOUNT			3
#define kDISPVALUE_INDPAIRSCOUNT		3
#define kDISPVALUE_CATHODECOUNT			(kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT)

volatile	uint16_t	aintDisplayValue[kDISPVALUE_COUNT+1];  				// 	Value to be displayed in the LEDs
																			//  Extra value is for regimes other than value cycling
volatile	uint8_t		idxDisplayValue = 0;
volatile	uint8_t		aintDisplaySegments[kDISPVALUE_COUNT+1][kDISPVALUE_CATHODECOUNT];   // 	Value to be displayed on each led
																							// 	Extra value is for other regimes
volatile	uint8_t		nResetting;
//~ volatile	uint8_t 	idxActiveCathode = 0;

inline uint16_t getDisplayValue(uint8_t idxDispValue) {
	return aintDisplayValue[idxDispValue];
}

void setDisplayValue(uint8_t idxDispValue, uint16_t intNewValue) {
	if (nResetting) { return; }
	if (aintDisplayValue[idxDispValue]==intNewValue) { return; }
	
	switch (aintDisplayValue[idxDispValue] = intNewValue) {
		case kDISPVALUE_NOVALUEAVAILABLE: {
			for (int idxDigit=0; idxDigit<kDISPVALUE_DIGITCOUNT; idxDigit++) {
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
					aintDisplaySegments[idxDispValue][idxDigit] = LED7OUT_DASH;
				}
			}
			break;
		}
		default: {
			for (int idxDigit=0; idxDigit<kDISPVALUE_DIGITCOUNT; idxDigit++) {
				int nCurrentDigit = intNewValue;
				for (int i = 0; i<idxDigit; i++) {
					nCurrentDigit = nCurrentDigit / 10;
				}
				nCurrentDigit = nCurrentDigit % 10;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
					aintDisplaySegments[idxDispValue][idxDigit] = aint7segdigits[nCurrentDigit];
				}
			}
		}
	}
}

//////////////////////////////////////////////
// ADC stuff

#define ADC_VBITS	16

#if ADC_VBITS==13
// Virtual 13-bit ADC settings
#define kSAMPLE_COUNT	64
#define kDECIMATE_RIGHTSHIFT	3
#define kADV_TEMPCONVERT_MULT	0.125
#elif ADC_VBITS==14
// Virtual 14-bit ADC settings
#define kSAMPLE_COUNT	256
#define kDECIMATE_RIGHTSHIFT	4
#define kADV_TEMPCONVERT_MULT	0.0625
#elif ADC_VBITS==15
// Virtual 15-bit ADC settings
#define kSAMPLE_COUNT	1024
#define kDECIMATE_RIGHTSHIFT	5
#define kADV_TEMPCONVERT_MULT	0.03125
#elif ADC_VBITS==16
// Virtual 16-bit ADC settings
#define kSAMPLE_COUNT	4096
#define kDECIMATE_RIGHTSHIFT	6
#define kADV_TEMPCONVERT_MULT	0.015625
#endif 

#define kADC_COUNT		3

			int8_t 		aintTempAdjust[kADC_COUNT] = {0, 0, +7};
			uint8_t 	aidxADC2DispValue[kADC_COUNT] = {1, 2, 3};	// Index of ADC value to Disp value index
			uint32_t	anADCRead[kADC_COUNT];  					// 	Values read from the ADC
			uint16_t	anSampleCount[kADC_COUNT]; 	 				// 	Number of samples read from the ADC
volatile	uint8_t		anLastDisplayedValue[kADC_COUNT];  			// 	Final values
			uint8_t		idxADCValue;
			uint8_t		ADMUXbase;									// ADMUX without the channel bits

uint16_t tempFromADC(uint16_t intADCValue) {
	/* 	Michaelis-Menten Isotope Displacement Double ([Hot] subsumed) With Offset
		y = a / (b + x) + c / (d + x) + Offset
		Fri Jul 13 15:52:38 2012 local server time
	*/
	double a = -1.7317894461536247E+05;
	double b = -1.5625932997339971E+03;
	double c = -1.9589674327464432E+04;
	double d = 1.4053861754270164E+02;
	double Offset = 1.5684078545920563E+01;
	double ADCValueStart = intADCValue * kADV_TEMPCONVERT_MULT;
	double fRetVal;
	fRetVal = (a / (b + ADCValueStart) + c / (d + ADCValueStart) ) + Offset;
	return round(fRetVal);
}


//////////////////////////////////////////////
// Beeps
#define kBEEP_CONTROL_MAXENTRIES	9

#define SPKOUT_OC2A	1
#define SPKOUT_OC2B	2
#define SPEAKER_OUT_PIN SPKOUT_OC2B

#if SPEAKER_OUT_PIN == 0
#error Programming error in speaker settings
#elif SPEAKER_OUT_PIN == SPKOUT_OC2A
#warning SPEAKER_OUT_PIN is set to SPKOUT_OC2A
#define kSPEAKER_PORT			PORTB
#define kSPEAKER_DDR			DDRB
#define kSPEAKER_PIN			3
#elif SPEAKER_OUT_PIN == SPKOUT_OC2B
#warning SPEAKER_OUT_PIN is set to SPKOUT_OC2B
#define kSPEAKER_PORT			PORTD
#define kSPEAKER_DDR			DDRD
#define kSPEAKER_PIN			3
#else
#error Unknown SPEAKER_OUT_PIN
#endif

#ifndef kSPEAKER_PORT
#error kSPEAKER_PORT is not defined
#endif
#ifndef kSPEAKER_DDR
#error kSPEAKER_DDR is not defined
#endif
#ifndef kSPEAKER_PIN
#error kSPEAKER_PIN is not defined
#endif

#define kIDXNEXTBEEP_NEXT			0xFF
#define kIDXNEXTBEEP_STOP			0xFE
#define kIDXNEXTBEEP_FIRST			0x00
typedef struct {
	uint16_t	nFreq;
	uint16_t	nAudibleCount;
	uint16_t	nSilentCount;
	uint8_t		idxNextBeep;
} beepStruct;
volatile		beepStruct	anBeepControl[kBEEP_CONTROL_MAXENTRIES];
#define kBEEPCONTROL_NOBEEP	0xFF
volatile		uint8_t		idxBeepControl = kBEEPCONTROL_NOBEEP;
volatile		uint8_t		nBeepControlCount;
				uint16_t	nBeepTimer;
				uint8_t		nBeepAudible;
volatile		uint8_t 	nBeginBeep;


inline void beepTurnOff() {
	kSPEAKER_DDR &= ~(1<<kSPEAKER_PIN);			/* turn off the OC PIN */
	kSPEAKER_PORT |= 1<<kSPEAKER_PIN;			/* set this pin high */
	TCCR2A &= ~((1<<COM2A1)|(1<<COM2A0));		/* Normal operation: OC pins disconnected */
	//~ TCCR2A &= ~((1<<COM2A1)|(1<<COM2A0)|(1<<COM2B1)|(1<<COM2B0));		/* Normal operation: OC pins disconnected */
	TCCR2B = 0; 								// Timer stopped
}	

void beepStop() {
	nBeepControlCount = 0;
	idxBeepControl = kBEEPCONTROL_NOBEEP; 
	beepTurnOff();
}

void beepProcessing() {
	if ( nBeginBeep || (nBeepControlCount && (idxBeepControl != kBEEPCONTROL_NOBEEP)) ) {
		// We are beeping
		
		uint8_t nDoNextBeepStructure = 0;
		if (nBeginBeep) {
			idxBeepControl = 0;
			nBeepTimer = 0;
			nDoNextBeepStructure = 1;
		} else {
			if (nBeepAudible) {
				if (++nBeepTimer > anBeepControl[idxBeepControl].nAudibleCount) {
					if (anBeepControl[idxBeepControl].nSilentCount > 0) {
						// Do the silent part of the beep
						beepTurnOff();
						nBeepAudible = 0;
						nBeepTimer = 0;
					} else {
						// Proceed to next beep structure
						nDoNextBeepStructure = 1;
					}
				} // else: still doing audible
			} else { // Silent part
				if (++nBeepTimer > anBeepControl[idxBeepControl].nSilentCount) {
					// Proceed to next beep structure
					nDoNextBeepStructure = 1;
				} // else: still doing silent
			}
		}
		
		if (nDoNextBeepStructure) {
			nBeepTimer = 0;

			if (!nBeginBeep) {
				switch (anBeepControl[idxBeepControl].idxNextBeep) {
					case kIDXNEXTBEEP_NEXT : 
						idxBeepControl++;
						break;
					case kIDXNEXTBEEP_STOP : 
						idxBeepControl=nBeepControlCount;
						break;
					default:
						idxBeepControl = anBeepControl[idxBeepControl].idxNextBeep;
				}
			} else {
				nBeginBeep = 0;
			}
			
			if (idxBeepControl < nBeepControlCount) {
				nBeepAudible = 1;
				kSPEAKER_DDR |= 1<<kSPEAKER_PIN;			/* Connect the OC PIN */
				TIMSK2 = 0;							// No interrupts
				TCNT2  = 0;            				// 	Initial counter value
				/* CTC mode, toggle the output pin */
				TCCR2A = 0
#if SPEAKER_OUT_PIN == SPKOUT_OC2A
					| 0<<COM2A1	// COM2A1
					| 1<<COM2A0 // COM2A0
					| 0<<COM2B1 // COM2B1
					| 0<<COM2B0 // COM2B0
#elif SPEAKER_OUT_PIN == SPKOUT_OC2B
					| 0<<COM2A1	// COM2A1
					| 0<<COM2A0 // COM2A0
					| 0<<COM2B1 // COM2B1
					| 1<<COM2B0 // COM2B0
#endif
								// -
								// – 
					| 1<<WGM21	// WGM21
					| 0<<WGM20	// WGM20
					;
				TCCR2B = 0
					| 0<<FOC2A 	// FOC2A 
					| 0<<FOC2B 	// FOC2B
								// – 
								// – 
					| 0<<WGM22 	// WGM22
					;
				// Set the prescaler
				uint16_t nPrescaler; 
				switch (F_CPU) {
					case 20000000UL-16000000UL:
						// Prescaler = 1024
						nPrescaler = 1024;
						TCCR2B |=  (1<<CS22)|(1<<CS21)|(1<<CS20);
						break;
						// Prescaler = 1024
						nPrescaler = 1024;
						TCCR2B |=  (1<<CS22)|(1<<CS21)|(1<<CS20);
						break;
					case 8000000UL :
						// Prescaler = 256
						nPrescaler = 256;
						TCCR2B |=  (1<<CS22)|(1<<CS21)|(0<<CS20);
						break;
					case 1000000UL :
						// Prescaler = 32
						nPrescaler = 32;
						TCCR2B |=  (0<<CS22)|(1<<CS21)|(1<<CS20);
						break;
					default:
						// Prescaler = 256
						nPrescaler = 256;
						TCCR2B |=  (1<<CS22)|(1<<CS21)|(0<<CS20);
						break;
				}
				// Calculate the TOP
				//~ kSPEAKER_OCR2 = F_CPU / nPrescaler / anBeepControl[idxBeepControl].nFreq;
				OCR2A = F_CPU / nPrescaler / anBeepControl[idxBeepControl].nFreq;
				OCR2B = F_CPU / nPrescaler / anBeepControl[idxBeepControl].nFreq;
			} else {
				// Done beeping
				idxBeepControl = kBEEPCONTROL_NOBEEP;
				nBeepControlCount = 0;
				
				beepTurnOff();
			}
		}
	}
}

void beepA() {
	anBeepControl[0].nFreq = 2000;
	anBeepControl[0].nAudibleCount = 1;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_STOP;
	nBeepControlCount = 1;
	nBeginBeep = 1;
}

void beepB() {
	anBeepControl[0].nFreq = 440;
	anBeepControl[0].nAudibleCount = 16;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_NEXT;
	
	anBeepControl[1].nFreq = 554;
	anBeepControl[1].nAudibleCount = 16;
	anBeepControl[1].nSilentCount = 0;
	anBeepControl[1].idxNextBeep = kIDXNEXTBEEP_NEXT;
	
	anBeepControl[2].nFreq = 659;
	anBeepControl[2].nAudibleCount = 16;
	anBeepControl[2].nSilentCount = 0;
	anBeepControl[2].idxNextBeep = kIDXNEXTBEEP_NEXT;
	
	anBeepControl[3].nFreq = 880;
	anBeepControl[3].nAudibleCount = 32;
	anBeepControl[3].nSilentCount = 0;
	anBeepControl[3].idxNextBeep = kIDXNEXTBEEP_STOP;
	
	nBeepControlCount = 4;
	nBeginBeep = 1;
}

void beepC_lowlong() {
	anBeepControl[0].nFreq = 220;
	anBeepControl[0].nAudibleCount = 64;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_STOP;
	nBeepControlCount = 1;
	nBeginBeep = 1;
}

void beepD_higshort() {
	anBeepControl[0].nFreq = 800;
	anBeepControl[0].nAudibleCount = 32;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_STOP;
	nBeepControlCount = 1;
	nBeginBeep = 1;
}

void beepE_timerexpired() {
	for (int i = 0; i<5; i++) {
		anBeepControl[i].nFreq = 1000;
		anBeepControl[i].nAudibleCount = 16;
		anBeepControl[i].nSilentCount = 4;
		anBeepControl[i].idxNextBeep = kIDXNEXTBEEP_NEXT;
	}
	anBeepControl[4].idxNextBeep = 0;
	anBeepControl[4].nSilentCount = 80;
	nBeepControlCount = 5;
	nBeginBeep = 1;
}

void beepF_setTimer() {
	anBeepControl[0].nFreq = 800;
	anBeepControl[0].nAudibleCount = 16;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_NEXT;
	anBeepControl[1].nFreq = 951;
	anBeepControl[1].nAudibleCount = 16;
	anBeepControl[1].nSilentCount = 0;
	anBeepControl[1].idxNextBeep = kIDXNEXTBEEP_NEXT;
	anBeepControl[2].nFreq = 1270;
	anBeepControl[2].nAudibleCount = 32;
	anBeepControl[2].nSilentCount = 0;
	anBeepControl[2].idxNextBeep = kIDXNEXTBEEP_STOP;
	nBeepControlCount = 3;
	nBeginBeep = 1;
}

void beepG_finishSetTimer() {
	anBeepControl[0].nFreq = 1270;
	anBeepControl[0].nAudibleCount = 16;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_NEXT;
	anBeepControl[1].nFreq = 951;
	anBeepControl[1].nAudibleCount = 16;
	anBeepControl[1].nSilentCount = 0;
	anBeepControl[1].idxNextBeep = kIDXNEXTBEEP_NEXT;
	anBeepControl[2].nFreq = 800;
	anBeepControl[2].nAudibleCount = 32;
	anBeepControl[2].nSilentCount = 0;
	anBeepControl[2].idxNextBeep = kIDXNEXTBEEP_STOP;
	nBeepControlCount = 3;
	nBeginBeep = 1;
}

void beepH_440forever() {
	anBeepControl[0].nFreq = 440;
	anBeepControl[0].nAudibleCount = 999;
	anBeepControl[0].nSilentCount = 0;
	anBeepControl[0].idxNextBeep = kIDXNEXTBEEP_FIRST;
	nBeepControlCount = 1;
	nBeginBeep = 1;
}

void beepG_charge() {
#define	DEFAULT_LENGTH 32
	nBeepControlCount = 6;
	for (int i = 0; i<nBeepControlCount; i++) {
		anBeepControl[i].nAudibleCount = DEFAULT_LENGTH;
		anBeepControl[i].nSilentCount = DEFAULT_LENGTH/4;
		anBeepControl[i].nSilentCount = 0;
		anBeepControl[i].idxNextBeep = kIDXNEXTBEEP_NEXT;
	}
	anBeepControl[0].nFreq = 800;
	anBeepControl[1].nFreq = 1068;
	anBeepControl[2].nFreq = 1345;
	anBeepControl[3].nFreq = 1600;
	anBeepControl[3].nAudibleCount = DEFAULT_LENGTH*2;
	anBeepControl[4].nFreq = 1345;
	anBeepControl[4].nAudibleCount = DEFAULT_LENGTH/2;
	anBeepControl[5].nFreq = 1600;
	anBeepControl[5].nAudibleCount = DEFAULT_LENGTH*2;
	nBeginBeep = 1;
}


void beepH_vlesurodilas() {
#define	DEFAULT_LENGTH 32
	nBeepControlCount = 8;
	for (int i = 0; i<nBeepControlCount; i++) {
		anBeepControl[i].nAudibleCount = DEFAULT_LENGTH;
		anBeepControl[i].nSilentCount = DEFAULT_LENGTH/4;
		anBeepControl[i].nSilentCount = 0;
		anBeepControl[i].idxNextBeep = kIDXNEXTBEEP_NEXT;
	}
	anBeepControl[0].nFreq = 440;
	anBeepControl[1].nFreq = 784;
	anBeepControl[2].nFreq = 784;
	anBeepControl[3].nFreq = 698;
	anBeepControl[4].nFreq = 784;
	anBeepControl[5].nFreq = 622;
	anBeepControl[6].nFreq = 440;
	anBeepControl[7].nFreq = 440;
	nBeginBeep = 1;
}

void beepI_vlesuonarosla() {
#define	DEFAULT_LENGTH 32
	nBeepControlCount = 6;
	for (int i = 0; i<nBeepControlCount; i++) {
		anBeepControl[i].nAudibleCount = DEFAULT_LENGTH;
		anBeepControl[i].nSilentCount = DEFAULT_LENGTH/4;
		anBeepControl[i].nSilentCount = 0;
		anBeepControl[i].idxNextBeep = kIDXNEXTBEEP_NEXT;
	}
	anBeepControl[0].nFreq = 440;
	anBeepControl[1].nFreq = 784;
	anBeepControl[2].nFreq = 784;
	anBeepControl[3].nFreq = 698;
	anBeepControl[4].nFreq = 932;
	anBeepControl[5].nFreq = 784;
	anBeepControl[5].nAudibleCount = DEFAULT_LENGTH*2;
	nBeginBeep = 1;
}


//~ #include "uart.h"
//////////////////////////////////////////////
// Serial 
#include <stdio.h>
#include <avr/io.h>

#ifndef F_CPU
#       error Must define F_CPU or pass it as compiler argument
#endif

extern "C"{
 FILE * uart_out;
}

//~ FILE uart_out = FDEV_SETUP_STREAM(uart_putChar, NULL, _FDEV_SETUP_WRITE);
/*
void uart_init(){
#       define BAUD 9600
#       include <util/setbaud.h>
        UBRR0H = UBRRH_VALUE;
        UBRR0L = UBRRL_VALUE;
//~ #       if USE_2x
        //~ USCR0A |= _BV(U2X);
//~ #       else
        //~ USCR0A &= ~_BV(U2X);
//~ #       endif
        UCSR0B |= _BV(TXEN);                    // enable transmit
}

int uart_putChar(char c, FILE *unused){
        if (c == '\n')                          // a line feed character also
                uart_putChar('\r', unused);     // requires a carriage return
        loop_until_bit_is_set(UCSR0A, UDRE0);   // wait for UDR0 to be ready
        UDR0 = c;                               // write character
        return 0;                               // return
}
#endif
*/

//////////////////////////////////////////////
// Serial 

#include <stdio.h>
#define BAUD 9600

#include <util/setbaud.h>

void uart_init(void) {
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit data  
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX 
}

int uart_putChar(char c, FILE *stream) {
    if (c == '\n') {
        uart_putChar('\r', stream);
    }
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
	return 0;
}

int uart_getChar(FILE *stream) {
    loop_until_bit_is_set(UCSR0A, RXC0); // Wait until data exists. 
    return UDR0;
}


//////////////////////////////////////////////
// Setting indicators
inline void setIndicator(uint8_t idxDisplayValue, uint8_t idxIndicatorLed, uint8_t R, uint8_t G, uint8_t B) {
	uint8_t nCathode = kDISPVALUE_DIGITCOUNT + (idxIndicatorLed>>1);
	aintDisplaySegments[idxDisplayValue][nCathode] = 
			(R ? aintIndicatorSegmentR[idxIndicatorLed % 2] : 0)
		| 
			(G ? aintIndicatorSegmentG[idxIndicatorLed % 2] : 0)
		| 
			(B ? aintIndicatorSegmentB[idxIndicatorLed % 2] : 0)
	;
}



//////////////////////////////////////////////
// Bluetooth update
inline void doBthUpdate() {
	static char strValue[10];
	fputs("@", stdout);
	fputs(((nCountUp)?"+":"-"), stdout);
	itoa(nTimerMinutes, strValue, 10);
	fputs(strValue, stdout);
	fputs(":", stdout);
	uint8_t nSecs = nTimerSeconds % 60;
	itoa(nSecs, strValue, 10);
	if (nSecs<10) { fputs("0", stdout); }
	fputs(strValue, stdout);
	for (int i=0; i<kADC_COUNT; i++) {
		fputs(" ADC", stdout);
		itoa(i, strValue, 10);
		fputs(strValue, stdout);
		fputs(":", stdout);
		itoa(anLastDisplayedValue[i], strValue, 10);
		fputs(strValue, stdout);
	}
	puts("");
}

//////////////////////////////////////////////
// Main

int main(void) {

	sr.setOutput(0);
	//~ sr.setOeDisableDuringLoad(1);

	//~ _delay_ms(1000);							// Wait a bit for bluetooth module to power up
	// Set up the bluetooth

	/* Set up the serial comm */
	uart_out = fdevopen(uart_putChar, uart_getChar);
	stdout = stdin = uart_out;
    uart_init();
	
#ifdef BTH_USE_POWER
	BTH_POWER_DDR  	|= BTH_POWER_PIN;
	//~ BTH_POWER_PORT 	|= 0;							// Turn off everything on that port
#endif 
#	ifdef BTH_USE_PINKEY	
	BTH_PINKEY_DDR 	|= BTH_PINKEY_PIN;				// Make the key pin output
	BTH_PINKEY_PORT &= ~(BTH_PINKEY_PIN);			// Set the key pin low
	//~ BTH_PINKEY_PORT |= BTH_PINKEY_PIN;			// Set the key pin high
#	endif	
	//~ _delay_ms(1500);							// Wait a bit for everything to clear
#ifdef BTH_USE_POWER
	BTH_POWER_PORT 	|= BTH_POWER_PIN;				// Power up the bluetooth module
#endif
	if (0) {
		// Set the name of the device
		fputs ("AT+NAMETemp-o-matic", stdout);
		_delay_ms(1500);
		// Set device password
		fputs ("AT+PIN1234", stdout);
		_delay_ms(1500);
	}		
#	ifdef BTH_USE_PINKEY	
	BTH_PINKEY_PORT |= BTH_PINKEY_PIN;			// Set the key pin high
#endif
#	ifdef BTH_USE_PINKEY	
	_delay_ms(1500);
	//~ BTH_PINKEY_PORT &= ~(BTH_PINKEY_PIN);	// Lower the key line to make bluetooth transparent
#endif
	

	// Run at 8mhz
#ifdef SPEEDUP8X
	CLKPR = 1<<CLKPCE;
	CLKPR = 0;
#else	
	CLKPR = 1<<CLKPCE;
	CLKPR = 3;
#endif
	
	DDRD = 0xFF;
	
	//~ sr.writeByte(1, 0xFF);	// Enable all common cathodes
	//~ sr.writeByte(0, 0xFF);	// Nothing is on
	//~ sr.setOutput(1);
	
	while (0) {
		sr.writeByte(1, 1<<3);	// Enable all common cathodes
		//~ _delay_ms(250);
		//~ sr.writeByte(0, 0x7E);	// Nothing is on
		sr.setOutput(1);
	}
	

	/* 	Hi-res timebase - using timer 0
		Used for display PWM - 
		Using 8 bit timer because this update happens fast, 
		the timer does not need to count very high
	*/
	TCNT0 = 0;										// 	Initial counter value
	TCCR0A =(1<<WGM01);								// 	CTC (Clear on capture = comparison) mode
	//~ TCCR0B = (1<<CS02) | (0<<CS01) | (1<<CS00);		// Prescaler = 1024
	//~ TCCR0B = (1<<CS02) | (0<<CS01) | (0<<CS00);		// Prescaler = 256
	//~ TCCR0B = (1<CS02) | (0<<CS01) | (1<<CS00);		// Prescaler = 8
	TCCR0B = (0<CS02) | (1<<CS01) | (1<<CS00);		// Prescaler = 8
	//~ OCR0A = F_CPU/1024/1000;						// 	Refresh every second
	OCR0A = 0x40;										// 	About 1000 times per second (1Khz)
	//~ OCR0A = 0xFF;										// 	About 1000 times per second (1Khz)
	TIMSK0 |= (1<<OCIE0A);								//	Enable interrupts on compare match A

	// Set up the outputs
	// ... Initialize everything to zero
	for (int i = 0; i<kDISPVALUE_COUNT+1; i++) {
		for (int j = 0; j<kDISPVALUE_CATHODECOUNT; j++) {
			aintDisplaySegments[i][j] = 0;
		}
	}
	
	// Set the indicators
	
	for (int i = 0; i<kDISPVALUE_COUNT; i++) {
		setIndicator(i, i, 0, 1, 0);
	}

	// Set up the inputs
	// ... Initialize reading direction
	INPUTS_DIR &= ~(INPUT_ALL);
	// ... Initialize pullup resistors
	INPUTS_PORT |=  INPUT_ALL;
	// ... Initialize the debouncing structure
	idxKeyState = 0;
	for (int i=0; i<kMAX_KEYBOUNCE_CHECKS; i++) {
		aintDebounceState[i] = INPUT_ALL;				// Inputs are high by default, 
														// so set them up to be high
	}
	intKeyState = INPUT_ALL;
	
	/* 	Low res timebase - using timer 1
		Used for 
			cycling between readings 
			reading keys 
	*/
	TCNT1  = 0;            				// 	Initial counter value
	TCCR1A =0x00;						// 	Not connected to any pin, normal operation
	/* 	Prescaler = 1024
		CTC (Clear on capture = comparison) mode */
	TCCR1B = 0
		| (0<<ICNC1) 					// ICNC1
		| (0<<ICES1)					// ICES1
										// -
		| (0<<WGM13)					// WGM13
		| (1<<WGM12)					// WGM12
		| (1<<CS12)						// CS12
		| (0<<CS11)						// CS11
		| (1<<CS10)						// CS10
		;								
	TIMSK1 |= (1<<OCIE1A);				//	Enable timer interrupts
	/*
		The F_CPU / 175680 OCR1A setting 
		[ 5 on 1Mhz, 45 on 8Mhz, and so on]
		yields 171 Hz timer hits 
		If we are doing 8 debounce checks, 
		then the debouncing will occur within 50ms
	*/
	OCR1A = F_CPU / TIMER1_OCR1A_FPU_DIV;
		
	// Start the ADC
	//ADMUXbase = (1<<REFS0);	// ADMUX bits
	ADMUXbase = 0			// ADMUX bits
		|(0<<REFS1)			//	REFS1
		|(1<<REFS0)			//	REFS0
							//	ADLAR	left-adjust
							//	Reserved
							//	MUX3
							//	MUX2
							//	MUX1
							//	MUX0
		;
	ADMUX = ADMUXbase
		|(0<<MUX3)
		|(0<<MUX2)
		|(0<<MUX1)
		|(0<<MUX0)
		;					// Start with ADC 0;
	ADCSRA = 0				// ADSRA bits
		|(1<<ADEN)			// ADEN: ADC Enabled
							// ADSC: Start
		|(0<<ADATE)			// ADATE: Auto-trigger
							// ADIF: Conversion ready signal
		|(1<<ADIE)			// ADIE: Interrupt enabled
		|(1<<ADPS2)			// ADPS2: Prescaler bit
		|(0<<ADPS1)			// ADPS1: Prescaler bit
		|(1<<ADPS0)			// ADPS0: Prescaler bit
		;
	ADCSRB = 0;			// ADCSRB is irrelevant when ADATE is not set
	DIDR0 = 0xFF;			// Digital input buffer disable
	PRR &= ~(1<<PRADC);		// Turn off ADC power disable
	// Turn off digital input on the ADC pins
	for (uint8_t i = 0; i<kADC_COUNT; i++) {
		DIDR0 |= 1<<(ADC0D+i);
	}
	// Initialize the ADC
	for (uint8_t i = 0; i<kADC_COUNT; i++) {
		setDisplayValue(aidxADC2DispValue[i], kDISPVALUE_NOVALUEAVAILABLE);
		anSampleCount[i] = 0;
	}
	idxADCValue = 0;					// Initialize the index
	//Start converting
	ADCSRA = 	
		 (1<<ADEN)			// ADEN: ADC Enabled
		|(1<<ADSC) 			// ADSC: Start
		|(0<<ADATE)			// ADATE: Auto-trigger
							// ADIF: Conversion ready signal
		|(1<<ADIE)			// ADIE: Interrupt enabled
		|(1<<ADPS2)			// ADPS2: Prescaler bit
		|(1<<ADPS1)			// ADPS1: Prescaler bit
		|(1<<ADPS0)			// ADPS0: Prescaler bit
		;
	//~ ADCSRA |= (1<<ADSC) ;	
	
	sei();								//	Start interrupt handling
	
	uint16_t nDisplayValue = 0; 
	uint16_t nDispCounter = 0;
	
	// Turn off the speaker
	beepTurnOff();
	
	beepG_charge();
	
	//~ _delay_ms(1000);
	//~ uint8_t anData[2];
	//~ anData[0] = aintIndicatorSegmentB[1]|aintIndicatorSegmentB[0];
	//~ anData[1] = 0x40;
	//~ sr.writeData(0, 2, anData);
	//~ sr.setOutput(1);
	
	//~ while(1) {
	//~ }
	//~ while (0) {
		//~ char strNumber[10];
		//~ puts("AT+NAMEtempar");
		//~ fputs ("AT+NAMEtempar", stdout)	;
		//~ puts ("Hello, world");
		//~ _delay_ms(3000);
		//~ nDisplayValue++;
	//~ }
	
	while (0) {
		// Just count up

		setDisplayValue(0, nDisplayValue);
		_delay_ms(250);
		if ( ++nDisplayValue > 999) {
			nDisplayValue = 0;
		}
		if (++nDispCounter > 20) {
			fputs ("Hello, world", stdout)	;
			nDispCounter=0;
		}
		//~ sr.setOutput(1);
		//~ while(1) {
			//~ for (int i= 0; i<8; i++) {
				//~ sr.write_byte(0, 1<<i);
				//~ sr.write_byte(1, 0xFF);
				//~ sr.writeByte(1, 1<<i);
				//~ DO_SHORT_DELAY;
				//~ sr.toggleOutput();
			//~ }
		//~ }
	}
	
	#define MAX_CMD_LENGTH	20
	char strCommand[MAX_CMD_LENGTH+1];
	uint8_t idxChar = 0;
	while (1) {
		if (nBthUpdateCount > KEYTIMER_MAX_BTHUPDATECOUNT) {
			nBthUpdateCount = 0;
			doBthUpdate();
		}
#		ifdef READ_BTH			
		if (UCSR0A & (1<<RXC0)) {
			// Read serial character
			char charRead = UDR0;
		
			if ( (charRead == '\r') || (charRead == '\n') ) {
				// End of command
				strCommand[idxChar] = 0x00;
				fputs ("You commanded ", stdout);
				puts (strCommand);
				idxChar = 0;
			} else {
				if (idxChar < MAX_CMD_LENGTH) {
					strCommand[idxChar++] = charRead;
				} else {
					// Command too long, discard the character
				}
			}
		}
#		endif /* READ_BTH */
		
		//~ _delay_ms(300);		
	}
	return 0;
}



//////////////////////////////////////////////
// Interrupt routine for servicing ADC

ISR(ADC_vect)
{
	uint8_t intADCLo = ADCL;
	uint8_t intADCHi = ADCH;
	uint16_t intADCfullValue = ((intADCHi << 8) + intADCLo);

	// Update the samples
	anADCRead[idxADCValue] += intADCfullValue;
	if (++anSampleCount[idxADCValue] >= kSAMPLE_COUNT) {
		uint32_t nNewValue = anADCRead[idxADCValue] >> kDECIMATE_RIGHTSHIFT;
		anLastDisplayedValue[idxADCValue] = tempFromADC(nNewValue) + aintTempAdjust[idxADCValue];
		setDisplayValue(aidxADC2DispValue[idxADCValue], anLastDisplayedValue[idxADCValue]);
		anADCRead[idxADCValue] = 0;
		anSampleCount[idxADCValue] = 0;
	}
	
	//~ // Start the next conversion
	if (++idxADCValue >= kADC_COUNT) { idxADCValue = 0; }
	ADMUX = ADMUXbase | idxADCValue;
	ADCSRA |= (1<<ADSC);	//Start converting
} 

//////////////////////////////////////////////
// Interrupt routine for servicing LED refreshment

ISR(TIMER0_COMPA_vect) {
	static uint8_t idxActiveCathode = 0;
	
#	ifdef PARALLEL_595
	sr.setOutput(0);
#	endif PARALLEL_595
	uint8_t anData[2];
	// anData[0] is the anodes
	anData[0] = aintDisplaySegments[idxDisplayValue][idxActiveCathode];
	uint8_t nCathodeVal; 
	switch(idxActiveCathode) {
		case 0: 	{	(nCathodeVal) = 1<<1; break;		}		// 7SEG 0
		case 1: 	{	(nCathodeVal) = 1<<2; break;		}		// 7SEG 1
		case 2: 	{	(nCathodeVal) = 1<<3; break;		}		// 7SEG 2
		case 3: 	{	(nCathodeVal) = 1<<5; break;		}		// INDIC1&2
		case 4: 	{	(nCathodeVal) = 1<<4; break;		}		// INDIC3&4
		case 5: 	{	(nCathodeVal) = 1<<6; break;		}		// INDIC5&6
		default:	{	(nCathodeVal) = 0;					}
	}
	// anData[0] is the cathodes
	anData[1] = nCathodeVal;
	sr.writeData(0, 2, anData);
	sr.setOutput(1);
	
	if (++idxActiveCathode >= kDISPVALUE_CATHODECOUNT) {
		idxActiveCathode = 0;
	}
	
	return;
}

//////////////////////////////////////////////
// Regimes


class CRegime {
	public:
		virtual void encoderLeft() 		{ return;}
		virtual void encoderRight() 	{ return; }
		virtual void encoderPress() 	{ return; }
		virtual void encoderLongPress() { return; }
		virtual void start();
};

class CDisplayValuesRegime: public CRegime {
	public:
		virtual void encoderLeft();
		virtual void encoderRight();
		virtual void encoderPress();
		virtual void encoderLongPress();
} regimeDisplayValues;

class CSetTimerRegime: public CRegime {
	public:
		virtual void encoderLeft();
		virtual void encoderRight();
		virtual void encoderPress();
		virtual void encoderLongPress();
} regimeSetTimer;

class CTestRegime : public CRegime {
	public:
		virtual void encoderLeft();
		virtual void encoderRight();
		virtual void encoderPress();
		virtual void start();
		//~ virtual void encoderLongPress();
	protected:
		void updateDisplay();
		uint8_t m_nCathode;
		uint8_t m_nAnode;
} regimeTest;

CRegime * volatile ptrCurrentRegime = &regimeDisplayValues;

//////////////////////////////////////////////

void CRegime::start() {
	ptrCurrentRegime = this;
};

//////////////////////////////////////////////

void CDisplayValuesRegime::encoderLeft() {
	// Next value
	if ( ++idxDisplayValue >= (kDISPVALUE_COUNT) ) {
		idxDisplayValue = 0;
	}
	// 2. reset the value display timer
	nValueCycleCount = 0;	
}

void CDisplayValuesRegime::encoderRight() {
	// Go to the previous value
	if (idxDisplayValue==0) {
		idxDisplayValue = kDISPVALUE_COUNT - 1; 
	} else {
		--idxDisplayValue;
	}
	// 2. reset the value display timer
	nValueCycleCount = 0;
}

void CDisplayValuesRegime::encoderPress() {
	nStopValueCycling = nStopValueCycling ^ 0x01;
	if (nStopValueCycling) {
		// Set all indicators to red
		for (int idxVal = 0; idxVal<kDISPVALUE_COUNT; idxVal++) {
			setIndicator(idxVal, idxVal, 1, 0, 0);
		}
	} else {
		// Set all indicators to green
		for (int idxVal = 0; idxVal<kDISPVALUE_COUNT; idxVal++) {
			setIndicator(idxVal, idxVal, 0, 1, 0);
		}
		
		// Go to the next value
		nValueCycleCount = 0;
		if ( (++idxDisplayValue) >= (kDISPVALUE_COUNT) ) {
			idxDisplayValue = 0;
		}
	}
}

void CDisplayValuesRegime::encoderLongPress() {
	regimeSetTimer.start();
	nRegime = kREGIME_SETTIMER;				// Change the regime
	beepF_setTimer();						// Audio indicator
	setDisplayValue(kIDXDISPVALUE_SETTING, 0);	// Initialize the value
	idxDisplayValue = kIDXDISPVALUE_SETTING;		// Make the leds display this value
	setIndicator(kIDXDISPVALUE_SETTING, kIDXDISPVALUE_TIMER, 0, 0, 1);	// Turn on blue led
}

//////////////////////////////////////////////

void CSetTimerRegime::encoderLeft() {
	// Go to the previous value
	if (getDisplayValue(kIDXDISPVALUE_SETTING) > 0) {
		setDisplayValue(kIDXDISPVALUE_SETTING, getDisplayValue(kIDXDISPVALUE_SETTING) - 1);
	} else {
		setDisplayValue(kIDXDISPVALUE_SETTING, COUNTDOWNTIMER_MAXMINUTES);
	}	
}

void CSetTimerRegime::encoderRight() {
	if (getDisplayValue(kIDXDISPVALUE_SETTING) < COUNTDOWNTIMER_MAXMINUTES) {
		setDisplayValue(kIDXDISPVALUE_SETTING, getDisplayValue(kIDXDISPVALUE_SETTING) + 1);
	} else {
		setDisplayValue(kIDXDISPVALUE_SETTING, 0);
	}
}

void CSetTimerRegime::encoderLongPress() {
	beepH_vlesurodilas();
	regimeTest.start();
	nRegime = kREGIME_TESTLEDS;
}

void CSetTimerRegime::encoderPress() {
	if (getDisplayValue(kIDXDISPVALUE_SETTING) > 0) {
		// Set up the countdown timer
		nCountUp = 0;
		nTimingCount  = 
				getDisplayValue(kIDXDISPVALUE_SETTING)			// Minutes
			*	60												// Seconds
			*	TIMER1_OCR1A_FPU_DIV							// Cycle requency
			/	1024											// Prescaler
			;
	} else {
		// Nothing to do
		nCountUp = 1;
		nTimingCount = 0;
	}
	regimeDisplayValues.start();
	nRegime = kREGIME_DISPLAYVALUES;		// Change the regime
	beepG_finishSetTimer();						// Audio indicator
}


//////////////////////////////////////////////

void CTestRegime::encoderPress() {
	// Short encoder button press
	// Exit test mode
	regimeDisplayValues.start();
	nRegime = kREGIME_DISPLAYVALUES;		// Change the regime
	beepI_vlesuonarosla();						// Audio indicator
}

void CTestRegime::updateDisplay() {
	// Zero out everything
	for (int i=0; i<kINDICATOR_COUNT; i++) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			setIndicator(kIDXDISPVALUE_SETTING, i, 0, 0, 0);	// Turn on all leds
		}
	}
	for (int idxDigit=0; idxDigit<kDISPVALUE_DIGITCOUNT; idxDigit++) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			aintDisplaySegments[kIDXDISPVALUE_SETTING][idxDigit] = 0x00;
		}
	}
	if (m_nCathode==0) {
		// Turn everything on
		for (int i=0; i<kINDICATOR_COUNT; i++) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				setIndicator(kIDXDISPVALUE_SETTING, i, 1, 1, 1);	// Turn on all leds
			}
		}
		for (int idxDigit=0; idxDigit<kDISPVALUE_DIGITCOUNT; idxDigit++) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				aintDisplaySegments[kIDXDISPVALUE_SETTING][idxDigit] = 0xFF;
			}
		}
	} else {
		aintDisplaySegments[kIDXDISPVALUE_SETTING][m_nCathode-1] = 1<<m_nAnode;
	}	
}

void CTestRegime::start() {
	CRegime::start();
	
	m_nCathode = 0;
	m_nAnode = 0;
	idxDisplayValue = kIDXDISPVALUE_SETTING;		// Make the leds display this value
	
	updateDisplay();
	
}

void CTestRegime::encoderRight() {
	if (m_nCathode == 0) {
		m_nCathode++;
	} else {
		if (m_nAnode<0x7) {
			m_nAnode++;
		} else {
			m_nAnode = 0;
			if (++m_nCathode > kDISPVALUE_CATHODECOUNT) {
				m_nCathode = 0;
			}
		}
	}
	updateDisplay();
}

void CTestRegime::encoderLeft() {
	if (m_nCathode == 0) {
		m_nCathode = kDISPVALUE_CATHODECOUNT;
		m_nAnode = 7;
	} else {
		if (m_nAnode>0) {
			m_nAnode--;
		} else {
			m_nAnode = 7;
			m_nCathode--;
		}
	}
	updateDisplay();	
}


//////////////////////////////////////////////

ISR(TIMER1_COMPA_vect) {
/*
	Services display update and key reading
*/	
	// Process previousely remembered longpress
	if (nKeyPressCycleCount) {
		if (++nKeyPressCycleCount > kLONGPRESS_MAX_CYCLE_COUNT) {
			if ((intKeyState & INPUT_ENCODERBTN) == 0) {
				nIgnoreKeyRelease = 1;
				ptrCurrentRegime->encoderLongPress();
			}
			nKeyPressCycleCount = 0;
		}
	}
	
	// Move on in ignoring encoder in the opposite direction
	if (nEncoderRotationIgnoreTickCount) {
		nEncoderRotationIgnoreTickCount--;
	}
	
	// Do reading of keys here
	aintDebounceState[idxKeyState] = INPUTS_PIN & INPUT_ALL;
	if (++idxKeyState >= kMAX_KEYBOUNCE_CHECKS) {
		idxKeyState=0;
	}
	
	// Debounce data
	uint8_t i, intAccAND, intAccOR;
	intAccAND = 0xFF;
	intAccOR = 0x00;
	for (i=0; i<kMAX_KEYBOUNCE_CHECKS; i++) {
		intAccAND &= aintDebounceState[i];
		intAccOR  |= aintDebounceState[i];
	}
	uint8_t intNewKeyState = intKeyState;
	uint8_t nDeboundedBitMask = ~(intAccAND ^ intAccOR); 	// The bits intAccAND and intAccOR agree on are deboundeced
	intNewKeyState &= ~nDeboundedBitMask; 					// Clear the bits that are debounced
	intNewKeyState |= (intAccAND & nDeboundedBitMask); 		// Set the debounced bits
	uint8_t nChangedBits = intNewKeyState ^ intKeyState;
	if (nChangedBits) {
		// Key bits changed
		
		if ((nChangedBits & intNewKeyState) == 0) {
			// This is a press event
			nKeyPressCycleCount = 1;				// Initiate the long press counting
			
		} else {
			// This is a release event
			nKeyPressCycleCount = 0;				// Clear the long press counting
		}
	}
	
	// Process keys by regime
	if (nChangedBits) {
		if ((nChangedBits & intNewKeyState) == 0) {
			// This is a press event
			if ((nChangedBits & INPUT_ENCODERLEFT) && !nEncoderRotationIgnoreTickCount) {
				ptrCurrentRegime->encoderLeft();
				nEncoderRotationIgnoreTickCount = INPUT_IGNORE_ENCODER_OPPDIR_TICK_COUNT;
			} else if ((nChangedBits & INPUT_ENCODERRIGHT) && !nEncoderRotationIgnoreTickCount) {
				ptrCurrentRegime->encoderRight();
				nEncoderRotationIgnoreTickCount = INPUT_IGNORE_ENCODER_OPPDIR_TICK_COUNT;
			}
			// Kill sounds()
			beepStop();
		} else {
			// This is a release event
			if (nIgnoreKeyRelease) {
				// Ignoring release event
				nIgnoreKeyRelease = 0;
			} else {
				// Unignored release event
				if (nChangedBits & INPUT_ENCODERBTN) {
					ptrCurrentRegime->encoderPress();
				} // else encoder roation release does not matter
			}
		}
	}
	
	if (nChangedBits) {
		// Key bits changed: save key values
		intKeyState = intNewKeyState;
	}
	
	if (nRegime==kREGIME_DISPLAYVALUES) {
		// Do display updating
		if (nStopValueCycling == 0) {
			if (nKeyPressCycleCount==0) { // Do not cycle while something is pressed
				if (++nValueCycleCount >= KEYTIMER_MAX_VALUECYCLE) {
					nValueCycleCount = 0;
					if ( ++idxDisplayValue >= (kDISPVALUE_COUNT) ) {
						idxDisplayValue = 0;
					}
				}
			}
		}
	}
	
	// Update time elapsed
	if (nCountUp) { 
		nTimingCount++;
	} else {
		if (--nTimingCount == 0) {
			// Timer expired
			beepE_timerexpired();
			nCountUp = 1;
		}
	}
	
	uint32_t nCountTimesPrescale = nTimingCount  * 1024;
	uint32_t nTimebase = 
			nCountTimesPrescale
		*	16		 
		/ 	TIMER1_OCR1A_FPU_DIV
		;
	nCountTimesPrescale = nCountTimesPrescale / TIMER1_OCR1A_FPU_DIV;
	uint32_t nNewMinuteTimerDot = nTimebase / 2 % 2;
	nTimerSeconds = nCountTimesPrescale;
	nTimerMinutes = nTimerSeconds / 60;
	
	if (nNewMinuteTimerDot != nMinuteTimerDot) {
		nMinuteTimerDot = nNewMinuteTimerDot;
		// Update the minute timer display value
		nDisplayTimerValue = 
				nCountTimesPrescale
			/ 	60 					/* 60 seconds per minute */
			;
		uint8_t idxLEDdot = 0;
		if (nDisplayTimerValue < 10) {
			// Display seconds, blink dot on the first led
			nDisplayTimerValue = 
					nDisplayTimerValue * 100			// Minutes
				+ 	nCountTimesPrescale % 60;			// Seconds
			idxLEDdot = 2;
		} else {
			// Display minutes only, blink last dot 
			// Nothing to do here
		}
		setDisplayValue(kIDXDISPVALUE_TIMER, nDisplayTimerValue);
		
		// Blink the dot
		if (nMinuteTimerDot) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				aintDisplaySegments[kIDXDISPVALUE_TIMER][idxLEDdot] |= LEDSEG_DP;
			}
		} else {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				aintDisplaySegments[kIDXDISPVALUE_TIMER][idxLEDdot] &= (~LEDSEG_DP);
			}
		}
	}
	
	// Update beeping
	if (nTimebase % 2) {
		beepProcessing();
	}
	
	// Update Bluetooth timing
	nBthUpdateCount++;
}


//~ ISR(RESET) {
	//~ nResetting = 0xFF;
	//~ // Shut down the display
	//~ for (int i=0;i<kDISPVALUE_COUNT;i++) 
		//~ for (int j=0;j<kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT;j++) 
			//~ aintDisplaySegments[i][j] = 0;
//~ }
