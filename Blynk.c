// TM4C123       ESP8266-ESP01 (2 by 4 header)
// PE5 (U5TX) to Pin 1 (Rx)
// PE4 (U5RX) to Pin 5 (TX)
// PE3 output debugging
// PE2 nc
// PE1 output    Pin 7 Reset
// PE0 input     Pin 3 Rdy IO2
//               Pin 2 IO0, 10k pullup to 3.3V  
//               Pin 8 Vcc, 3.3V (separate supply from LaunchPad 
// Gnd           Pin 4 Gnd  
// Place a 4.7uF tantalum and 0.1 ceramic next to ESP8266 3.3V power pin
// Use LM2937-3.3 and two 4.7 uF capacitors to convert USB +5V to 3.3V for the ESP8266
// http://www.ti.com/lit/ds/symlink/lm2937-3.3.pdf
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "PLL.h"
#include "Timer2.h"
#include "Timer3.h"
#include "UART.h"
#include "PortF.h"
#include "esp8266.h"
#include "PeriodMeasure.h"

void EnableInterrupts(void);    // Defined in startup.s
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s
int32_t getCalculatedSpeed(uint32_t period);
uint32_t getSpeed(void);

uint32_t LED;      
volatile uint32_t speed;	// VP1
volatile uint32_t kp1;		// VP2
volatile uint32_t kp2;		// VP3
volatile uint32_t ki1;		// VP4
volatile uint32_t ki2;		// VP5
volatile uint32_t tau;		// VP6
uint32_t LastF;    // VP74
// These 6 variables contain the most recent Blynk to TM4C123 message
// Blynk to TM4C123 uses VP0 to VP15
char serial_buf[64];
char Pin_Number[2]   = "99";       // Initialize to invalid pin number
char Pin_Integer[8]  = "0000";     //
char Pin_Float[8]    = "0.0000";   //
uint32_t pin_num; 
uint32_t pin_int;
 
// ----------------------------------- TM4C_to_Blynk ------------------------------
// Send data to the Blynk App
// It uses Virtual Pin numbers between 70 and 99
// so that the ESP8266 knows to forward the data to the Blynk App
void TM4C_to_Blynk(uint32_t pin,uint32_t value){
  if((pin < 70)||(pin > 99)){
    return; // ignore illegal requests
  }
// your account will be temporarily halted if you send too much data
  ESP8266_OutUDec(pin);       // Send the Virtual Pin #
  ESP8266_OutChar(',');
  ESP8266_OutUDec(value);      // Send the current value
  ESP8266_OutChar(',');
  ESP8266_OutString("0.0\n");  // Null value not used in this example
}
 
 
// -------------------------   Blynk_to_TM4C  -----------------------------------
// This routine receives the Blynk Virtual Pin data via the ESP8266 and parses the
// data and feeds the commands to the TM4C.
void Blynk_to_TM4C(void){int j; char data;
// Check to see if a there is data in the RXD buffer
  if(ESP8266_GetMessage(serial_buf)){  // returns false if no message
    // Read the data from the UART5
#ifdef DEBUG1
    j = 0;
    do{
      data = serial_buf[j];
      UART_OutChar(data);        // Debug only
      j++;
    }while(data != '\n');
    UART_OutChar('\r');        
#endif
           
// Rip the 3 fields out of the CSV data. The sequence of data from the 8266 is:
// Pin #, Integer Value, Float Value.
    strcpy(Pin_Number, strtok(serial_buf, ","));
    strcpy(Pin_Integer, strtok(NULL, ","));       // Integer value that is determined by the Blynk App
    strcpy(Pin_Float, strtok(NULL, ","));         // Not used
    pin_num = atoi(Pin_Number);     // Need to convert ASCII to integer
    pin_int = atoi(Pin_Integer);  
  // ---------------------------- VP #1 ----------------------------------------
  // This VP is the LED select button
    if(pin_num == 0x01)  {
			speed = pin_int;
      LED = pin_int;
      PortF_Output(LED<<2); // Blue LED
    }                               // Parse incoming data    
		if(pin_num == 0x02)  {
			kp1 = pin_int;
    } 
		if(pin_num == 0x03)  {
			kp2 = pin_int;
    } 
		if(pin_num == 0x04)  {
			ki1 = pin_int;
    } 
		if(pin_num == 0x05)  {
			ki2 = pin_int;
    } 
		if(pin_num == 0x06)  {
			tau = pin_int;
    } 
#ifdef DEBUG1
    UART_OutString(" Pin_Number = ");
    UART_OutString(Pin_Number);
    UART_OutString("   Pin_Integer = ");
    UART_OutString(Pin_Integer);
    UART_OutString("   Pin_Float = ");
    UART_OutString(Pin_Float);
    UART_OutString("\n\r");
#endif
  }  
}

void SendInformation(void){
	uint32_t speed = getCalculatedSpeed(getPeriod());
	if(getSpeed()<30) speed = 0;
	TM4C_to_Blynk(70, speed);  // VP70
	TM4C_to_Blynk(71, speed);  // VP71
#ifdef DEBUG3
		Output_Color(ST7735_WHITE);
		ST7735_OutString("Send 70/71 data=");
		ST7735_OutUDec(speed);
		ST7735_OutChar('\n');
#endif
}

  
void Blynk_Init(void){       
  PortF_Init();
  LastF = PortF_Input();
#ifdef DEBUG3
  ST7735_OutString("EE445L Lab 4D\nBlynk example\n");
#endif
#ifdef DEBUG1
  UART_Init(5);         // Enable Debug Serial Port
  UART_OutString("\n\rEE445L Lab 4D\n\rBlynk example");
#endif
  ESP8266_Init();       // Enable ESP8266 Serial Port
  ESP8266_Reset();      // Reset the WiFi module
  ESP8266_SetupWiFi();  // Setup communications to Blynk Server  
  
}

uint32_t getSpeed(void){
	return speed;
}

uint32_t getKP1(void){
	return kp1;
}

uint32_t getKP2(void){
	return kp2;
}

uint32_t getKI1(void){
	return ki1;
}

uint32_t getKI2(void){
	return ki2;
}

uint32_t getTAU(void){
	return tau;
}

int32_t getCalculatedSpeed(uint32_t period){
	int32_t speed = period*4;
	speed = 80000000/speed;
	if(period==0) speed=0;
	else if(period>=650000) speed = 0;
	else if(speed>60 && speed<1000) speed = 60;
	else if(speed>=1000) speed = 0;
	return speed;
}
