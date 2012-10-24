#define SPEEDUP8X
//~ #undef SPEEDUP8X
#ifdef SPEEDUP8X
#define F_CPU 8000000UL /* 1 MHz Internal Oscillator */
#else
#define F_CPU 1000000UL /* 1 MHz Internal Oscillator */
#endif
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include "sr595.h"
//~ #include <math.h>

#define LEDSEG_A	(1<<5)	
#define LEDSEG_B	(1<<4)	
#define LEDSEG_C	(1<<2)	
#define LEDSEG_DP	(1<<3)	
#define LEDSEG_D	(1<<1)	
#define LEDSEG_E	(1<<0)	
#define LEDSEG_F	(1<<6)	
#define LEDSEG_G	(1<<7)	

#define IND0_R		LEDSEG_D	
#define IND0_G		LEDSEG_E	
#define IND0_B		LEDSEG_F	
#define IND1_R		LEDSEG_A	
#define IND1_G		LEDSEG_B	
#define IND1_B		LEDSEG_C	
uint8_t	aintIndicatorSegmentR[2] = {IND0_R, IND1_R};
uint8_t	aintIndicatorSegmentG[2] = {IND0_G, IND1_G};
uint8_t	aintIndicatorSegmentB[2] = {IND0_B, IND1_B};

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
#define LED7OUT_B		LEDSEG_A | LEDSEG_F | LEDSEG_G | LEDSEG_E | LEDSEG_C | LEDSEG_D
#define LED7OUT_C		LEDSEG_A | LEDSEG_F | LEDSEG_E | LEDSEG_D
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
#define SR74XX595_OE	03
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
		SR74XX595_DS, 		// nDS
		SR74XX595_SHCP, 	// nSHCP
		STCP				// anSTCP
		);


/////////////////////////////////////////////////////////////////////
// Keys control
#define TIMER1_OCR1A_FPU_DIV	175680

#define kMAX_KEYBOUNCE_CHECKS	8
#define INPUTS_DIR		DDRB
#define INPUTS_PORT		PORTB
#define INPUTS_PIN		PINB
#define	INPUT_BTNRIGHT		(01<<02)
#define	INPUT_BTNLEFT		(01<<04)
#define	INPUT_ENCODERLEFT	(01<<06)
#define	INPUT_ENCODERRIGHT	(01<<07)
#define	INPUT_ENCODERBTN	(01<<05)
#define INPUT_ALL		(INPUT_BTNRIGHT | INPUT_BTNLEFT | INPUT_ENCODERLEFT | INPUT_ENCODERRIGHT | INPUT_ENCODERBTN)
			uint8_t		aintDebounceState[kMAX_KEYBOUNCE_CHECKS];
			uint8_t		intKeyState;
			uint8_t		idxKeyState;
			uint8_t		nStopValueCycling;
			
/////////////////////////////////////////////////////////////////////
// Other low-res timer stuff

// Value cycling
volatile	uint16_t	nValueCycleCount;							// Count of for the purposes of value cycling
#define KEYTIMER_MAX_VALUECYCLE	258									// Update the display every 1.5 seconds

volatile	uint32_t	nTimingCount;								// Count for the purposes of displaying a timer
			uint16_t	nDisplayMinuteTimer; 
			uint8_t		nMinuteTimerDot; 

/////////////////////////////////////////////////////////////////////
// Led control


#define kDISPVALUE_COUNT				6
#define kIDXDISPVALUE_TIMER				0
#define kDISPVALUE_NOVALUEAVAILABLE		0xFFFF
#define kDISPVALUE_DIGITCOUNT			3
#define kDISPVALUE_INDPAIRSCOUNT		3
#define kDISPVALUE_CATHODECOUNT			(kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT)

volatile	uint16_t	aintDisplayValue[kDISPVALUE_COUNT];  				// 	Value to be displayed in the LEDs
volatile	uint8_t		idxDisplayValue = 0;
volatile	uint8_t		aintDisplaySegments[kDISPVALUE_COUNT][kDISPVALUE_CATHODECOUNT];  // 	Value to be displayed on each led
volatile	uint8_t		nResetting;
volatile	uint8_t 	idxActiveCathode = 0;


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
// Beeps
#define kBEEP_CONTROL_MAXENTRIES	5
#define kSPEAKER_PORT				PORTB
#define kSPEAKER_DDR				DDRB
#define kSPEAKER_PIN				3

typedef struct {
	uint16_t	nFreq;
	uint16_t	nAudibleCount;
	uint16_t	nSilentCount;
} beepStruct;
	beepStruct	anBeepControl[kBEEP_CONTROL_MAXENTRIES];
	uint8_t		idxBeepControl = 0xFF;
	uint8_t		nBeepControlCount;
	uint16_t	nBeepTimer;
	uint8_t		nBeepAudible;
	uint8_t 	nBeginBeep;


inline void beepTurnOff() {
	kSPEAKER_DDR &= ~(1<<kSPEAKER_PIN);			/* turn off the OC PIN */
	kSPEAKER_PORT |= 1<<kSPEAKER_PIN;			/* set this pin high */
	TCCR2A &= ~((1<<COM2A1) | (1<<COM2A0));		/* Normal operation: OC pin disconnected */
}	

