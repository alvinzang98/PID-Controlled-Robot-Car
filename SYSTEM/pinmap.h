//pinmap

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

#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //Êä³ö
