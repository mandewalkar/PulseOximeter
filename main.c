#include <msp430.h>
#include <msp430g2553.h>
#include <stdbool.h>




int i;
char charmemval[] = "P2.5 Value: ";
char charmemval2[] = "P2.1 Value: ";
char charmemval3[] = "SpO2 Value: ";
char charmemval4[] = "R Value: ";
char charmemval5[] = ",";
char charreturn[] = "\r\n";
char mv_char[8];
char sp_char[8];
float SpO2 = 0;
float R=0;
void ser_output(char *str);
void itoa(int n, char s[]);
void ftoa(float n, char* buffer, int afterpoint);
void reverse(char s[]);

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



#define maxperiod_siz 20 // max number of samples in a period
#define measures 10      // number of periods stored
#define samp_siz 5       // number of (averaged) samples for average
#define rise_threshold 3 // number of rising measures to determine a peak

// a liquid crystal displays BPM
//LiquidCrystal_I2C lcd(0x3F, 16, 2);

int T = 20;    // slot milliseconds to read a value from the sensor

int avBPM;

int p25_edge_flag = 0;
int n3;
int n2;

int reading_flag = 0; // 0 is RED LED; 1 is IR LED


int main(void)
{
    __enable_interrupt();                     // Enable interrupts.
      TACCR0 = 30;                              // Delay to allow Ref to settle
      TACCTL0 |= CCIE;                          // Compare-mode interrupt.
      TACTL = TASSEL_2 | MC_1;                  // TACLK = SMCLK, Up mode.
      LPM0;                                     // Wait for delay.
      TACCTL0 &= ~CCIE;                         // Disable timer Interrupt
      __disable_interrupt();

      P2REN = BIT3;
      P2IES = (BIT3); // low to high transition
      P2IFG &=  ~(BIT3); // clear any pending interrupts
      P2IE = BIT3; // enable interrupts for these pins
      __enable_interrupt();              // enable all interrupts

//      configure_clocks();
//      lcd_init();
//
//      send_string("SpO2:");
//      send_command(0xC0); //move cursor to second row
//      send_string("100%");

    //    WDTCTL = WDTPW | WDTHOLD;
      BCSCTL1 = CALBC1_1MHZ;
      BCSCTL3 = LFXT1S_2;
      DCOCTL = CALDCO_1MHZ;
      // Set up WDT interrupt for helpfulness
    //  WDTCTL = WDT_ADLY_1000;       // WDT interrupt
    //  IE1 |= WDTIE;               // Enable WDT interrupt

      //WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
          WDTCTL = WDT_ADLY_1000;
          IE1 |= WDTIE;
      //  ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; // ADC10ON, interrupt enabled
        ADC10CTL0 = SREF_0 | ADC10SHT_2|ADC10ON|ENC;
          ADC10CTL1 = INCH_3|SHS_0|ADC10DIV_0|ADC10SSEL_0|CONSEQ_0;;                       // input A1
          ADC10AE0 |= BIT3;                         // PA.1 ADC option select

      P1SEL = BIT1|BIT2;
      P1SEL2 = BIT1|BIT2;

      UCA0CTL1 |= UCSWRST+UCSSEL_2;
      UCA0BR0 = 52;  //settings for 19200 baud
      UCA0BR1 = 0;
      UCA0MCTL = UCBRS_0;
      UCA0CTL1 &= ~UCSWRST;


//      P1DIR = BIT4 | BIT5 | BIT6; //for BJT gates
//        //P1SEL = BIT4;
//      //  P1IN |= BIT3;
//        P1OUT &= BIT4 | BIT6;
//        P1OUT &= ~BIT5;

      TA1CTL = TASSEL_1 + MC_1; //ACLK, upmode

      TA1CCR0 = 100;
      TA1CCR1 = 50;
      TA1CCR2 = 50;

      TA1CCTL2 = OUTMOD_3; //PWM Set/Reset
      TA1CCTL1 = OUTMOD_7; //PWM Reset/Set for "inverse" phase between Red and IR LEDs

      P2DIR = BIT5 | BIT1;
      P2SEL = BIT5 | BIT1;

      int temp;

      bool finger_status = true;

        float readsIR[samp_siz], sumIR,lastIR, start;
        float readsRED[samp_siz], sumRED,lastRED;


        int period, samples;
        period=0; samples=0;
        int samplesCounter = 0;
        int readsIRMM[maxperiod_siz];
        int readsREDMM[maxperiod_siz];
        int ptrMM = 0;
        int i;
        for (i = 0; i < maxperiod_siz; i++) {
            readsIRMM[i] = 0;
            readsREDMM[i]=0;
        }
        float IRmax=-1;
        float IRmin=700;
        float REDmax=-1;
        float REDmin=700;
        float measuresR[10];
        int measuresPeriods[10];
        int m = 0;
        for (i = 0; i < 10; i++) { measuresPeriods[i]=0; measuresR[i]=0; }

        int red_ptr, ir_ptr, reader, cycle_count;

        float beforeIR;

        bool rising;
        int rise_count;
        long int last_beat;
        for (i = 0; i < samp_siz; i++) { readsIR[i] = 0; readsRED[i]=0; }
        sumIR = 0; sumRED=0;
        ir_ptr = 0;
        red_ptr = 0;
        cycle_count = 0;
        n3 = 0;
        n2 = 0;
        reader = 0;

  while(1)
  {

//      configure_clocks();
//      lcd_init();
//
//      send_string("SpO2:");a

//     send_command(0x01); //clear LCD
//     itoa(SpO2, sp_char);
//     send_string("SpO2:");
//     send_command(0xC0); //move cursor to second row
//     send_string(sp_char);
//     send_string("%");
      if (reading_flag == 0){ //IR Reading
          //P1OUT ^= (BIT2 | BIT3 | BIT6);   ~00?000000 & 00100000       001000000 & 11011111
          while((p25_edge_flag == 0)); //red LED is off, IR on
          p25_edge_flag = 0;
          ADC10CTL0 |= ADC10SC;
          while(ADC10CTL1 & ADC10BUSY); //wait until conversion completed
          ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
          temp = ADC10MEM;
          reader += temp;
//          itoa(temp, mv_char); //UART the lastRED (average red) value
//          ser_output(charmemval);
//          ser_output(mv_char);
//          ser_output(charreturn);
          n2++; //one more sample collected

          //reader /= n;  // we got an average
          // Add the newest measurement to an array
          // and subtract the oldest measurement from the array
          // to maintain a sum of last measurements
          /*
          sumIR -= readsIR[ptr];
          sumIR += reader;
          readsIR[ptr] = reader;
          lastIR = sumIR / samp_siz;
          */

          if (n2 >= 10){
                reader /= n2; //average over last 5
               // readsIR[ir_ptr] = reader;
                sumIR -= readsIR[ir_ptr];
                sumIR += reader;
                readsIR[ir_ptr] = reader;
                lastIR = sumIR / samp_siz;
                reading_flag = 1;
                P2IES &= ~(BIT3); // low to high transition
                P2IFG &=  ~(BIT3); // clear any pending interrupts
                p25_edge_flag = 0;
                n2 = 0;
//                itoa(reader, mv_char); //UART the lastIR (average IR) value
//                ser_output(charmemval);
//                ser_output(mv_char);
//                ser_output(charreturn);
                reader = 0;
                ir_ptr++;
                ir_ptr %= samp_siz;
          }

      } else if (reading_flag == 1){
          while((p25_edge_flag == 0)); //red LED is off, IR on
          p25_edge_flag = 0;
          ADC10CTL0 |= ADC10SC;
          while(ADC10CTL1 & ADC10BUSY);
          ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
          temp = ADC10MEM;
          reader += temp;
//          itoa(temp, mv_char); //UART the lastRED (average red) value
//          ser_output(charmemval2);
//          ser_output(mv_char);
//          ser_output(charreturn);
          n3++; //one more sample collected
      if (n3 >= 10){
          reader /= n3; //average over last 20
          //readsRED[red_ptr] = reader;
          sumRED -= readsRED[red_ptr];
          sumRED += reader;
          readsRED[red_ptr] = reader;
          lastRED = sumRED / samp_siz;
          //P2IES &= ~(BIT3); // low to high transition
          //P2IFG &=  ~(BIT3); // clear any pending interrupts
          P2IES = (BIT3); // low to high transition
          P2IFG ^=  (BIT3); // clear any pending interrupts
          n3 = 0;
//          itoa(reader, mv_char); //UART the lastRED (average red) value
//          ser_output(charmemval2);
//          ser_output(mv_char);
//          ser_output(charreturn);
          reader = 0;
          red_ptr++;
          red_ptr %= samp_siz;
          cycle_count++;
          if (cycle_count == 5){
               reading_flag = 2; //switched to calculations
               cycle_count = 0;
          } else{
               reading_flag = 0; //switch back to IR reads
          }
      }
    //__bis_SR_register(LPM3_bits + GIE);     // Enter LPM3
      } else if (reading_flag == 2){
          readsIRMM[ptrMM] = lastIR;
          readsREDMM[ptrMM] = lastRED;
          ptrMM++;
          ptrMM %= maxperiod_siz - 1;
          //samplesCounter++;
          //n3 = 0;
          if(1){
            //samplesCounter =0;
            IRmax = 0; IRmin=1023; REDmax = 0; REDmin=1023;
            for(i=0;i<maxperiod_siz;i++) {
              if( readsIRMM[i]> IRmax) IRmax = readsIRMM[i];
              if( readsIRMM[i]>0 && readsIRMM[i]< IRmin ) IRmin = readsIRMM[i];
              //readsIRMM[i] =0;
              if( readsREDMM[i]> REDmax) REDmax = readsREDMM[i];
              if( readsREDMM[i]>0 && readsREDMM[i]< REDmin ) REDmin = readsREDMM[i];
              //readsREDMM[i] =0;
            }
            R =  ( (REDmax-REDmin) / REDmin) / ( (IRmax-IRmin) / IRmin );
          }
          int status;
          if (R >= 1.2 || R <= 0.4){
              status = 0;
          } else {
              status = 1;
          }
          SpO2 = -10 * R + 104;
          ftoa(R, mv_char, 4); //UART the lastIR (average IR) value
//          ser_output(charmemval4);
          ser_output(mv_char);
          ser_output(charmemval5);
          ftoa(SpO2, mv_char, 4); //UART the lastIR (average IR) value
//          ser_output(charmemval3);
          ser_output(mv_char);
          ser_output(charmemval5);
          itoa(status, mv_char);
          ser_output(mv_char);
          ser_output(charreturn);

//       if(SpO2 <= 100){
//       itoa(SpO2, sp_char);
//       send_string("SpO2:");
//       send_command(0xC0); //move cursor to second row
//       send_string(sp_char);
//       send_string("%");
//       }

       reading_flag = 0;
      }
  }
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
#else
#error Compiler not supported!
#endif
{
  __bic_SR_register_on_exit(LPM3_bits);     // Clear LPM3 bits from 0(SR)
}




// ADC10 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC10_VECTOR))) ADC10_ISR (void)
#else
#error Compiler not supported!
#endif
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void ta0_isr(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) ta0_isr (void)
#else
#error Compiler not supported!
#endif
{
  TACTL = 0;
  LPM0_EXIT;                                // Exit LPM0 on return
}



void ser_output(char *str){
    while(*str != 0) {
        while (!(IFG2&UCA0TXIFG));
        UCA0TXBUF = *str++;
        }
}


/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void ftoa(float n, char* buffer, int afterpoint) {
    int ipart = (int)n;  // Extract integer part
    float fpart = n - (float)ipart; // Extract floating part
    int i = 0;

    // Handle negative numbers
    if (n < 0) {
        buffer[i++] = '-';
        ipart = -ipart;
        fpart = -fpart;
    }

    // Convert integer part to string
    itoa(ipart, buffer + i);

    // Find length of integer part
    while (buffer[i] != '\0') {
        i++;
    }

    // Add decimal point
    buffer[i++] = '.';

    // Extract digits from the floating part one by one
    int j;
    for (j = 0; j < afterpoint; j++) {
        fpart *= 10;
        int digit = (int)fpart;
        buffer[i++] = digit + '0';
        fpart -= digit;
    }

    // Null terminate the string
    buffer[i] = '\0';
}


void initialize(){
    __enable_interrupt();                     // Enable interrupts.
      TACCR0 = 30;                              // Delay to allow Ref to settle
      TACCTL0 |= CCIE;                          // Compare-mode interrupt.
      TACTL = TASSEL_2 | MC_1;                  // TACLK = SMCLK, Up mode.
      LPM0;                                     // Wait for delay.
      TACCTL0 &= ~CCIE;                         // Disable timer Interrupt
      __disable_interrupt();

      P2REN = BIT3;
      P2IES = (BIT3); // low to high transition
      P2IFG &=  ~(BIT3); // clear any pending interrupts
      P2IE = BIT3; // enable interrupts for these pins
      __enable_interrupt();              // enable all interrupts

//      configure_clocks();
//      lcd_init();
//
//      send_string("SpO2:");
//      send_command(0xC0); //move cursor to second row
//      send_string("100%");

    //    WDTCTL = WDTPW | WDTHOLD;
      BCSCTL1 = CALBC1_1MHZ;
      BCSCTL3 = LFXT1S_2;
      DCOCTL = CALDCO_1MHZ;
      // Set up WDT interrupt for helpfulness
    //  WDTCTL = WDT_ADLY_1000;       // WDT interrupt
    //  IE1 |= WDTIE;               // Enable WDT interrupt

      //WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
          WDTCTL = WDT_ADLY_1000;
          IE1 |= WDTIE;
      //  ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; // ADC10ON, interrupt enabled
        ADC10CTL0 = SREF_2 | ADC10SHT_2|ADC10ON|ENC;
          ADC10CTL1 = INCH_3|SHS_0|ADC10DIV_0|ADC10SSEL_0|CONSEQ_0;;                       // input A1
          ADC10AE0 |= BIT3;                         // PA.1 ADC option select

      P1SEL = BIT1|BIT2;
      P1SEL2 = BIT1|BIT2;

      UCA0CTL1 |= UCSWRST+UCSSEL_2;
      UCA0BR0 = 52;  //settings for 19200 baud
      UCA0BR1 = 0;
      UCA0MCTL = UCBRS_0;
      UCA0CTL1 &= ~UCSWRST;


//      P1DIR = BIT4 | BIT5 | BIT6; //for BJT gates
//        //P1SEL = BIT4;
//      //  P1IN |= BIT3;
//        P1OUT &= BIT4 | BIT6;
//        P1OUT &= ~BIT5;

      P2DIR = BIT5 | BIT1;
      P2SEL = BIT5 | BIT1;
}
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void button(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT2_VECTOR))) button (void)
#else
#error Compiler not supported!
#endif
{
    if (P2IFG & BIT3){ // if PWM jumps high
        p25_edge_flag = 1;
        P2IFG &= ~(BIT3); // toggles off all the flags so that interrupt can be used again
    }
    __bic_SR_register_on_exit(LPM3_bits); // exit LPM3 when returning to program (clear LPM3 bits)
}
