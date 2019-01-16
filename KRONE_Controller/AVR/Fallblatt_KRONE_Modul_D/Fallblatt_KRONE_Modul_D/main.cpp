/*
 * Fallblatt_KRONE_Modul_D.cpp
 *
 * Created: 04.10.2017 16:06:27
 * Author : Julian Metzler
 */

#define F_CPU 8000000UL

#define NUM_CARDS 52

#define sbi(a, b) ((a) |= (1 << (b)))
#define cbi(a, b) ((a) &= ~(1 << (b)))
#define rbi(a, b) ((a) & (1 << (b)))

#include <avr/io.h>
#include <util/delay.h>
#include "uart.h"

int curCardIndex;

unsigned char getAddress() {
	return ~PINC & 0b00011111;
}

bool getSystemFlag1() {
	return !rbi(PIND, PD2);
}

bool getSystemFlag2() {
	return !rbi(PIND, PD3);
}

bool getSystemFlag3() {
	return !rbi(PINC, PC5);
}

bool getCardSensor() {
	if(getSystemFlag3()) {
		return rbi(PINB, PB2);
	} else {
		return !rbi(PINB, PB2);
	}
}

bool getHomeSensor() {
	return !rbi(PINB, PB1);
}

void setSensors(bool state) {
	if(state) {
		sbi(PORTB, PB0);
	} else {
		cbi(PORTB, PB0);
	}
}

void setMotor(bool state) {
	if(state) {
		sbi(PORTD, PD5);
	} else {
		cbi(PORTD, PD5);
	}
}

void flipCards(int numCards) {
	// Enable Photointerrupters
	setSensors(1);
	_delay_ms(5);
	bool homeLeft = rbi(PINB, PB1);
	bool homeDetected = false;
	// Enable Motor
	setMotor(1);
	for(int i = 0; i < numCards; i++) {
		// Wait for Out of Card Flip
		while(getCardSensor());
		// Wait for Card Flip
		while(!getCardSensor());
		// Check if still registering Home Position
		if(!homeLeft && !getHomeSensor()) {
			homeLeft = true;
		}
		curCardIndex++;
		// Check for Home Position
		//if(homeLeft && !homeDetected && getHomeSensor()) {
			//// Only do this once because the home position is being detected across multiple cards
			//numCards += curCardIndex;
			//if(numCards >= NUM_CARDS) {
				//numCards -= NUM_CARDS;
			//}
			//curCardIndex = 0;
			//homeDetected = true;
		//}
		if(curCardIndex >= NUM_CARDS) {
			curCardIndex = 0;
		}
	}
	_delay_ms(10);
	// Disable Motor
	setMotor(0);
	// Disable Photointerrupters
	setSensors(0);
}

void flipToHome() {
	// Enable Photointerrupters
	setSensors(1);
	_delay_ms(5);
	// Enable Motor
	setMotor(1);
	// Wait for Out of Home Position
	while(getHomeSensor());
	while(1) {
		// Wait for Card Flip
		while(!getCardSensor());
		// Check for Home Position
		if(getHomeSensor()) {
			break;
		}
	}
	if(getSystemFlag1()) {
		_delay_ms(2);
	}
	if(getSystemFlag2()) {
		_delay_ms(10);
	}
	// Disable Motor
	setMotor(0);
	// Disable Photointerrupters
	setSensors(0);
	curCardIndex = 0;
}

void flipToCard(int cardIndex) {
	if(curCardIndex == cardIndex) {
		return;
	}
	int cardsToFlip = cardIndex - curCardIndex;
	if(cardsToFlip < 0) {
		cardsToFlip += NUM_CARDS;
	}
	flipCards(cardsToFlip);
}

void detectCurCardIndex() {
	// Enable Photointerrupters
	setSensors(1);
	_delay_ms(5);
	// Flip through all cards, registering the home position in the process
	int cardsFlipped = 0;
	bool homeLeft = !getHomeSensor();
	bool homeDetected = false;
	// Enable Motor
	setMotor(1);
	while(cardsFlipped < NUM_CARDS) {
		// Wait for Out of Card Flip
		while(getCardSensor());
		// Wait for Card Flip
		while(!getCardSensor());
		cardsFlipped++;
		// Check if still registering Home Position
		if(!homeLeft && !getHomeSensor()) {
			homeLeft = true;
		}
		// Check for Home Position
		if(homeLeft && !homeDetected && getHomeSensor()) {
			// Only do this once because the home position is being detected across multiple cards
			curCardIndex = NUM_CARDS - cardsFlipped;
			homeDetected = true;
		}
	}
	_delay_ms(10);
	// Disable Motor
	setMotor(0);
	// Disable Photointerrupters
	setSensors(0);
}

void rs485PutByte(unsigned char byte) {
	sbi(PORTD, PD4);
	_delay_ms(10);
	uart0_putc(byte);
	uart0_flush();
	_delay_ms(10);
	cbi(PORTD, PD4);
}

