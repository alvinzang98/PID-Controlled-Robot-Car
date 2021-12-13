//function prototype of init.c


void RCC_ENABLE_ALL(void);              //
void TIM3_PWM_CH12_Init(void);          // Output PWM to two wheels
void TIM2_COUNT_Init(void);             // Count the time, and interrupt every 10ms
void LEFT_PWM_Update(float );			// Update the left wheel pwm
void RIGHT_PWM_Update(float );			// Update the right wheel pwm 
//void EXTI1_CAPTURE_COUNT_Init(void);
//void EXTI6_CAPTURE_COUNT_Init(void);
void EXTI_SWITCH_Init(void);            // Switch is PB8
void LED(void);                         // Flashing LED