void beepProcessing() {
	if ( nBeginBeep || (nBeepControlCount && (idxBeepControl != 0xFF)) ) {
		// We are beeping
		
		uint8_t nDoNextBeepStructure = 0;
		if (nBeginBeep) {
			idxBeepControl = 0;
			nBeepTimer = 0;
			nDoNextBeepStructure = 1;
			nBeginBeep = 0;
		} else {
			if (nBeepAudible) {
				if (++nBeepTimer > anBeepControl[idxBeepControl].nAudibleCount) {
					if (anBeepControl[idxBeepControl].nSilentCount > 0) {
						// Do the silent part of the beep
						beepTurnOff();
						nBeepTimer = 0;
					} else {
						// Proceed to next beep structure
						nDoNextBeepStructure = 1;
						idxBeepControl++;
					}
				} // else: still doing audible
			} else { // Silent part
				if (++nBeepTimer > anBeepControl[idxBeepControl].nSilentCount) {
					// Proceed to next beep structure
					nDoNextBeepStructure = 1;
					idxBeepControl++;
				} // else: still doing silent
			}
		}
		
		if (nDoNextBeepStructure) {
			nBeepTimer = 0;
			if (idxBeepControl < nBeepControlCount) {
				nBeepAudible = 1;
				TCNT2  = 0;            				// 	Initial counter value
				/* CTC mode, toggle the output pin */
				TCCR2A = 0
					| 0<<COM2A1	// COM2A1
					| 1<<COM2A0 // COM2A0
					| 0<<COM2B1 // COM2B1
					| 0<<COM2B0 // COM2B0
								// -
								// – 
					| 1<<WGM21	// WGM21
					| 0<<WGM20	// WGM20
					;
				TCCR2B = 
					/* Prescaler = 1025	*/
					0
					| 0<<FOC2A 	// FOC2A 
					| 0<<FOC2B 	// FOC2B
								// – 
								// – 
					| 0<<WGM22 	// WGM22
					;
				// Set the prescaler
				uint16_t nPrescaler; 
				switch (F_CPU) {
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
				OCR2A = F_CPU / nPrescaler / anBeepControl[idxBeepControl].nFreq;
				// Connect the OC pin
				kSPEAKER_DDR |= 1<<kSPEAKER_PIN;			/* Connect the OC PIN */
			} else {
				// Done beeping
				idxBeepControl = 0xFF;
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
	nBeepControlCount = 1;
	nBeginBeep = 1;
}

void beepB() {
	anBeepControl[0].nFreq = 440;
	anBeepControl[0].nAudibleCount = 16;
	anBeepControl[0].nSilentCount = 0;
	
	anBeepControl[1].nFreq = 554;
	anBeepControl[1].nAudibleCount = 16;
	anBeepControl[1].nSilentCount = 0;
	
	anBeepControl[2].nFreq = 659;
	anBeepControl[2].nAudibleCount = 16;
	anBeepControl[2].nSilentCount = 0;
	
	anBeepControl[3].nFreq = 880;
	anBeepControl[3].nAudibleCount = 32;
	anBeepControl[3].nSilentCount = 0;
	
	nBeepControlCount = 4;
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
// Main
void setIndicator(uint8_t idxDisplayValue, uint8_t idxIndicatorLed, uint8_t R, uint8_t G, uint8_t B) {
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
	OCR0A = 0x20;										// 	About 1000 times per second (1Khz)
	//~ OCR0A = 0xFF;										// 	About 1000 times per second (1Khz)
	TIMSK0 |= (1<<OCIE0A);								//	Enable interrupts on compare match A

	// Set up the outputs
	// ... Initialize everything to zero
	for (int i = 0; i<kDISPVALUE_COUNT; i++) {
		for (int j = 0; j<kDISPVALUE_CATHODECOUNT; j++) {
			aintDisplaySegments[i][j] = 0;
		}
	}
	
	// Set the indicators
	
	for (int idxDisplayValue = 0; idxDisplayValue<kDISPVALUE_COUNT; idxDisplayValue++) {
		setIndicator(idxDisplayValue, idxDisplayValue, 0, 1, 0);
	}
	
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
	
	sei();								//	Start interrupt handling
	
	uint16_t nDisplayValue = 0; 
	uint16_t nDispCounter = 0;
	
    if (kDISPVALUE_COUNT > 0) { setDisplayValue(0, 123);	}
    if (kDISPVALUE_COUNT > 1) { setDisplayValue(1, 216);	}
    if (kDISPVALUE_COUNT > 2) { setDisplayValue(2, 40);	}
    if (kDISPVALUE_COUNT > 3) { setDisplayValue(3, 255);	}
    if (kDISPVALUE_COUNT > 4) { setDisplayValue(4, 128);	}
    if (kDISPVALUE_COUNT > 5) { setDisplayValue(5, 101); 	}
	
	// Turn off the speaker
	beepTurnOff();
	
	//~ _delay_ms(1000);
	//~ uint8_t anData[2];
	//~ anData[0] = aintIndicatorSegmentB[1]|aintIndicatorSegmentB[0];
	//~ anData[1] = 0x40;
	//~ sr.writeData(0, 2, anData);
	//~ sr.setOutput(1);
	
	
	//~ while(1) {
	//~ }
	while (0) {
		char strNumber[10];
		puts("AT+NAMEtempar");
		fputs ("AT+NAMEtempar", stdout)	;
		puts ("Hello, world");
		_delay_ms(3000);
		nDisplayValue++;
	}
	
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
	while (1) {
	}
		
}


// Interrupt routine for servicing LED refreshment
ISR(TIMER0_COMPA_vect) {
	//~ static uint8_t idxActiveCathode = 1;
	
#	ifdef PARALLEL_595
	sr.setOutput(0);
#	endif PARALLEL_595
	uint8_t anData[2];
	// anData[0] is the anodes
	anData[0] = aintDisplaySegments[idxDisplayValue][idxActiveCathode];
	uint8_t nCathodeVal; 
	switch(idxActiveCathode) {
		case 0: 	{	(nCathodeVal) = 1<<1; break;		}
		case 1: 	{	(nCathodeVal) = 1<<2; break;		}
		case 2: 	{	(nCathodeVal) = 1<<3; break;		}
		case 3: 	{	(nCathodeVal) = 1<<6; break;		}
		case 4: 	{	(nCathodeVal) = 1<<5; break;		}
		case 5: 	{	(nCathodeVal) = 1<<7; break;		}
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


ISR(TIMER1_COMPA_vect) {
/*
	Services display update and key reading
*/	
	
	
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
		// Bits changed
		
			uint8_t nGotoNextValue = 0;
			
		if ( (nChangedBits & INPUT_BTNRIGHT) && ((intNewKeyState & INPUT_BTNRIGHT) == 0) ) {
			// Right key pressed
			nGotoNextValue = 1;
			if (++idxActiveCathode >= kDISPVALUE_CATHODECOUNT) {
				idxActiveCathode = 0;
			}
			beepA();
		}
		if ( (nChangedBits & INPUT_BTNLEFT) && ((intNewKeyState & INPUT_BTNLEFT) == 0) ) {
			// Left key pressed
			nStopValueCycling = nStopValueCycling ^ 0x01;
			if (nStopValueCycling) {
				// Set all indicators to red
				for (int idxDisplayValue = 0; idxDisplayValue<kDISPVALUE_COUNT; idxDisplayValue++) {
					setIndicator(idxDisplayValue, idxDisplayValue, 1, 0, 0);
				}
			} else {
				// Set all indicators to green
				for (int idxDisplayValue = 0; idxDisplayValue<kDISPVALUE_COUNT; idxDisplayValue++) {
					setIndicator(idxDisplayValue, idxDisplayValue, 0, 1, 0);
				}
				
				// Go to the next value
				nGotoNextValue = 1;
			}
			beepB();
		}
		
		if (nGotoNextValue) {
			nValueCycleCount = 0;
			if ( ++idxDisplayValue >= (kDISPVALUE_COUNT) ) {
				idxDisplayValue = 0;
			}
		}
		
		intKeyState = intNewKeyState;
	}
	
	// Process display update
	if (nStopValueCycling == 0) {
		if (++nValueCycleCount >= KEYTIMER_MAX_VALUECYCLE) {
			nValueCycleCount = 0;
			if ( ++idxDisplayValue >= (kDISPVALUE_COUNT) ) {
				idxDisplayValue = 0;
			}
		}
	}
	
	//~ // Update displayed time elapsed
	uint32_t nCountTimesPrescale = (++nTimingCount)  * 1024;
	uint32_t nTimebase = 
			nCountTimesPrescale
		*	16		 
		/ 	TIMER1_OCR1A_FPU_DIV
		;
	uint32_t nNewMinuteTimerDot = nTimebase / 2 % 2;
	if (nNewMinuteTimerDot != nMinuteTimerDot) {
		nMinuteTimerDot = nNewMinuteTimerDot;
		// Update the minute timer display value
		nDisplayMinuteTimer = 
				nCountTimesPrescale
			/ 	TIMER1_OCR1A_FPU_DIV
			/ 	60 					/* 60 seconds per minute */
			;
		setDisplayValue(kIDXDISPVALUE_TIMER, nDisplayMinuteTimer);
		
		// Blink the dot
		if (nMinuteTimerDot) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				aintDisplaySegments[kIDXDISPVALUE_TIMER][0] |= LEDSEG_DP;
			}
		} else {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				aintDisplaySegments[kIDXDISPVALUE_TIMER][0] &= (~LEDSEG_DP);
			}
		}
	}
	if (nTimebase % 2) {
		beepProcessing();
	}
}


//~ ISR(RESET) {
	//~ nResetting = 0xFF;
	//~ // Shut down the display
	//~ for (int i=0;i<kDISPVALUE_COUNT;i++) 
		//~ for (int j=0;j<kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT;j++) 
			//~ aintDisplaySegments[i][j] = 0;
//~ }