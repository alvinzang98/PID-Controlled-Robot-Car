#include "init.h"
#include "WIFI.h"
#include "USART.h"
#include "math.h"
#include "PID.h"

static __IO uint32_t msTicks;   
char LabelBuf[4] = "\0";      					//Balls: BBE(Blue), BRD(ORANGE), BYW(Yellow)
//char LocationBufX[4] = "\0";   					//Label: COG(RED),CVT(Violet)
//char LocationBufY[4] = "\0";
char Buffer[10] = "\0";
int Ball_Order =1;
int LastOrder =0;
float StartSpeed = 90;
float StartSpeed1 = 140;
float pi = 3.1415;
/*
int x_BBE=0, x_BRD=0, x_BOE=0, x_BPK=0, x_BYW=0;
int y_BBE=0, y_BRD=0, y_BOE=0, y_BPK=0, y_BYW=0;*/

void DelayMs(uint32_t ms);
void ProcessData(char*);
void Update_PWM(void);
int ConvertData(char);

extern int TimerCount;
extern bool open;
extern bool USART2_State;
extern bool PrintfState;
extern bool ProcessAllow;
extern char USART2_RxBuf[50];

struct{
	int x;
	int y;
}BBE,CVT,COG,BRD,BYW;

struct{
	float theta_B_LO;
	float theta_B_LV;
	float error1;
}order1, order2, order3;

int main(){
	// Update SystemCoreClock value
	SystemCoreClockUpdate();
	// Configure the SysTick timer to overflow every 1 ms
	SysTick_Config(SystemCoreClock / 1000);
	RCC_ENABLE_ALL();
	TIM3_PWM_CH12_Init();
	TIM2_COUNT_Init();
	//EXTI1_CAPTURE_COUNT_Init();
	//EXTI6_CAPTURE_COUNT_Init();
	EXTI_SWITCH_Init() ;
	USART2_WIFI_Init();
	LED();
	
	//DelayMs(500);
	GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
	while(!open){
		;
	}
	
	WIFI_Connect("IntegratedProject","31053106");
	DelayMs(500);
	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //Allow updates to interrupt, allows the CC1IE to capture interrupt*/
	while(1){
		
		if(USART2_State && PrintfState){
			WIFI_ConnectServer("0",2);
			PrintfState = 0;
		}
		if(ProcessAllow){
			int i =0;int j =0;
			//printf3((const char*)USART2_RxBuf);
			for(i =0;i<strlen(USART2_RxBuf);i++){
				if(i%9 == 0 && i!=0){
					ProcessData(Buffer);
					j=0;
					//printf3((const char*)Buffer);
					//printf3("\r\n");
				}
				Buffer[j++] = USART2_RxBuf[i];
			}
			/*plan your trace here*/
			Update_PWM();
			TimerCount = 0;
			ProcessAllow =false;
		}
	}
}

