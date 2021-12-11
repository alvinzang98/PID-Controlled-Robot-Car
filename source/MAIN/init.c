// Peripherals initilization

#include "stm32f10x.h"
#include "PID.h"
#include "stdbool.h"

#define TIM3_CH12_PWM_GPIO		GPIOA
#define TIM34_CH31_PWM_GPIO		GPIOB

#define TIM3_CH3_PWM_PIN		GPIO_Pin_0
#define TIM2_CH2_COUNTER_PIN    GPIO_PIN_1
#define TIM34_CH1_PWM_PIN		GPIO_Pin_6
#define TIM3_CH2_PWM_PIN	    GPIO_Pin_7
#define Switch_1                GPIO_Pin_8
#define SPI2_NSS                GPIO_Pin_12
#define SPI2_SCK                GPIO_Pin_13
#define SPI2_MISO               GPIO_Pin_14
#define SPI2_MOSI               GPIO_Pin_15 //PB15

//int countleft=0;		        //******************************************************************/
//int countright=0;		        //  Wheel PWM Pinout      Wheel Counter Pinout   SWITCH S1   PB8   */
//int timeperiod=0;		        //  RENBL TIM3_CH1 PA6    LEFT  TIM4_CH1 PB6     LED         PB7   */
bool state=false;		        //  LENBL TIM3_CH2 PA7    RIGHT TIM2_CH2 PA1     SPI2_MISO   PB14  */
bool open = true;		        //        *               RPHASE         PC15          *           */
float MAX = 180;		        //      *   *             LPHASE         PA0         *   *         */
float startspeedL = 117;          //    *       *                                    *       *       */
float startspeedR = 117;
char SPI_Detect0;	            //     *     * *********************************    *     *  *******/
char SPI_Detect;		        //      * * *                                        * * *
char SPI_Detect2;
int count =0;
bool flag =true;
int changeperiod =0;
bool delayopen=false;
int countblack=0;
int delayperiod = 0;
extern float PID_Update(float);

void RCC_ENABLE_ALL(void);              //
void TIM3_PWM_CH12_Init(void);          // Output PWM to two wheels
void TIM2_COUNT_Init(void);             // Count the time, and interrupt every 10ms
void LEFT_PWM_Update(float );			// Update the left wheel pwm
void RIGHT_PWM_Update(float );			// Update the right wheel pwm 
void EXTI1_CAPTURE_COUNT_Init(void );   // RIGHT Wheel COUNTER with external source
void EXTI6_CAPTURE_COUNT_Init(void);    // LEFT Wheel COUNTER with external source
void EXTI_SWITCH_Init(void);            // Switch is PB8
void LED(void);                         // Flashing LED
void SPI2_Init(void);                   // Initialize the SPI2
void Read_Data(void);                   // Get the data from SPI
void Direction_Modify(void);            // Apply PID to modify the direction


void RCC_ENABLE_ALL(){
	
	//SPI2 for getting the signals of phototransistors
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
	//Timer 2 for counting the PID control period
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	//Timer 3 for generating PWM
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//Timer 4 for counting time of turning right and left as well as rotate
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
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
	timerInitStruct.TIM_RepetitionCounter = 3;
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
	timerInitStructure.TIM_Prescaler = 28800;  //1/(72Mhz/14400)=0.2ms
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 1;  //0.2ms*2 = 0.4ms
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 1;
	TIM_TimeBaseInit(TIM2, &timerInitStructure);
	TIM_Cmd(TIM2, ENABLE);

	//Enable update event for Timer2
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	//Enable 10ms Interrupt
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;    //TIM4 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; //Preemptive priority level 0
    //NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //From the priority level 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //The IRQ channel is enabled
    NVIC_Init(&NVIC_InitStructure);
    	
}

