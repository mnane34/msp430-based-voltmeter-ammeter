/*----------------------------------------------------------------------------------------*/

/*

             # Pin Definitions #

    P1.4 -> Current Sensor Data Pin

    P1.5 -> SCLK on Nokia 5110 LCD Pins
    P1.1 -> DC on Nokia 5110 LCD Pins
    P1.0 -> SCE on Nokia 5110 LCD
    P1.7 -> DN on Nokia 5110 LCD Pins

    P1.2 -> Voltage Probe

*/

/*----------------------------------------------------------------------------------------*/

/* Library Definitions */
#include <msp430.h> 
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "PCD8544.h"

/*----------------------------------------------------------------------------------------*/

/* Makro Definitions */
#define analogCH1 BIT2 // P1.2
#define analogCH2 BIT3 // NC
#define analogCH3 BIT4 // P1.4
#define bufferSIZE 5

/*----------------------------------------------------------------------------------------*/

/* Global Veriables Definitions */
volatile unsigned short analogValue[3]={0};
volatile char voltageBuffer[bufferSIZE]={0};
volatile char currentBuffer[bufferSIZE]={0};
volatile char powerBuffer[bufferSIZE]={0};

/*----------------------------------------------------------------------------------------*/

/* Function Prototype Definitions */
void delayMS(unsigned int Value);
void systemConfigrations();

/*----------------------------------------------------------------------------------------*/

/* Class Definitions */
class Signal{

private:
        /* Veriables Definitions */
        volatile float Voltage=0.0;
        volatile float Current=0.0;
        volatile float Power=0.0;

public:
        /* Function Prototype Definitions */
        void  setVoltage(volatile float Voltage);
        float getVoltage();

        void  setCurrent(volatile float Current);
        float getCurrent();

        void  setPower(volatile float Power);
        float getPower();

        void readSignal();
        void determineVoltage();
        void determineCurrent();
        void determinePower();
        void determineDigit();
        void showLCD();
};

/*----------------------------------------------------------------------------------------*/

/* Class Methods Definitions */
void Signal::setVoltage(volatile float Voltage){
    this->Voltage=Voltage;
};

float Signal::getVoltage(){
  return this->Voltage;
};

void Signal::setCurrent(volatile float Current){
    this->Current=Current;
};

float Signal::getCurrent(){
    return this->Current;
};


void Signal::setPower(volatile float Power){
    this->Power=Power;
};

float Signal::getPower(){
    return this->Power;
};


void Signal::readSignal(){

    ADC10CTL0 &= ~ENC;
    while (ADC10CTL1 & BUSY);
    ADC10SA = (unsigned int)&analogValue;
    ADC10CTL0 |= ENC + ADC10SC;
    delayMS(10);
}

void Signal::determinePower(){

    setPower(getVoltage() * getCurrent());

}

void Signal::determineVoltage(){

    /* Local Veriables Definitions */
    float inProbe=0;
    float fakeProbe=0.0;
    float Probe=0.0;
    const float gpioVoltage=3.3;
    const float lowResistance = 9800.0; // For Voltage Divider
    const float highResistance = 98000.0; // For Voltage Divider
    unsigned long int storeValue=0;

    // For filter
    for(int varLoop=0; varLoop<5; varLoop++){
       readSignal();
       storeValue += analogValue[2];
       delayMS(1);
       analogValue[2]=0;
    }


    // Signal converted to voltage
    inProbe = storeValue / 5.0 ;
    fakeProbe = ((inProbe*gpioVoltage)/(1023.0));
    Probe = fakeProbe / (lowResistance/(lowResistance+highResistance));
    delayMS(10);

    if(Probe < 1.0){
        setVoltage(0.0);
    }
    else{
        setVoltage(Probe);
    }

}

void Signal::determineCurrent(){

    /* Local veriables definitions */
    float rawValue;
    float rawVoltage=0.0;
    volatile float rawCurrent=0.0;
    unsigned long int storeValue=0;
    const float gpioVoltage=3.28;
    float gain = 6.45;
    float shunt = 0.1;

    // For filter
    for(int varLoop=0; varLoop<30; varLoop++){
       readSignal();
       storeValue += analogValue[0];
       delayMS(1);
       analogValue[0]=0;
    }

    /* Signal Converted to current*/
    rawValue = storeValue / 30.0 ;
    rawVoltage = rawValue * (gpioVoltage/1023.0) ;
    rawCurrent = (rawVoltage / gain) /  shunt ;

    if(rawCurrent < 0.1){
        setCurrent(0.0);
    }
    else{
        setCurrent(rawCurrent);
    }

}

