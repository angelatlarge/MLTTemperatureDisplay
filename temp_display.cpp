#define F_CPU 1000000UL /* 1 MHz Internal Oscillator */
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
//~ #include <math.h>

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
		void write_byte(uint8_t nIndex, uint8_t nData);				
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
		

void sr595::write_byte(uint8_t nIndex, uint8_t nData)
{
	if (1) { //(m_anData[nIndex] != nData) {
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
int main(void) {
	// RUn at 8mhz
	//~ CLKPR = 1<<CLKPCE;
	//~ CLKPR = 0;
	
	DDRD = 0xFF;
	
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
	
	sr.write_byte(1, 0xFF);	// Enable all common cathodes
	sr.write_byte(0, 0x00);	// Nothing is on
	sr.setOutput(1);
	
	while (1) {

		
		sr.setOutput(1);
		while(1) {
			for (int i= 0; i<8; i++) {
				sr.write_byte(0, 1<<i);
				sr.write_byte(1, 0xFF);
				//~ sr.write_byte(1, 1<<i);
				DO_SHORT_DELAY;
				//~ sr.toggleOutput();
			}
		}
	}
		
}