void LEFT_PWM_Update(float pwm){
	pwm = MAX < pwm? MAX:pwm;
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
	//PWM = PWM<1? 1:PWM;
	TIM_OCInitTypeDef outputChannelInit;
	outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
	outputChannelInit.TIM_Pulse = (int)PWM;
	outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
	outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &outputChannelInit);//ÓÒ±ßchannel 1 £¡£¡£¡£¡
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
}

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
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStruct);
	
	/*
	//Tim4 RCC enable for left PB6
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
 
    TIM_TimeBaseInitTypeDef timerInitStructure; 
    timerInitStructure.TIM_Prescaler = 0;  
    timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    timerInitStructure.TIM_Period = 1;  
    timerInitStructure.TIM_ClockDivision = 0;
    timerInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &timerInitStructure);
    TIM_Cmd(TIM4, ENABLE);
	
	TIM_TIxExternalClockConfig(TIM4, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Rising, 0);
	
	//Enable Input Capture Interrupt
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;    //TIM4 interrupt
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //Preemptive priority level 2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //From the priority level 0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //The IRQ channel is enabled
    NVIC_Init(&NVIC_InitStructure);
    
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //Allow updates to interrupt, allows the CC1IE to capture interrupt*/
}


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
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
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
	
	GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
	GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_SET);
	GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);
	
}

void SPI2_Init(){
	
	GPIO_InitTypeDef GPIO_InitStructure;  
	GPIO_InitStructure.GPIO_Pin = SPI2_SCK | SPI2_MOSI ; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;   
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//NSS and CLK are output 
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = SPI2_MISO;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //MISO, signls transferred from 74HC299 to STM32
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = SPI2_NSS;//Ñ¡Æ¬
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//SPI Configuration  
	SPI_InitTypeDef  SPI_InitStructure;  
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;   
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;   
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;   
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;   
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;   
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;   
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;   
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;    
	SPI_Init(SPI2, &SPI_InitStructure); 
    SPI_Cmd(SPI2, ENABLE);
    
	
}

void TIM4_COUNT_Init(){
	
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef timerInitStructure; 
	timerInitStructure.TIM_Prescaler = 18000-1;  //1/(72Mhz/18000)=0.25ms
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 4000-1;  //0.25ms*4000 = 1s
	timerInitStructure.TIM_ClockDivision = 0; //TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &timerInitStructure);
	TIM_Cmd(TIM4, ENABLE);
	
	//Enable update event for Timer4
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	//Enable 10ms Interrupt
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;    //TIM4 interrupt
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //Preemptive priority level 0
 // NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //From the priority level 0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //The IRQ channel is enabled
  NVIC_Init(&NVIC_InitStructure);

}

void Read_Data(){
		while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
		
		GPIO_WriteBit(GPIOB,GPIO_Pin_12,Bit_SET);
		SPI_I2S_SendData(SPI2,'A');
		while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
		SPI_Detect0 = SPI_I2S_ReceiveData(SPI2);  
		SPI_I2S_ClearITPendingBit(SPI2, SPI_I2S_FLAG_RXNE); 
		
		GPIO_WriteBit(GPIOB,GPIO_Pin_12,Bit_RESET);
		SPI_I2S_SendData(SPI2,'A');
		while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
		SPI_Detect = SPI_I2S_ReceiveData(SPI2);   
		SPI_I2S_ClearITPendingBit(SPI2, SPI_I2S_FLAG_RXNE);
		

		if(SPI_Detect ==0 && SPI_Detect2!=0 && delayopen==false){
			//if(SPI_Detect==0){
			countblack++;
			delayopen=true;
		}
		
		/*if(countblack == 5 && open ==true){
			open=false;
			RIGHT_PWM_Update(startspeed*0.8 - PID_Update(5));
			LEFT_PWM_Update(startspeed*0.9 +PID_Update(5));
		}
		else if (countblack == 6 && open ==true){
			open=false;
			RIGHT_PWM_Update(0);
			LEFT_PWM_Update(startspeed -PID_Update(2));
		}*/
		SPI_Detect2=SPI_Detect;
}
void Direction_Modify(){
	
		if(((SPI_Detect & 0x01)== 0) && flag ==true){          //T1 becomes 0, turn left				
			LEFT_PWM_Update(startspeedL - PID_Update(6.5));
			RIGHT_PWM_Update(startspeedR +PID_Update(3.5));		
		}
		else if(((SPI_Detect) & 0x80) == 0 ){ //T8 becomes 0 turn right
			RIGHT_PWM_Update(startspeedR - PID_Update(8));
			LEFT_PWM_Update(startspeedL +PID_Update(4));
		}
		else if(((SPI_Detect<<1) & 0x80) == 0 && flag ==true){ //T7 becomes 0 turn right
			RIGHT_PWM_Update(startspeedR - PID_Update(5));
			LEFT_PWM_Update(startspeedL +PID_Update(2));
		}
		else if(((SPI_Detect>>1) & 0x01) == 0){ //T2 becomes 0 turn left
			LEFT_PWM_Update(startspeedL - PID_Update(4.5 ));
			RIGHT_PWM_Update(startspeedR +PID_Update(1));		
		}
		else if(((SPI_Detect>>2) & 0x01) == 0){ //T3 becomes 0 turn left
			LEFT_PWM_Update(startspeedL - PID_Update(1));
			RIGHT_PWM_Update(startspeedR);		
		}
		else if(((SPI_Detect<<2) & 0x80) == 0){ //T6 becomes 0 turn right
			RIGHT_PWM_Update(startspeedR - PID_Update(1));
			LEFT_PWM_Update(startspeedL );
		}
		/*if (countblack != 5)
		{
		for (int i = 0; i < 8; i++)
		{
			if ((SPI_Detect & (1<<i)) == 0)
			{
				LEFT_PWM_Update(startspeed + (int)(1.5 * PID_Update(i - 3)));	
				RIGHT_PWM_Update(startspeed - PID_Update(i - 3));	
				break;
			}
		}
		}
		else{
				for (int i = 7; i > -1; i++)
		{
			if ((SPI_Detect & (1<<i)) == 0)
			{
				LEFT_PWM_Update(startspeed + (int)(1.8 * PID_Update(i - 4)));	
				RIGHT_PWM_Update(startspeed - PID_Update(i - 4));	
				break;
			}
		}
		}*/
	
}