void ProcessData(char data[10]){
	int i =0;int xdata=0; int ydata =0;
	for(i =0;i<strlen(data);i++){
		if(i<3)
			LabelBuf[i] = data[i];
		else if(i>=3 && i<6)
			xdata += pow(16,5-i)*ConvertData(data[i]);
			//LocationBufX[i-3] = data[i];
		else if(i>=6)
			ydata += pow(16,8-i)*ConvertData(data[i]);
			//LocationBufY[i-6] = data[i];
		//printf3((const char*)LabelBuf); 
	}
	if(!strcmp(LabelBuf,"BBE")){
		BBE.x = xdata;
		BBE.y = ydata+40;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}
	else if(!strcmp(LabelBuf,"CVT")){
		CVT.x = xdata;
		CVT.y = ydata;
	}
	else if(!strcmp(LabelBuf,"COG")){
		COG.x = xdata;
		COG.y = ydata;
	}
	else if(!strcmp(LabelBuf,"BRD")){
		BRD.x = xdata;
		BRD.y = ydata-10;
	}
	else if(!strcmp(LabelBuf,"BYW")){
		BYW.x = xdata;
		BYW.y = ydata-20;
	}
	else{
		;
	}

}
bool interchange =false;
char chc[10]="\0";
void Update_PWM(){
	if(Ball_Order ==1 && BBE.x>0 && BBE.y >0){//BBE
		order1.theta_B_LO = atan2((COG.y-CVT.y),(COG.x - CVT.x)); 
		order1.theta_B_LV = atan2((COG.y-BBE.y),(COG.x - BBE.x)); 
		order1.error1 = (order1.theta_B_LO - order1.theta_B_LV )*180/pi;
		//sprintf(chc,"%f", error1);
		//printf3((const char*)chc);
		//printf3("\r\n");
		LEFT_PWM_Update(StartSpeed -PID_Update(order1.error1));
		RIGHT_PWM_Update(StartSpeed + PID_Update(order1.error1));//盖和球最长距离是30
		if( (CVT.x-BBE.x)<20 ){//compensator ?? unknown
			LastOrder = Ball_Order;
			Ball_Order =0;
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_RESET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);
			LEFT_PWM_Update(StartSpeed1);
			RIGHT_PWM_Update(StartSpeed1);
		}
	}
	else if(Ball_Order ==2 && BRD.x>0 && BRD.y >0){//BRD
		order2.theta_B_LO = atan2((COG.y-CVT.y),(COG.x - CVT.x)); 
		order2.theta_B_LV = atan2((COG.y-BRD.y),(COG.x - BRD.x)); 
		order2.error1 = (order2.theta_B_LO - order2.theta_B_LV )*180/pi;
		LEFT_PWM_Update(StartSpeed -PID_Update(order2.error1));
		RIGHT_PWM_Update(StartSpeed + PID_Update(order2.error1));//盖和球最长距离是30
		if( (CVT.x-BRD.x)<20 ){//compensator ?? unknown
			LastOrder = Ball_Order;
			Ball_Order =0;
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_RESET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);
			LEFT_PWM_Update(StartSpeed1);
			RIGHT_PWM_Update(StartSpeed1);
		}
	}
	else if(Ball_Order ==3 && BYW.x>0 && BYW.y>0){//BYW
		
		order3.theta_B_LO = atan2((COG.y-CVT.y),(COG.x - CVT.x)); 
		order3.theta_B_LV = atan2((COG.y-BYW.y),(COG.x - BYW.x)); 
		order3.error1 = (order3.theta_B_LO - order3.theta_B_LV )*180/pi;
		LEFT_PWM_Update(StartSpeed - PID_Update(order3.error1));
		RIGHT_PWM_Update(StartSpeed + PID_Update(order3.error1));//盖和球最长距离是30
		if( (CVT.x-BYW.x)<20){//compensator ?? unknown
			LastOrder = Ball_Order;
			Ball_Order =0;
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_RESET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);
			LEFT_PWM_Update(StartSpeed1);
			RIGHT_PWM_Update(StartSpeed1);
		}
	}
	else{
		if(COG.x>800){
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_SET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);
			LEFT_PWM_Update(0);
			RIGHT_PWM_Update(0);
			PID_clear();
			Ball_Order =LastOrder+1;
			DelayMs(200);
		}
		//TIM3->CCR1 =0;
		//TIM2->CCR2 =0;
	}
	


}
int ConvertData(char a){
	if (a<58)
		return a-48;
	else {
		switch(a){
			case 'a': return 10;
			case 'b': return 11;
			case 'c': return 12;
			case 'd': return 13;
			case 'e': return 14;
			case 'f': return 15;		
			default:  return 0;
		}
	}

}
void DelayMs(uint32_t ms)
{
	// Reload us value
	msTicks = ms;
	// Wait until usTick reach zero
	while (msTicks);
}
void SysTick_Handler()
{
	if (msTicks != 0)
	{
		msTicks--;
	}
}

