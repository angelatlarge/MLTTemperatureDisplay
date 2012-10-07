#define F_CPU 1000000UL /* 1 MHz Internal Oscillator */
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
//~ #include <math.h>

#define LEDSEG_A	(1<<5)	
#define LEDSEG_B	(1<<4)	
#define LEDSEG_C	(1<<2)	
#define LEDSEG_DP	(1<<3)	
#define LEDSEG_D	(1<<1)	
#define LEDSEG_E	(1<<0)	
#define LEDSEG_F	(1<<6)	
#define LEDSEG_G	(1<<7)	

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

#define SR74XX595_PORT	PORTD
#define SR74XX595_DS	02
#define SR74XX595_SHCP	05
#define SR74XX595_STCP0	04
#define SR74XX595_OE	03
#define SR74XX595_STCP1	06


class sr595
{
	public:
		sr595(uint8_t nCascadeCount, uint8_t fParallel, volatile uint8_t *ptrPort, uint8_t nOE, uint8_t nDS, uint8_t nSHCP, uint8_t anSTCP[]);
		~sr595();
		void writeByte(uint8_t nIndex, uint8_t nData);				
	protected:
		uint8_t m_nCascadeCount;
		volatile uint8_t *m_ptrPort;
		uint8_t m_nOE;
		uint8_t m_nDS;
		uint8_t m_nSHCP;
		uint8_t *m_anSTCP;
		uint8_t *m_anData;
		uint8_t m_nPortBitMask;
		uint8_t m_fParallel;
		uint8_t m_fOutput;
		void SHCP_LO() 						{	*m_ptrPort &= ~(1<<m_nSHCP);				}
		void SHCP_HI() 						{	*m_ptrPort |=   1<<m_nSHCP;					}
		void DS_LO() 						{	*m_ptrPort &= ~(1<<m_nDS);					}
		void DS_HI() 						{	*m_ptrPort |=   1<<m_nDS;					}
		void STCP_LO(uint8_t nIndex) 		{	*m_ptrPort &= ~(1<<m_anSTCP[nIndex]);		}
		void STCP_HI(uint8_t nIndex) 		{	*m_ptrPort |=   1<<m_anSTCP[nIndex];		}
		void OE_LO() 						{	*m_ptrPort &= ~(1<<m_nOE);					}
		void OE_HI() 						{	*m_ptrPort |=   1<<m_nOE;					}
	public:
		void setOutput(uint8_t nOutput) {
			if (nOutput != m_fOutput) {
				if (m_fOutput = nOutput) {				/* Assign AND TEST */
					OE_LO();
				} else {
					OE_HI();
				}
			}
		}
		void toggleOutput() {
			if (m_fOutput = (m_fOutput != 0) ^ 1) {	
				OE_LO();
			} else {
				OE_HI();
			}
		}
		
};

sr595::sr595(uint8_t nCascadeCount, uint8_t fParallel, volatile uint8_t *ptrPort, uint8_t nOE, uint8_t nDS, uint8_t nSHCP, uint8_t anSTCP[])
{
	m_nCascadeCount	= nCascadeCount;
	m_ptrPort		= ptrPort;
	m_nOE			= nOE;
	m_nDS			= nDS;
	m_nSHCP			= nSHCP;
	m_anSTCP 		= (uint8_t *)malloc(nCascadeCount);
	memcpy(m_anSTCP, anSTCP, nCascadeCount);
	m_fParallel 	= fParallel;
	m_anData 		= (uint8_t *)malloc(nCascadeCount);
	
	// Set the port to output
	/* Here I am assuming that the DDR_ address is always one less than the port itself */ 
	m_nPortBitMask = 0;
	m_nPortBitMask = m_nOE | m_nDS | m_nSHCP;
	for (int i=0; i<m_nCascadeCount; i++) {
		m_nPortBitMask |= m_anSTCP[i];
	}
	_SFR_IO8(ptrPort-1) = m_nPortBitMask;
	// Write zeros to all port values
	*m_ptrPort &= 0x00 | (~m_nPortBitMask);
	
	// Disable output
	OE_HI();
	
/*	
#define DDRC 
#define PORTC _SFR_IO8(0x15)

#define PORTD _SFR_IO8(0x12)
#define DDRD _SFR_IO8(0x11)

#define DDRB _SFR_IO8(0x17)
*/	
	// Assume shift registers read all zeros
	for (int i=0; i<m_nCascadeCount; i++) {
		m_anData[i]=0;
	}
	
}

sr595::~sr595() {
	free(m_anSTCP);
}
		

void sr595::writeByte(uint8_t nIndex, uint8_t nData)
{
	if (m_anData[nIndex] != nData) {
		m_anData[nIndex] = nData;
		for (int nByte = nIndex; nByte>=0; nByte++) {
			for (int nBit=7; nBit>=0; nBit--) {
				SHCP_LO();
				if (m_anData[nByte] & (0x01 << nBit)) {
					DS_HI();
				} else {
					DS_LO();
				}
				SHCP_HI();
			}
			if (m_fParallel) { break; }
		}
	}
	STCP_HI(nIndex);
	SHCP_LO();
	STCP_LO(nIndex);
	
}