void Signal::showLCD(){

    clearLCD();
    setAddr(0,0);  writeStringToLCD("U : ");
    setAddr(25,0); for(int i=0; i<4; i++){ writeCharToLCD(voltageBuffer[i]); delayMS(1);}
    setAddr(60,0); writeStringToLCD(" V");

    setAddr(0,2);  writeStringToLCD("I : ");
    setAddr(25,2); for(int i=0; i<5; i++){ writeCharToLCD(currentBuffer[i]); delayMS(1); }
    setAddr(60,2); writeStringToLCD(" A");

    setAddr(0,4);  writeStringToLCD("P : ");
    setAddr(25,4); for(int i=0; i<5; i++){ writeCharToLCD(powerBuffer[i]); delayMS(1); }
    setAddr(60,4); writeStringToLCD(" W");

    delayMS(250);
}

void Signal::determineDigit(){

    /* Local Veriables */
    char voltageDigits1[1]={0};
    char voltageDigits2[1]={0};
    char voltageDigits3[1]={0};
    int  fakeVoltage=0;

    char currentDigits1[1]={0};
    char currentDigits2[1]={0};
    char currentDigits3[1]={0};
    char currentDigits4[1]={0};
    int  fakeCurrent=0;

    char powerDigits1[1]={0};
    char powerDigits2[1]={0};
    char powerDigits3[1]={0};
    char powerDigits4[1]={0};
    int  fakePower=0;

    if(getVoltage() < 10.0){
        sprintf(voltageDigits2,"%d",(int)getVoltage());
        fakeVoltage = getVoltage()*10.0;
        sprintf(voltageDigits3,"%d",fakeVoltage%10);
        voltageBuffer[0]=voltageDigits2[0];
        voltageBuffer[1]='.';
        voltageBuffer[2]=voltageDigits3[0];
        voltageBuffer[3]=' ';
    }
    else if(getVoltage() >= 10.0){
        fakeVoltage = getVoltage()*10.0;
        sprintf(voltageDigits1,"%d",((int)fakeVoltage)/100);
        sprintf(voltageDigits2,"%d",(((int)fakeVoltage)%100)/10);
        sprintf(voltageDigits3,"%d",(((int)fakeVoltage)%10));
        voltageBuffer[0]=voltageDigits1[0];
        voltageBuffer[1]=voltageDigits2[0];
        voltageBuffer[2]='.';
        voltageBuffer[3]=voltageDigits3[0];
    }

    if((int)getCurrent() == 0){
        sprintf(currentDigits1,"%d",(int)getCurrent());
        fakeCurrent = getCurrent()*1000.0;
        sprintf(currentDigits2,"%d",fakeCurrent/100);
        sprintf(currentDigits3,"%d",((fakeCurrent%100)/10));
        sprintf(currentDigits4,"%d",fakeCurrent%10);

        currentBuffer[0]=currentDigits1[0];
        currentBuffer[1]='.';
        currentBuffer[2]=currentDigits2[0];
        currentBuffer[3]=currentDigits3[0];
        currentBuffer[4]=currentDigits4[0];
    }
    else{
        sprintf(currentDigits1,"%d",(int)getCurrent());
        fakeCurrent = getCurrent()*1000.0;
        sprintf(currentDigits2,"%d",(fakeCurrent/100)%10);
        sprintf(currentDigits3,"%d",((fakeCurrent%100)/10));
        sprintf(currentDigits4,"%d",fakeCurrent%10);

        currentBuffer[0]=currentDigits1[0];
        currentBuffer[1]='.';
        currentBuffer[2]=currentDigits2[0];
        currentBuffer[3]=currentDigits3[0];
        currentBuffer[4]=currentDigits4[0];
    }

    if((int)getPower() == 0){
        sprintf(powerDigits1,"%d",(int)getPower());
        fakePower = getPower()*1000.0;
        sprintf(powerDigits2,"%d",(fakePower/100)%10);
        sprintf(powerDigits3,"%d",((fakePower%100)/10));
        sprintf(powerDigits4,"%d",fakePower%10);

        powerBuffer[0]=powerDigits1[0];
        powerBuffer[1]='.';
        powerBuffer[2]=powerDigits2[0];
        powerBuffer[3]=powerDigits3[0];
        powerBuffer[4]=powerDigits4[0];
    }
    else if((int)getPower() != 0 && (int)getPower() <= 9){
        sprintf(powerDigits1,"%d",(int)getPower());
        fakePower = getPower()*1000.0;
        sprintf(powerDigits2,"%d",(fakePower/100)%10);
        sprintf(powerDigits3,"%d",((fakePower%100)/10));
        sprintf(powerDigits4,"%d",fakePower%10);

        powerBuffer[0]=powerDigits1[0];
        powerBuffer[1]='.';
        powerBuffer[2]=powerDigits2[0];
        powerBuffer[3]=powerDigits3[0];
        powerBuffer[4]=powerDigits4[0];
    }
    else if((int)getPower() > 9 && (int)getPower() <= 99){
        sprintf(powerDigits1,"%d",((int)getPower())/10);
        sprintf(powerDigits2,"%d",((int)getPower())%10);
        fakePower = getPower()*100.0;
        sprintf(powerDigits3,"%d",((fakePower%100)/10));
        sprintf(powerDigits4,"%d",fakePower%10);

        powerBuffer[0]=powerDigits1[0];
        powerBuffer[1]=powerDigits2[0];
        powerBuffer[2]='.';
        powerBuffer[3]=powerDigits3[0];
        powerBuffer[4]=powerDigits4[0];
    }

    else if((int)getPower() > 99 && (int)getPower() <= 999){
        sprintf(powerDigits1,"%d",((int)getPower())/100);
        sprintf(powerDigits2,"%d",(((int)getPower())/10)%10);
        sprintf(powerDigits3,"%d",((int)getPower())%10);
        fakePower = getPower()*10.0;
        sprintf(powerDigits4,"%d",fakePower%10);

        powerBuffer[0]=powerDigits1[0];
        powerBuffer[1]=powerDigits2[0];
        powerBuffer[2]=powerDigits3[0];
        powerBuffer[3]='.';
        powerBuffer[4]=powerDigits4[0];
    }
}

