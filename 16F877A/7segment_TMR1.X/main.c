#include <xc.h>
//#include "bk300.h"

// CONFIG
#pragma config FOSC = XT        // Oscillator Selection bits (XT oscillator)
#pragma config WDTE = OFF        // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF       // Power-up Timer Enable bit (PWRT enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#define _XTAL_FREQ 4000000

#define uchar  unsigned char
#define uint   unsigned int

//7seg - digit mapping
uchar LED_DIS[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};   
/*
 * 11000000 0xc0 0
 * 11111001 0xf9 1
 * 10100100 0xa4 2
 * 10110000 0xb0 3
 * 10011001 0x99 4
 * 10010010 0x92 5
 * 10000010 0x82 6
 * 11111000 0xf8 7
 * 10000000 0x80 8
 * 10010000 0x90 9
 * 11111111 0xff NULL
*/

#define Strobe8Leds() RC5=1;asm("NOP");RC5=0;
#define Strobe7Seg() RC3=1;asm("NOP");RC3=0;
#define Strobe7SegTubeStepper() RC4=1;asm("NOP");RC4=0;


uchar DIS_NUM[4];   // store 7segData
uint tmr0_counter = 0;
uint tmr1_counter = 0;
uchar disDigitCounter = 0;  // store which 7segTube to update next


void Board_init(void) {
    TRISA=0x00;	   
    TRISB=0x00;
    TRISC=0x00;
    TRISD=0x00;
    TRISE=0X00;

    RA3=0;  // nOE   
    // off 8 LEDs
    PORTD=0xff;  
    Strobe8Leds();
    // clear 7seg buffer
    PORTD=0xff;
    Strobe7Seg();     
    // disable 7seg tubes
    PORTD = 0x00;    
    Strobe7SegTubeStepper();    
    
    PORTD=0xff;
    RC2=1;        // PWM LED off
    RE0=0;        // buzzer off 
    RE1=0;        // relay off
    RE2=0;        //MAX485 as receive
}

void RefreshLedDisplayDigit(uchar tube) {
    // select 7seg tube
    PORTD = (uchar)(0x80 >> tube);
    Strobe7SegTubeStepper();
    // give it number
    PORTD = LED_DIS[DIS_NUM[tube]];
    Strobe7Seg();
    // give LED time to reach full brightness
    __delay_ms(5); 
    // disable 7seg tube
    PORTD=0x00;
    Strobe7SegTubeStepper();
    __delay_us(5);
}

void UpdateLedDisplay(void) {
    RA3=0; // nOE
    for(uchar i=0;i<4;i++) RefreshLedDisplayDigit(i);
}


void __interrupt() TMR0INT() {
    if (TMR0IF) {
        TMR0IF = 0;
        tmr0_counter++;
        PORTD = ~(tmr0_counter & 0x0ff);
        Strobe8Leds();
        TMR0 = 0x00;
    }
    if (TMR1IF) {
        TMR1IF = 0;
        tmr1_counter++;
        if(tmr1_counter == 5) {
            tmr1_counter = 0;
            RefreshLedDisplayDigit(disDigitCounter);
            disDigitCounter++;
            if (disDigitCounter > 3) disDigitCounter = 0;
        }
        TMR1H = 0xff;
        TMR1L = 0xe0;  
    }
}

void ConvertDecimalTo7Seg(uint val) {
    DIS_NUM[0]=(uchar)(val/1000);          //???????
    DIS_NUM[1]=(uchar)((val%1000)/100);    //??
    DIS_NUM[2]=(uchar)((val%100)/10);      //??
    DIS_NUM[3]=(uchar)(val%10);            //??    
}

void main (void) {
    uint counter;
    // interrupt config
    INTCON = 0xe0;  // GIE on, PEIE on, TMR0IE on, INTE off, RBIE off
    // TMR0 config
    OPTION_REG = 0x07;  // TMR0 prescaler 1:256
    TMR0 = 0x00;
    // TMR1 config
    TMR1IE = 1;  // TMR1 interrupt enable    
    TMR1H = 0xff;
    TMR1L = 0xfb;  
    T1CON = 0x31;   // 1:8 prescale, TMR1ON
    Board_init();

    while(1)
    {
        for(counter=0;counter<9999;counter++)
        {
            ConvertDecimalTo7Seg(counter);
            UpdateLedDisplay();
            __delay_ms(5);
        }        
    }  
}




