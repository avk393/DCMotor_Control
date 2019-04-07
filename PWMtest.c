// PWMtest.c
// Runs on TM4C123
// Use PWM0/PB6 and PWM1/PB7 to generate pulse-width modulated outputs.
// Daniel Valvano
// March 28, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 6.8, section 6.3.2

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015
   Program 8.4, Section 8.3

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
#include <stdint.h>
#include <stdio.h>
#include "PLL.h"
#include "PWM.h"
#include "PeriodMeasure.h"
#include "Blynk.h"
#include "Timer2.h"
#include "Timer3.h"
#include "ST7735.h"

void WaitForInterrupt(void);  // low power mode
//#define Size 64
//int periods[Size];
#define N 128
char *itoa (int32_t value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void InitDisplay(void)
{
	//ST7735_InitR(ST7735_CYAN);
	ST7735_SetCursor(0,0);
	ST7735_OutString("Desired Speed:");ST7735_OutUDec(getSpeed());
	ST7735_SetCursor(0,2);
	ST7735_OutString("Current Speed:");ST7735_OutUDec(getCalculatedSpeed(getPeriod()));
	ST7735_FillRect(0,30,128,130,ST7735_WHITE);
	ST7735_FillRect(0,0,128,2,0);	
}
/*
		//graphing
		int32_t current_graph = current_speed;
		int32_t desired_graph = desired_speed;
		ST7735_PlotPointBlack(current_graph);  // Measured speed
		ST7735_PlotPointBlue(desired_graph);  // Desired speed
		if((j&(N-1))==0){          // fs sampling, fs/N samples plotted per second
			ST7735_PlotNextErase();  // overwrites N points on same line
		}
		j++;     				              // counts the number of samples
*/
//xmax = 0-127
//ymax = 0-158
//y range for graph = 32-158
void UpdateDisplay(void)
{
	static int prev_desiredspeed;
	uint32_t graph_desiredspeed;
	int32_t MotorSpeed = getCalculatedSpeed(getPeriod());
	int32_t DesiredSpeed = getSpeed();
	ST7735_FillRect(0,0,128,2,ST7735_WHITE);
	ST7735_FillRect(0,0,128,2,0);
	ST7735_SetCursor(0,0);
	ST7735_OutString("Desired Speed:");ST7735_OutUDec(DesiredSpeed);
	ST7735_SetCursor(0,2);
	ST7735_OutString("Current Speed:");ST7735_OutUDec(MotorSpeed);
	
	graph_desiredspeed = (DesiredSpeed*-2) + 158;
	ST7735_Line(prev_desiredspeed, graph_desiredspeed, ST7735_BLUE);
	prev_desiredspeed = graph_desiredspeed;
}

int main(void){
  PLL_Init(Bus80MHz);               // bus clock at 80 MHz
//  PWM0A_Init(40000, 30000);         // initialize PWM0, 1000 Hz, 75% duty
	//PWM0B_Init(40000, 0);         // initialize PWM0, 1000 Hz, 0% duty
	PWM0B_Init(40000,39999);
	PeriodMeasure_Init();
	Output_Init();        // initialize ST7735

	Blynk_Init();
	Timer2_Init(&Blynk_to_TM4C,800000); 
// check for receive data from Blynk App 100ms
	Timer3_Init(&SendInformation,40000000); 
// Send data back to Blynk App every 1/2 second
	volatile int32_t speed_diff;
	volatile int32_t i, d, j;
	volatile uint16_t duty_cycle;
	volatile int32_t prev_speed;
	//int screen = 0;
	InitDisplay();


	while(1){
		int32_t desired_speed = getSpeed();
		uint32_t period = getPeriod();
		int32_t current_speed = getCalculatedSpeed(getPeriod());
		
		//100% duty cycle, period 4.3ms --> speed = 60 rps 
		//75% duty cycle, period 6.5ms --> speed = 38.5 rps
		speed_diff = desired_speed-current_speed;
		int32_t kp1 = getKP1();
		int32_t kp2 = getKP2();
		int32_t ki1 = getKI1();
		int32_t ki2 = getKI2();
		uint32_t tau = getTAU();
		
		i = (i+(ki1*speed_diff)/ki2);
		d = ((kp1*speed_diff)/kp2);
		if(i<-39999) i = -39999;
		else if(i>39999) i = 39999;
		
		if((d+i)> 39999){
			duty_cycle = 39999;
			//d = 39999;
			//i = 39999;
		} else if((d+i)<0) {
			duty_cycle = 0;
		} else{
			duty_cycle =  d+ i;
		}
		
		if(desired_speed<30){
			duty_cycle = 0;
			d = 0;
			i = 0;
		}
		
		PWM0B_Duty(duty_cycle);
		/*char iii[20];
		char ddd[20];
		char dutyy[20];
		char iiii[20];
		char dddd[20];
		char shit[20];
		char *ii = &iii[0];
		char *dd = & ddd[0];
		char *duty= &dutyy[0];
		char *ddddd = &dddd[0];
		char *iiiii = &iiii[0];
		char duty_c[20];
		char *duty_cy = &duty_c[20];
		char *shit1 = &shit[0];
		
		itoa(desired_speed,duty,10);
		itoa(current_speed,dd,10);
		itoa(speed_diff,ii,10);
		itoa(i,iiiii,10);
		itoa(d,ddddd,10);
		itoa(duty_cycle,duty_cy,10);
		itoa(period,shit1,10);
		
		ST7735_SetCursor(0,0);
		ST7735_OutString(duty);
		ST7735_SetCursor(0,2);
		ST7735_OutString(dd);
		ST7735_SetCursor(0,4);
		ST7735_OutString(ii);
		ST7735_SetCursor(0,6);
		ST7735_OutString(iiiii);
		ST7735_SetCursor(0,8);
		ST7735_OutString(ddddd);
		ST7735_SetCursor(0,10);
		ST7735_OutString(duty_cy);
		ST7735_SetCursor(0,12);
		ST7735_OutString(shit1);
		
		
		screen++;
		if(screen>=100) {
			screen=0;
			ST7735_FillScreen(0);
		}*/
		UpdateDisplay();
	}
}
