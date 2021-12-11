#include "stm32f10x.h"
#include "stdbool.h"





extern void RCC_ENABLE_ALL(void);
extern void TIM3_PWM_CH12_Init(void);
extern void TIM2_COUNT_Init(void);
extern void EXTI1_CAPTURE_COUNT_Init(void );
extern void EXTI6_CAPTURE_COUNT_Init(void);
extern void EXTI_SWITCH_Init(void) ;
extern void LED(void);
extern void SPI2_Init(void);
extern void Read_Data(void);
extern void TIM4_COUNT_Init(void);
extern bool state;
//extern int countleft,countright;


int main(){
	
	RCC_ENABLE_ALL();
	TIM3_PWM_CH12_Init();
	TIM2_COUNT_Init();
	//EXTI1_CAPTURE_COUNT_Init();
	//EXTI6_CAPTURE_COUNT_Init();
	EXTI_SWITCH_Init() ;
	TIM4_COUNT_Init();
	LED();
	SPI2_Init();
	// Update SystemCoreClock value
	//SystemCoreClockUpdate();
	
	// Configure the SysTick timer to overflow every 1 ms
	//SysTick_Config(SystemCoreClock / 1000);
	while(1){
		if(state == true){
		TIM_Cmd(TIM3, ENABLE);
		TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
		TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //Allow updates to interrupt, allows the CC1IE to capture interrupt*/
		}
		Read_Data();
	
	}
}

