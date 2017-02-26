#include "msp430.h"
#include "eos.h"
#include "uart.h"

#define TXD BIT2
#define RXD BIT1

#define BUFFER_SIZE 64

char rxb[BUFFER_SIZE];
char txb[BUFFER_SIZE];

byte rxb_in = 0;
byte rxb_out = 0;

byte txb_in = 0;
byte txb_out = 0;

#define RXFULL (rxb_in == ((rxb_out - 1 + BUFFER_SIZE) % BUFFER_SIZE))
#define RXEMPTY (rxb_in == rxb_out)

#define TXFULL (txb_in == ((txb_out - 1 + BUFFER_SIZE) % BUFFER_SIZE))
#define TXEMPTY (txb_in == txb_out)

byte UART_sending = 0;

void uart_init(void)
{
    //WDTCTL = WDTPW + WDTHOLD; // Stop WDT
    //DCOCTL = 0; // Select lowest DCOx and MODx settings
    //  BCSCTL1 = CALBC1_16MHZ; // Set DCO
    // DCOCTL = CALDCO_16MHZ;
    //  P2DIR |= 0xFF; // All P2.x outputs
    //  P2OUT &= 0x00; // All P2.x reset

    UCA0CTL1 |= UCSWRST; // **Stop state machine during initialization**

    P1SEL |= RXD + TXD;  // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= RXD + TXD; // P1.1 = RXD, P1.2=TXD
    P1OUT &= 0x00;
    P1DIR &= ~RXD;

    UCA0CTL1 = UCSSEL_2; // SMCLK
    UCA0CTL0 = 0x0;      // LSB

    // BRCLK 16*10^6 BAUD = 9600 bps, N = BRCLK/BAUD
    // -> N = 1667  0x683

    UCA0BR0 = 0x83;
    UCA0BR1 = 0x6;
    UCA0MCTL = UCBRS_6; // Modulation USBR6

    /*
if (0) {
    UCA0BR0 = 0x68; // 1MHz 9600
    UCA0BR1 = 0x00; // 1MHz 9600
    UCA0MCTL = UCBRS1;// Modulation
  } else {
    UCA0BR0 = 0xe2; // 16MHz 9600
  UCA0BR1 = 0x04; // 16MHz 9600
  UCA0MCTL = UCBRS1;// Modulation
    }

*/

    UC0IE |= UCA0RXIE;  // Enable USCI_A0 RX interrupt
    UC0IE &= ~UCA0TXIE; // Disble USCI_A0 TX interrupt

    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**

    //  __bis_SR_register(CPUOFF + GIE); // Enter LPM0 w/ int until Byte RXed
}

inline void UART_putc(byte c)
{

    if (!UART_sending)
    {
        UART_sending = 1;
        UCA0TXBUF = c;
        UC0IE |= UCA0TXIE; // enable TX interrupt
    }
    else if (!TXFULL)
    {
        txb[txb_in] = c;
        txb_in = (txb_in + 1) % BUFFER_SIZE;
    }
    // otherwise c is lost...
}

inline byte UART_getc()
{
    byte c;
    if (!RXEMPTY)
    {
        c = rxb[rxb_out++];
        rxb_out = (rxb_out + 1) % BUFFER_SIZE;
        return c;
    }
    return 0;
}

void uart_write(char *str)
{
    while (*str)
    {
        UART_putc(*str++);
    }
}

byte uart_read(char *str, byte max)
{
    byte count = 0;
    while (count < max)
    {
        if (!(*str++ = UART_getc()))
        {
            break;
        }
        else
        {
            count++;
        }
    }
    return count;
}

__attribute__((interrupt(USCIAB0RX_VECTOR))) void USCI0RX_ISR(void)
{

    byte c;
    if (UC0IFG & UCA0RXIFG)
    {

        __disable_interrupt();
        IFG2 &= ~UCA0RXIFG; // Clear RX flag
        c = UCA0RXBUF;      // Read byte
        //P1OUT ^= BIT0; //Toggle red led
        if (!RXFULL)
        {
            rxb[rxb_in] = c;
            rxb_in = (rxb_in + 1) % BUFFER_SIZE;
        }
        __enable_interrupt();
    }
}


__attribute__((interrupt(USCIAB0TX_VECTOR))) void USCI0TX_ISR(void)
{
    //     if (IFG2 & UCA0TXIFG) {

    __disable_interrupt();
    if (TXEMPTY)
    {
        // Stop TX interrupt
        UC0IE &= ~UCA0RXIE;
        UART_sending = 0;
    }
    else
    {
        //P1OUT ^= BIT0; //Toggle red led

        UCA0TXBUF = txb[txb_out];
        txb_out = (txb_out + 1) % BUFFER_SIZE;
        //           UCA0TXBUF = 0xAA;
    }
    __enable_interrupt();

    //   }
}