unsigned char rs485GetByte() {
	cbi(PORTD, PD4);
	while(!uart0_available());
	return uart0_getc() & 0xff;
}

unsigned char letterToCardPosition(unsigned char letter) {
	if(letter >= 48 && letter <= 57) {
		// 0 to 9
		return letter - 47;
	} else if(letter >= 65 && letter <= 90) {
		// A to Z
		return letter - 54;
	} else if(letter >= 97 && letter <= 122) {
		// a to z, mapped to A to Z
		return letter - 86;
	} else {
		switch(letter) {
			case '-': return 40;
			case '.': return 41;
			case '(': return 42;
			case ')': return 43;
			case '!': return 44;
			case ':': return 45;
			case '/': return 46;
			case '"': return 47;
			case ',': return 48;
			case '=': return 49;
			default:  return 0;
		}
	}
}

#define R

int main(void) {
	DDRB  = 0b00000001;
	PORTB = 0b00000110;
	DDRC  = 0b00000000;
	PORTC = 0b00111111;
	DDRD  = 0b00110010;
	PORTD = 0b00001100;
	
	unsigned char lastStatus = 0x00;
	unsigned char text1[13] = " Heeyy Beter";
	unsigned char text2[13] = " Split-Flap ";
	unsigned char text3[13] = " Mach. broke";
	unsigned char text4[13] = " Undrstndbl!";
	
	uart0_init(UART_BAUD_SELECT(9600, F_CPU));
	
	#ifdef S
	flipToHome();
	rs485PutByte(0xff);
	rs485PutByte(0xff);
	rs485PutByte('h');
	_delay_ms(5000);
	while(1) {
		for(int i = 11; i > 0; i--) {
			rs485PutByte(0xff);
			rs485PutByte(i);
			rs485PutByte('c');
			rs485PutByte(i);
		}
		_delay_ms(2000);
		for(int i = 0; i < 10; i++) {
			rs485PutByte(0xff);
			rs485PutByte(0xff);
			rs485PutByte('+');
			rs485PutByte(1);
			_delay_ms(250);
		}
		_delay_ms(2000);
		rs485PutByte(0xff);
		rs485PutByte(0xff);
		rs485PutByte('+');
		rs485PutByte(11);
		_delay_ms(2000);
		for(int i = 1; i < 12; i++) {
			rs485PutByte(0xff);
			rs485PutByte(i);
			rs485PutByte('c');
			rs485PutByte(letterToCardPosition(text1[i]));
		}
		_delay_ms(5000);
		for(int i = 1; i < 12; i++) {
			rs485PutByte(0xff);
			rs485PutByte(i);
			rs485PutByte('c');
			rs485PutByte(letterToCardPosition(text2[i]));
		}
		_delay_ms(5000);
		for(int i = 1; i < 12; i++) {
			rs485PutByte(0xff);
			rs485PutByte(i);
			rs485PutByte('c');
			rs485PutByte(letterToCardPosition(text3[i]));
		}
		_delay_ms(5000);
		for(int i = 1; i < 12; i++) {
			rs485PutByte(0xff);
			rs485PutByte(i);
			rs485PutByte('c');
			rs485PutByte(letterToCardPosition(text4[i]));
		}
		_delay_ms(5000);
		rs485PutByte(0xff);
		rs485PutByte(0xff);
		rs485PutByte('h');
		_delay_ms(5000);
	}
	#endif
	while(1) {
		#ifdef R
		// Wait for start byte
		if(rs485GetByte() != 0xff) { continue; }
		// Check address byte (own address or broadcast address 0xff)
		unsigned char addr = rs485GetByte();
		bool broadcast = (addr == 0xff);
		if(addr != getAddress() && !broadcast) { continue; }
		// Check command byte
		unsigned char cmd = rs485GetByte();
		switch(cmd) {
			case '+': {
				// Flip a relative amount of cards
				unsigned char numCards = rs485GetByte();
				flipCards(numCards);
				lastStatus = 0xff;
				break;
			}
			case 'c': {
				// Flip to an absolute position
				unsigned char cardId = rs485GetByte();
				flipToCard(cardId);
				lastStatus = 0xff;
				break;
			}
			case 'h': {
				// Flip to the home position
				flipToHome();
				lastStatus = 0xff;
				break;
			}
			case 'g': {
				// Get current position
				if(!broadcast) { rs485PutByte(curCardIndex); }
				lastStatus = 0xff;
				break;
			}
			case 'd': {
				// Detect current position
				detectCurCardIndex();
				lastStatus = 0xff;
				break;
			}
		}
		#endif
		#ifdef T
		flipToHome();
		_delay_ms(2500);
		flipToCard(11);
		_delay_ms(1000);
		for(int i = 0; i < 10; i++) {
			flipCards(1);
			_delay_ms(500);
		}
		_delay_ms(750);
		flipToCard(36);
		_delay_ms(1000);
		#endif
	}
}

