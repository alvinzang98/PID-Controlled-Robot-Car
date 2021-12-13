// Peripherals initilization

#include "stm32f10x.h"
//#include "PID.h"
#include "init.h"
#include "pinmap.h"
#include "stdbool.h"
//#include "stdlib.h"
//#include "string.h"
								//******************************************************************/
								//  Wheel PWM Pinout      Wheel Counter Pinout   SWITCH S1   PB8   */
int CountRight = 0;				//  RENBL TIM3_CH1 PA6    LEFT  TIM4_CH1 PB6     LED         PB7   */
int CountLeft = 0;				//  LENBL TIM3_CH2 PA7    RIGHT TIM2_CH2 PA1     SPI2_MISO   PB14  */
int TimerCount = 0;				//        *               RPHASE         PC15          *           */
float MAX = 120;		        //      *   *             LPHASE         PA0         *   *         */
bool open = false;				//    *       *                                    *       *       */
bool RecAllow = false;			//     *     *  **********************************  *     *  *******/
								//      * * *                                        * * *
//float PID_Update(int,int);

void RCC_ENABLE_ALL(){
	
	//USART 2 for communication with WIFI
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	//USART 2 for TEST ONLY
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	//Timer 2 for counting the PID control period
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	//Timer 3 for generating PWM
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//PA0 for Left WHeel Rotation, PA1 for Right Wheel Counter, PA6 for Right Wheel PWM, PA7 for Left Wheel PWM 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	//PB6 for Left Wheel Counter, PB7 for LED, PB8 for Switch 1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	//PC1 for Right Wheel Counter
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//Timer 3 PWM, EXTI 1,6,8, 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
}

void TIM3_PWM_CH12_Init(){
	//TIM3 PWM Output

	GPIO_InitTypeDef GPIO_InitStruct;//PA6 PA7
	GPIO_InitStruct.GPIO_Pin = TIM34_CH1_PWM_PIN | TIM3_CH2_PWM_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(TIM3_CH12_PWM_GPIO, &GPIO_InitStruct);
	
	TIM_TimeBaseInitTypeDef timerInitStruct;
	timerInitStruct.TIM_Prescaler = 720-1;
	timerInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStruct.TIM_Period = 200-1;
	timerInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &timerInitStruct);
	//TIM_Cmd(TIM3, ENABLE);
	
	TIM_OCInitTypeDef outputChannelInit;
	outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
	outputChannelInit.TIM_Pulse = 0;
	outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
	outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &outputChannelInit);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2Init(TIM3, &outputChannelInit);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	
		
}

void TIM2_COUNT_Init(){ //Interrupt every 10ms
	//Timer 2 set up 
	
	TIM_TimeBaseInitTypeDef timerInitStructure; 
	timerInitStructure.TIM_Prescaler = 14400;  //1/(72Mhz/14400)=0.2ms
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 5;  //0.2ms*5 = 1ms
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 1;
	TIM_TimeBaseInit(TIM2, &timerInitStructure);
	TIM_Cmd(TIM2, ENABLE);

	//Enable update event for Timer2
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//Enable 10ms Interrupt
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;    //TIM4 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //Preemptive priority level 1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //From the priority level 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //The IRQ channel is enabled
    NVIC_Init(&NVIC_InitStructure);
    	
}

void LEFT_PWM_Update(float pwm){
	pwm = MAX < pwm? MAX:pwm;
	pwm = 0 > pwm? 0:pwm;
	//pwm = pwm<1? 1:pwm;
	TIM_OCInitTypeDef outputChannelInit;
	outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
	outputChannelInit.TIM_Pulse = (int)pwm;
	outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
	outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM3, &outputChannelInit);//×ó±ßchannel 2 £¡£¡£¡£¡
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
}

void RIGHT_PWM_Update(float PWM){
	PWM = MAX < PWM? MAX:PWM;
	PWM = 0 > PWM? 0:PWM;
	//PWM = PWM<1? 1:PWM;
	TIM_OCInitTypeDef outputChannelInit;
	outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
	outputChannelInit.TIM_Pulse = (int)PWM;
	outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
	outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &outputChannelInit);//ÓÒ±ßchannel 1 £¡£¡£¡£¡
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
}
/*
void EXTI1_CAPTURE_COUNT_Init(){
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// Configure I/O for PA1
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	// EXTI Configuration
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
	EXTI_InitTypeDef EXTI_InitStruct;
	EXTI_InitStruct.EXTI_Line = EXTI_Line1;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
	
	//ENABLE THE INTERRUPT
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	 NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStruct);
	
}

void EXTI6_CAPTURE_COUNT_Init(){
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// Configure I/O for PB6
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	// EXTI Configuration FOR LEFT PB6
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
	EXTI_InitTypeDef EXTI_InitStruct;
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
	
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStruct);
	
}
*/



void EXTI_SWITCH_Init(){//switch pb8
	GPIO_InitTypeDef GPIO_InitStruct;
	// Configure I/O for EXTI8
	GPIO_InitStruct.GPIO_Pin = Switch_1;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	
	// EXTI Configuration
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
	EXTI_InitTypeDef EXTI_InitStruct;
	EXTI_InitStruct.EXTI_Line = EXTI_Line8;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
	
	// Enable Interrupt
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_Init(&NVIC_InitStruct);
}

void LED(){
	//PB7
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
	 
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//PC15
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStruct);
	
	//PA0
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
	GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_SET);
	GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);
	
}




void TIM2_IRQHandler(void) {//
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TimerCount++;
		if (TimerCount == 100)
			RecAllow = true;
		/*100ms interval for receive a info!!!! and may process here*/
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}                                                 
}

void EXTI9_5_IRQHandler(void) {
	/*if (EXTI_GetITStatus(EXTI_Line6) != RESET) {//capture left wheel counter	
		CountLeft++;
		EXTI_ClearITPendingBit(EXTI_Line6);
	}*/
	
	if (EXTI_GetITStatus(EXTI_Line8) != RESET) {							
		
		open = true;
		
      	EXTI_ClearITPendingBit(EXTI_Line8);
    }
}
/*
void EXTI1_IRQHandler(void) {//Capture Right wheel counter
	if (EXTI_GetITStatus(EXTI_Line1) != RESET) {
		CountRight++;
		EXTI_ClearITPendingBit(EXTI_Line1);
	}
}*/


