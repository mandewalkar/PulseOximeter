// interfacing 16x2 LCD
// Author: Luan Ferreira Reis de Jesus

#include <msp430g2553.h>

// define RS high
#define DR P1OUT = P1OUT | BIT0

// define RS low
#define CR P1OUT = P1OUT & (~BIT0)

// define Read signal R/W = 1 for reading
#define READ P1OUT = P1OUT | BIT1

// define Write signal R/W = 0 for writing
#define WRITE P1OUT = P1OUT & (~BIT1)

// define Enable high signal
#define ENABLE_HIGH P1OUT = P1OUT | BIT2

// define Enable Low signal
#define ENABLE_LOW P1OUT = P1OUT & (~BIT2)

void configure_clocks()
{
  // Stop watchdog
  WDTCTL = WDTPW + WDTHOLD;
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
}


void delay_us(unsigned int us)
{
  while (us)
  {
    // 1 for 1 Mhz set 16 for 16 MHz
    __delay_cycles(1);
    us--;
  }
}


void delay_ms(unsigned int ms)
{
  while (ms)
  {
    // 1000 for 1MHz and 16000 for 16MHz
    __delay_cycles(1000);
    ms--;
  }
}


void data_write(void)
{
  ENABLE_HIGH;
  delay_ms(5);
  ENABLE_LOW;
}


void send_data(unsigned char data)
{
  unsigned char higher_nibble = 0xF0 & (data);
  unsigned char lower_nibble = 0xF0 & (data << 4);

  delay_us(200);

  WRITE;
  DR;
  // send higher nibble
  P1OUT = (P1OUT & 0x0F) | (higher_nibble);
  data_write();

  // send lower nibble
  P1OUT = (P1OUT & 0x0F) | (lower_nibble);
  data_write();
}


void send_string(char *s)
{
  while(*s)
  {
    send_data(*s);
    s++;
  }
}


void send_command(unsigned char cmd)
{
  unsigned char higher_nibble = 0xF0 & (cmd);
  unsigned char lower_nibble = 0xF0 & (cmd << 4);

  WRITE;
  CR;
  // send higher nibble
  P1OUT = (P1OUT & 0x0F) | (higher_nibble);
  data_write();

  // send lower nibble
  P1OUT = (P1OUT & 0x0F) | (lower_nibble);
  data_write();
}


void lcd_init(void)
{
  P1DIR |= 0xFF;
  P1OUT &= 0x00;

  delay_ms(15);
  send_command(0x33);

  delay_us(200);
  send_command(0x32);

  delay_us(40);
  send_command(0x28); // 4 bit mode

  delay_us(40);
  send_command(0x0E); // clear the screen

  delay_us(40);
  send_command(0x01); // display on cursor on

  delay_us(40);
  send_command(0x06); // increment cursor

  delay_us(40);
  send_command(0x80); // row 1 column 1
}

/*
int main(void)
{
  configure_clocks();

  lcd_init();

  send_string("SpO2:");
  send_command(0xC0);
  send_string("100%");

  delay_ms(1000);

  send_command(0x01);
  send_command(0x0C);
  send_command(0x80);
  send_string("Sp02:");
  send_command(0xC0);
  send_string("99%");


  while (1)
  {
  }
}
*/
