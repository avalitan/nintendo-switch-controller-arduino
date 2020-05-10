/**
 * IMPORTANT!
 * WHEN wiring the Arduino Uno R3 board,
 * the TX and RX ports connected to the atmega16u2
 * are switched since it is meant to be a middleman!!
 *
 * This means (from the atmega16u2 perspective):
 * - Board Pin TX -> 1 = atmega16u2 Receive Pin
 * - Board Pin RX <- 0 = atmega16u2 Transmit Pin
 */
#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/Board/LEDs.h>

#include "LightweightRingBuff.h"

RingBuff_t RX_Buffer;
RingBuff_t TX_Buffer;

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
    // serial init with interrupts
    Serial_Init(9600, false);
    UCSR1B |= (1<<RXCIE1);

    // LED init
    LEDs_Init();
}

void SetupSoftware(void) {
    RingBuffer_InitBuffer(&RX_Buffer);
    RingBuffer_InitBuffer(&TX_Buffer);
}

void Setup(void) {
    SetupSoftware();
    SetupHardware();
}

bool CanWrite(void) {
    return !RingBuffer_IsFull(&TX_Buffer);
}

void Write(const uint8_t data) {
    RingBuffer_Insert(&TX_Buffer, data);
    // activate interrupt for transmit register empty
    UCSR1B |= (1<<UDRIE1);
}

bool CanRead(void) {
    return !RingBuffer_IsEmpty(&RX_Buffer);
}

uint8_t Read(void) {
    return RingBuffer_Remove(&RX_Buffer);
}

void Loop(void) {
    static uint8_t state = 0;

    if (CanWrite()) {
        state ^= 1;
        LEDs_ToggleLEDs(LEDMASK_TX);
        Write(state);
    }

    if (CanRead()) {
        if (Read()) {
            LEDs_TurnOnLEDs(LEDMASK_RX);
        } else {
            LEDs_TurnOffLEDs(LEDMASK_RX);
        }
    }

    Delay_MS(50);
}

// Main entry point.
int main(void) {
	Setup();
    LEDs_TurnOffLEDs(LEDS_ALL_LEDS);

	GlobalInterruptEnable();


	for (;;)
		Loop();
}

ISR(USART1_UDRE_vect)
{
    if (!RingBuffer_IsEmpty(&TX_Buffer)) {
        UDR1 = RingBuffer_Remove(&TX_Buffer);
    } else {
        // deactivate isr when there is no data to send
        UCSR1B &= ~(1<<UDRIE1);
    }
}

ISR(USART1_RX_vect)
{
	volatile uint8_t ReceivedByte = UDR1;
	RingBuffer_Insert(&RX_Buffer, ReceivedByte);
}