void TIM4_IRQHandler(void) {//0.1s
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET) {
		if (state ==true && delayopen){
			changeperiod++;
			if(countblack == 5 ){
				flag =false;
				//startspeedL=110;
				//startspeedR=80;
				/*open=false;
				RIGHT_PWM_Update(startspeed*0.7 - PID_Update(3.5));
				LEFT_PWM_Update(startspeed*0.6 +PID_Update(4.5));*/
			}
			else if (countblack == 6){
				changeperiod=0;
				delayopen =false;//°ÑÍùÓÒµÄµ÷½ÚÍ£Ö¹£¡£¡
				open =true;
			}
			else if (countblack == 7){
				open=false;		
				//GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_RESET);				
				LEFT_PWM_Update(8);
				RIGHT_PWM_Update(112);
				//startspeed=100;
			}
			
			if(changeperiod ==2){
				delayopen =false;
				changeperiod=0;
				open =true;
				if (countblack == 7){
					flag =false;
					//GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_SET);
					startspeedR = 115;
					startspeedL = 115; 
				}
				else if (countblack == 5){
					flag=true;
					startspeedR = 110;
					startspeedL = 110;
				}
				else if (countblack == 8){
					flag = true;
				}
					
			}		
		}
		
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}                                                 
}
void TIM2_IRQHandler(void) {//
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {	
		if(open == true){//0.4ms interval for each one pid
			//	timeperiod++;
			//if(timeperiod == 1){
			//if(flag ==true)
			Direction_Modify();		
			//timeperiod = 0;
			//SPI_Detect2 = SPI_Detect;
		//}	
		}
		if(countblack%2 ==1)
			GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_RESET);
		else if (countblack%2 ==0)
			GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
		/*else if (open ==false){
			if (countblack==5 ){
				delayperiod++;
				if(delayperiod==100){
					open=true;
					delayperiod=0;
				}
			}
			else if (countblack==6 ){
				delayperiod++;
				if(delayperiod==300){
					open=true;
					delayperiod=0;
				}
			}
		}*/
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}                                                 
}


void EXTI9_5_IRQHandler(void) {	
	if (EXTI_GetITStatus(EXTI_Line8) != RESET) {							
		state =true;
		open = true;
      	EXTI_ClearITPendingBit(EXTI_Line8);
    }
}


