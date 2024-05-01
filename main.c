#include <msp430.h>
#include <msp430g2553.h>
#include <stdbool.h>




int i; // var used as index in for loops

// variables for printing values through UART/Serial
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



#define maxperiod_siz 20 // max number of samples in a period
#define measures 10      // number of periods stored
#define samp_siz 5       // number of (averaged) samples for average


int p25_edge_flag = 0; // flag that lets the code know if the PWM has gone on a falling or rising edge
int n3; // for indexing the number of averaged Red LED samples in order to move to next stage of calcs
int n2; // for indexing the number of averaged Red LED samples in order to move to next stage of calcs

int reading_flag = 0; // 0 is RED LED; 1 is IR LED; 2 is for R calc


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

      BCSCTL1 = CALBC1_1MHZ;
      BCSCTL3 = LFXT1S_2;
      DCOCTL = CALDCO_1MHZ;

      // watchdog settings
      WDTCTL = WDT_ADLY_1000;
      IE1 |= WDTIE;


      // ADC settings
      ADC10CTL0 = SREF_0 | ADC10SHT_2|ADC10ON|ENC;
      ADC10CTL1 = INCH_3|SHS_0|ADC10DIV_0|ADC10SSEL_0|CONSEQ_0;;                       // input A3
      ADC10AE0 |= BIT3;                         // PA.3 ADC option select


      // Serial Comm settings
      P1SEL = BIT1|BIT2;
      P1SEL2 = BIT1|BIT2;

      UCA0CTL1 |= UCSWRST+UCSSEL_2;
      UCA0BR0 = 52;  //settings for 19200 baud
      UCA0BR1 = 0;
      UCA0MCTL = UCBRS_0;
      UCA0CTL1 &= ~UCSWRST;




      //Timer A1 settings
      TA1CTL = TASSEL_1 + MC_1; //ACLK, upmode

      TA1CCR0 = 100; // experimentally found val for the proper PWM freq
      TA1CCR1 = 50; // 50 % duty cycle
      TA1CCR2 = 50; // 50% duty cycle

      // one LED is on while other is off
      TA1CCTL2 = OUTMOD_3; //PWM Set/Reset
      TA1CCTL1 = OUTMOD_7; //PWM Reset/Set for "inverse" phase between Red and IR LEDs

      // Connections for the RED and IR LEDs
      P2DIR = BIT5 | BIT1;  // selects as outupts
      P2SEL = BIT5 | BIT1; // selecting PWM for BJTs

      int temp; // var that will be assigned to the value determmined from ADC


      // variables needed for collecting the average read value for each LED and the R calc
      float readsIR[samp_siz], sumIR,lastIR, start;
      float readsRED[samp_siz], sumRED,lastRED;


    // used for R calc and updates the arrays
    int readsIRMM[maxperiod_siz];
    int readsREDMM[maxperiod_siz];
    int ptrMM = 0;
    int i;
    for (i = 0; i < maxperiod_siz; i++) { // setting init vals to 0
        readsIRMM[i] = 0;
        readsREDMM[i]=0;
    }

    // setting init R ratio
    float IRmax=-1;
    float IRmin=700;
    float REDmax=-1;
    float REDmin=700;
    int red_ptr, ir_ptr, reader, cycle_count; // pointers needed for keeping track of arrays and data collection


    // setting init vals to 0
    for (i = 0; i < samp_siz; i++) { readsIR[i] = 0; readsRED[i]=0; }
    sumIR = 0; sumRED=0;
    ir_ptr = 0;
    red_ptr = 0;
    cycle_count = 0;
    n3 = 0;
    n2 = 0;
    reader = 0;

  while(1) // keeps going forever
  {

      if (reading_flag == 0){ //IR Reading
          while((p25_edge_flag == 0)); //red LED is off, IR on
          p25_edge_flag = 0; // sets flag off after it was high so it can be used again
          ADC10CTL0 |= ADC10SC; //start conversion
          while(ADC10CTL1 & ADC10BUSY); //wait until conversion completed
          ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
          temp = ADC10MEM; // assigns temp to the ADC val measured
          reader += temp; // adds it to reader for accumulating and the averaging
          n2++; //one more sample collected

          if (n2 >= 10){ // collecting IR readings for ten samples
                reader /= n2; //average over last 10
                sumIR -= readsIR[ir_ptr]; // taking away longest ago val
                sumIR += reader; // adding newest val
                readsIR[ir_ptr] = reader; //updating readings
                lastIR = sumIR / samp_siz; //averaging
                reading_flag = 1; // changes to read the Red LED
                P2IES &= ~(BIT3); // low to high transition
                P2IFG &=  ~(BIT3); // clear any pending interrupts
                p25_edge_flag = 0; // clears for future reading
                n2 = 0; // will start from scratch next time in this state
                reader = 0; // done reading IR
                ir_ptr++; // will subtract from longest ago val next time
                ir_ptr %= samp_siz; // for wrap around of array
          }

      } else if (reading_flag == 1){ // Red Reading
          while((p25_edge_flag == 0)); //red LED is on, IR off
          p25_edge_flag = 0; // sets flag off after it was high so it can be used again
          ADC10CTL0 |= ADC10SC; //start conversion
          while(ADC10CTL1 & ADC10BUSY); //wait until conversion completed
          ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
          temp = ADC10MEM; // assigns temp to the ADC val measured
          reader += temp; // adds it to reader for accumulating and the averaging
          n3++; //one more sample collected
      if (n3 >= 10){
          reader /= n3; //average over last 10
          sumRED -= readsRED[red_ptr]; // taking away longest ago val
          sumRED += reader; // adding newest val
          readsRED[red_ptr] = reader; //updating readings
          lastRED = sumRED / samp_siz;
          P2IES = (BIT3); // low to high transition
          P2IFG ^=  (BIT3); // clear any pending interrupts
          n3 = 0; // will start from scratch next time in this state
          reader = 0; // done reading IR
          red_ptr++; // will subtract from longest ago val next time
          red_ptr %= samp_siz; // for wrap around of array
          cycle_count++; // checks how many time went through IR and Red collection process
          if (cycle_count == 5){
               reading_flag = 2; //switched to calculations
               cycle_count = 0;
          } else{
               reading_flag = 0; //switch back to IR reads
          }
      }
      } else if (reading_flag == 2){ // calculations
          readsIRMM[ptrMM] = lastIR; // assigns newest reading
          readsREDMM[ptrMM] = lastRED; // assigns newest reading
          ptrMM++;
          ptrMM %= maxperiod_siz - 1; // for indexing purposes
          if(1){ // comparison and assigning max and mins for R (looking within arrays)
            IRmax = 0; IRmin=1023; REDmax = 0; REDmin=1023;
            for(i=0;i<maxperiod_siz;i++) {
              if( readsIRMM[i]> IRmax) IRmax = readsIRMM[i];
              if( readsIRMM[i]>0 && readsIRMM[i]< IRmin ) IRmin = readsIRMM[i];
              if( readsREDMM[i]> REDmax) REDmax = readsREDMM[i];
              if( readsREDMM[i]>0 && readsREDMM[i]< REDmin ) REDmin = readsREDMM[i];
            }
            R =  ( (REDmax-REDmin) / REDmin) / ( (IRmax-IRmin) / IRmin ); // max min caluclation
          }
          int status; // determines if reading is nonsensical
          if (R >= 1.2 || R <= 0.4){ // of outside .4 to 1.2, we have experimentally determined that the reading is with a finger off or bad
              status = 0;
          } else {
              status = 1;
          }

          // printing values through serial
          SpO2 = -10 * R + 104; // calibrated IR sensor slope and intercept
          ftoa(R, mv_char, 4); //UART the lastIR (average IR) value
          ser_output(mv_char);
          ser_output(charmemval5);
          ftoa(SpO2, mv_char, 4); //UART the lastIR (average IR) value
          ser_output(mv_char);
          ser_output(charmemval5);
          itoa(status, mv_char);
          ser_output(mv_char);
          ser_output(charreturn);

       reading_flag = 0; // send back to IR reading for new calc round
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
