#include "msp.h"
#include <stdio.h>
#include <stdlib.h>


//void Timer32Init(int period);
//void ToggleInterface();

void choice1();
void choice2();
void choice3();

void ADCInit(void);
void UARTInit(void);
void TX(char text[]);
int RX(void);
float tempRead(void);
void myTimer(int time);
void ADCInit(void)
{
	//Ref_A settings
	REF_A ->CTL0 &= ~0x8; //enable temp sensor
	REF_A ->CTL0 |= 0x30; //set ref voltage
	REF_A ->CTL0 &= ~0x01; //enable ref voltage
	//do ADC stuff
	ADC14 ->CTL0 |= 0x10; //turn on the ADC
	ADC14 ->CTL0 &= ~0x02; //disable ADC
	ADC14 ->CTL0 |=0x4180700; //no prescale, mclk, 192 SHT
	ADC14 ->CTL1 &= ~0x1F0000; //configure memory register 0
	ADC14 ->CTL1 |= 0x800000; //route temp sense
	ADC14 ->MCTL[0] |= 0x100; //vref pos int buffer
	ADC14 ->MCTL[0] |= 22; //channel 22
	ADC14 ->CTL0 |=0x02; //enable adc
	return;
}

//format
void UARTInit(void)
{
EUSCI_A0 ->CTLW0 |= 1;
EUSCI_A0 ->MCTLW = 0;
EUSCI_A0 ->CTLW0 |= 0x80;
EUSCI_A0 ->BRW = 0x34;
EUSCI_A0 ->CTLW0 &= ~0x01;
P1->SEL0 |= 0x0C;
P1->SEL1 &= ~0x0C;
return;
}

void TX(char text[])
{
	int i =0;
	while(text[i] != '\0')
	{
		EUSCI_A0 ->TXBUF = text[i];
		while((EUSCI_A0 ->IFG & 0x02) == 0)
			{
				//wait until character sent
			}
			i++;
	}
	return;
}


int RX(void)
{
	int i = 0;
	char command[2];
	char x;
	while(1)
	{
		if((EUSCI_A0 ->IFG & 0x01) !=0) //data in RX buffer
			{
				command[i] = EUSCI_A0 ->RXBUF;
				EUSCI_A0 ->TXBUF = command[i]; //echo
				while((EUSCI_A0 ->IFG & 0x02)==0); //wait
				if(command[i] == '\r')
				{
					command[i] = '\0';
					break;
				}
				else
				{
					i++;
				}
		}
	}
	x = atoi(command);
	TX("\n\r");
	return x;
}
int main(void){
	ADCInit();
	UARTInit();
	
	
	//ADCInit();
	
	while(1){
		int userC;
		TX("\nMSP432 Menu \n\r1. RGB Control\n\r2. Digital Input\n\r3. Temperature Reading\n\r");
		userC = RX();
		
		switch(userC){
			case 1:
				choice1();
				break;
			case 2:
				choice2();
				break;
			case 3:
				choice3();
				break;
			default:
				TX("Invalid option\n\r");
	}
		
	}
	
	
	
	while(1);
}

void choice1(){
	int rgb;
	int toggleTime;
	int numBlinks;
	float const1 = 3000000;
	
	TX("Enter Combination of RGB (1 - 7):");
	rgb = RX();
	
	if(rgb > 7 || rgb < 1){
		rgb = 7;
	}
		
	TX("Enter Toggle Time: ");
	toggleTime = RX();
	
	TX("Enter Number of Blinks:");
	numBlinks = RX();
	
	P2 -> SEL0 &= ~rgb;
	P2 -> SEL1 &= ~rgb;
	P2 -> DIR |= rgb;
	//P2 -> OUT &= ~7;
	
	TIMER32_1 ->LOAD = toggleTime * const1 -1;//setting up timer
	TIMER32_1 ->CONTROL |= 0x42; // no interuptions
	P2 -> OUT &= ~rgb; // turn pin to 0
	
	//P2 -> OUT |= rgb; // turning on the pin
	for(int i = 0; i < numBlinks; i++){
		P2 ->OUT |= rgb;//pin on
		TIMER32_1 -> CONTROL |= 0x80; //enable timer
		
		while((TIMER32_1-> RIS & 0x1)!=1){}
		TIMER32_1 -> INTCLR &= 0x80; // clear the count flag
		P2 -> OUT &= ~rgb; //turn off pin	
		
		while((TIMER32_1-> RIS & 0x1)!=1){}	
		TIMER32_1 -> INTCLR &= 0x80; // clear the count flag
	}
	TIMER32_1 -> CONTROL &= ~0x80; //disable timer
}

/*
void myTimer(int time){
	float const1 = 3000000;
	TIMER32_1 ->LOAD = time * const1 -1;//setting up timer
	TIMER32_1 ->CONTROL |= 0x42; // no interuptions
	//P2 -> OUT &= ~rgb; // turn pin to 0	
}
*/

void choice2(){
	P1 -> SEL0 &= ~18;
	P1 -> SEL1 &= ~18;
	P1 -> DIR &= ~18;
	P1 -> REN |= 18;
	P1 -> OUT |= 18;
	
	if((P1 -> IN & 2) == 0){
		if((P1 -> IN & 16) == 0){
			TX("Button 1 and 2 pressed.\n\r");
		}
		else{
			TX("Button 1 pressed.\n\r");
		}
	}
	else if((P1 -> IN & 16) == 0){
		TX("Button 2 pressed.\n\r");
	}
	else{
		TX("No Button pressed.\n\r");
	}
}

float tempRead(void)
{
	float temp; //temperature variable
	uint32_t cal30 = TLV ->ADC14_REF2P5V_TS30C; //calibration constant
	uint32_t cal85 = TLV ->ADC14_REF2P5V_TS85C; //calibration constant
	float calDiff = cal85 - cal30; //calibration difference
	ADC14 ->CTL0 |= 0x01; //start conversion
	while((ADC14 ->IFGR0) ==0)
	{
	//wait for conversion
	}
	temp = ADC14 ->MEM[0]; //assign ADC value
	temp = (temp - cal30) * 55; //math for temperature per manual
	temp = (temp/calDiff) + 30; //math for temperature per manual
	return temp; //return temperature in degrees C
	}

void choice3(){ //systick timer?
	int numReading;
	
	TX("Enter Number of Temperature Reading (1-5):\n\r");
	numReading = RX();
	
	if(numReading > 5 || numReading < 0){
		numReading = 5;
	}
	float c;
	float f;
	char word[200];
	
	for(int i = 1; i <= numReading; i++){
	
		SysTick -> LOAD = 3000000-1; //delay 1 second
		SysTick -> CTRL |= 0x4; //CLK, disabled
	
		SysTick ->CTRL |= 0x1; //enable clock
	
		while((SysTick ->CTRL & 0x10000)==0){//while counterflag not set
		// waiting 1 second
		}
		c = tempRead();
		f = c * (9.0/5.0) + 32.0;
		sprintf(word, "Reading %d: %.2f C & %.2f F\n\r",i,c,f);
		TX(word);
		
	}
	
	}