#define DELAY_LONG	1000
#define DO_SHORT_DELAY	_delay_ms(250)
//~ #define DO_SHORT_DELAY	;
	
uint8_t STCP[2] = {SR74XX595_STCP0, SR74XX595_STCP1};

sr595 sr(
		2, 					// nCascadeCount
		1, 					// fParallel
		&SR74XX595_PORT, 	// ptrPort
		SR74XX595_OE, 		// nOE
		SR74XX595_DS, 		// nDS
		SR74XX595_SHCP, 	// nSHCP
		STCP				// anSTCP
		);

#define kDISPVALUE_COUNT				1
#define kDISPVALUE_NOVALUEAVAILABLE		0xFFFF
#define kDISPVALUE_DIGITCOUNT			3
#define kDISPVALUE_INDPAIRSCOUNT		3

volatile	uint16_t	aintDisplayValue[kDISPVALUE_COUNT];  				// 	Value to be displayed in the LEDs
volatile	uint8_t		idxDisplayValue;
volatile	uint8_t		aintDisplaySegments[kDISPVALUE_COUNT][kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT];  // 	Value to be displayed on each led
volatile	uint8_t		nResetting;

void setDisplayValue(uint8_t index, uint16_t intNewValue) {
	if (nResetting) { return; }
	if (aintDisplayValue[index]==intNewValue) { return; }
	
	aintDisplayValue[index] = intNewValue;
	switch (intNewValue) {
		case kDISPVALUE_NOVALUEAVAILABLE: {
			uint8_t i, j;
			for (i=0; i<kDISPVALUE_DIGITCOUNT; i++) {
				aintDisplaySegments[index][i] = LED7OUT_DASH;
			}
			break;
		}
		default: {
			uint8_t i, j;
			for (i=0; i<kDISPVALUE_DIGITCOUNT; i++) {
				uint16_t nCounterDivisor = 10;
				for (j = 0; j<i; j++) {
					nCounterDivisor = nCounterDivisor * 10;
				}
				uint32_t nCurrentDigit = (intNewValue % nCounterDivisor);
				if (nCounterDivisor >= 100) { nCurrentDigit = nCurrentDigit / (nCounterDivisor / 10); }
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
					aintDisplaySegments[index][i] = aint7segdigits[nCurrentDigit];
				}
			}
		}
	}
}

int main(void) {
	// Run at 8mhz
	//~ CLKPR = 1<<CLKPCE;
	//~ CLKPR = 0;
	
	DDRD = 0xFF;
	
	
	sr.writeByte(1, 0xFF);	// Enable all common cathodes
	sr.writeByte(0, 0xFF);	// Nothing is on
	sr.setOutput(1);
	
	while (0) {
		sr.writeByte(1, 1<<3);	// Enable all common cathodes
		//~ _delay_ms(250);
		//~ sr.writeByte(0, 0x7E);	// Nothing is on
		sr.setOutput(1);
	}
	
	if (1) {
	/* 	Timer stuff for display PWM - using timer 0
		Using 8 bit timer because this update happens fast, 
		the timer does not need to count very high
	*/
	TCNT0 = 0;										// 	Initial counter value
	TCCR0A =(1<<WGM01);								// 	CTC (Clear on capture = comparison) mode
	//~ TCCR0B = (1<<CS02) | (0<<CS01) | (1<<CS00);		// Prescaler = 1024
	//~ TCCR0B = (1<<CS02) | (0<<CS01) | (0<<CS00);		// Prescaler = 256
	TCCR0B = (0<CS02) | (1<<CS01) | (1<<CS00);		// Prescaler = 8
	//~ OCR0A = F_CPU/1024/1000;						// 	Refresh every second
	OCR0A = 0x80;										// 	About 1000 times per second (1Khz)
	TIMSK0 |= (1<<OCIE0A);								//	Enable interrupts on compare match A

	sei();								//	Start interrupt handling
	}
	
	uint16_t nDisplayValue = 0; 
	setDisplayValue(0, 232);
	_delay_ms(1000);
	setDisplayValue(0, 233);
	_delay_ms(1000);
	while (1) {
	}
	while (0) {

		setDisplayValue(0, nDisplayValue);
		_delay_ms(1000);
		if ( ++nDisplayValue > 999) {
			nDisplayValue = 0;
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
		
}


// Interrupt routine for servicing LED refreshment
ISR(TIMER0_COMPA_vect) {
	static uint8_t idxActiveDigit = 1;
	
	// Turn off all LEDs
	sr.setOutput(0);
	sr.writeByte(0, aintDisplaySegments[idxDisplayValue][idxActiveDigit]);
	sr.writeByte(1, 1<<(idxActiveDigit + 1));
	sr.setOutput(1);
	
	if (++idxActiveDigit >= kDISPVALUE_DIGITCOUNT) {
		idxActiveDigit = 0;
	}
	
	return;
}

ISR(RESET) {
	nResetting = 0xFF;
	// Shut down the display
	for (int i=0;i<kDISPVALUE_COUNT;i++) 
		for (int j=0;j<kDISPVALUE_DIGITCOUNT+kDISPVALUE_INDPAIRSCOUNT;j++) 
			aintDisplaySegments[i][j] = 0;
}