/*----------------------------------------------------------------------------------------*/

/* Main Functions Definitions */
int main(void)

{
    Signal Signal;

    systemConfigrations();

    while(1){

        Signal.determineVoltage();
        Signal.determineCurrent();
        Signal.determinePower();
        Signal.determineDigit();
        Signal.showLCD();
    }

}

/*----------------------------------------------------------------------------------------*/

/* Function Definitions */
void delayMS(unsigned int Value)
{
    while(Value)
    {
         __delay_cycles(1000);
        Value--;
    }
}

void systemConfigrations(){

    /* Watch Dog Timer & Clock Configrations */
    WDTCTL = WDTPW + WDTHOLD; // Disable WDT
    DCOCTL=CALDCO_1MHZ; // 1MHz Clock , Internal RC Oscillator
    BCSCTL1=CALBC1_1MHZ;

    /* Timer Configrations */
    CCTL0 = CCIE;                            // CCR0 interrupt enabled
    CCR0 = 50000;                            // for 50ms
    TACTL = TASSEL_2 + MC_2;                 // SMCLK, continues mode

    /* ADC Configrations */
    ADC10CTL1 = INCH_4 + CONSEQ_3;
    ADC10CTL0 = ADC10SHT_3 + MSC + ADC10ON;
    ADC10AE0 = analogCH1 + analogCH2  + analogCH3;
    ADC10DTC1 = 0x03;

    /* Nokia 5110 LCD Configrations */
    P1OUT |= LCD5110_SCE_PIN + LCD5110_DC_PIN;
    P1DIR |= LCD5110_SCE_PIN + LCD5110_DC_PIN;
    P1SEL |= LCD5110_SCLK_PIN + LCD5110_DN_PIN;
    P1SEL2 |= LCD5110_SCLK_PIN + LCD5110_DN_PIN;
    UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC; // 3-Pin, 8-bit SPI master
    UCB0CTL1 |= UCSSEL_2; // SMCLK
    UCB0BR0 |= 0x01; // 1:1
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; // Clear SW
    initLCD();
    clearLCD();
    delayMS(1000); // For initialize

    for(int varLoop=0; varLoop<5; varLoop++) // Ýnitialize
    {
        ADC10CTL0 &= ~ENC;
        while (ADC10CTL1 & BUSY);
        ADC10SA = (unsigned int)&analogValue;
        ADC10CTL0 |= ENC + ADC10SC;
        delayMS(1);
    }

    analogValue[0]=analogValue[1]=analogValue[2]=0;

}
