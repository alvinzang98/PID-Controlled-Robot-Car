#include "init.h"
#include "WIFI.h"
#include "USART.h"
#include "math.h"
#include "PID.h"

static __IO uint32_t msTicks;   //Balls: BBE(Blue), BOG(ORANGE), BBE(Yellow)
char LabelBuf[4] = "\0";      	//Label: CRD(RED),CVT(Violet)				  					

bool interchange =false;
char chc[10]="\0";
char Buffer[10] = "\0";
int Ball_Order =1;
int LastOrder =0;
float StartSpeed = 95;
float StartSpeed1 = 90;
float pi = 3.1415;
static int StartxPosition = 880;
static int StartyPosition = 290;

int ConvertData(char);	 	//Convert hexidecimal to decimal
int LastxValue = 0;
int LastyValue = 0;
void DelayMs(uint32_t ms);
void ProcessData(char*);	// data fragments
void Update_PWM(void);

extern int TimerCount;
extern int CountLeft;
extern int CountRight;
extern bool open;
extern bool USART2_State;	//WIFI Connection
extern bool PrintfState;	//WIFI Connection only excuted once
extern bool ProcessAllow;   //10ms for one process
extern char USART2_RxBuf[50];

struct{
	int x;
	int y;
}COG,CVT,BBE;  //Blue Front Cap, Orange Back Cap, Yellow Ball

struct{
	float Theta_CC;
	float Theta_BC;
	float Error;
}Forward, Backward;

int main(){
	
	SystemCoreClockUpdate();  // Update SystemCoreClock value
	SysTick_Config(SystemCoreClock / 1000); // Configure the SysTick timer to overflow every 1 ms
	RCC_ENABLE_ALL();
	TIM3_PWM_CH12_Init();
	TIM2_COUNT_Init();
	EXTI_SWITCH_Init() ;
	//EXTI1_CAPTURE_COUNT_Init();
	//EXTI6_CAPTURE_COUNT_Init();
	USART2_WIFI_Init();
	LED();
	
	GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);
	
	while(!open){    //Wait for user pressing the start button
		;	
	}
	
	WIFI_Connect("IntegratedProject","31053106");
	DelayMs(500); 	//Wait for connection
	TIM_Cmd(TIM3, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //Allow updates to interrupt, allows the CC1IE to capture interrupt*/
	int loop =0;
	while(1){
		
		if(USART2_State && PrintfState){
			WIFI_ConnectServer("0",2);
			PrintfState = 0;
		}
		if(ProcessAllow){
			int i =0;int j =0;
			for(i =0;i<strlen(USART2_RxBuf);i++){
				if(i%9 == 0 && i!=0){
					ProcessData(Buffer);
					j=0;
				}
				Buffer[j++] = USART2_RxBuf[i];
			}
			 
			Update_PWM();
			/*char x[20]="\0";
			sprintf(x,"abs is %d\r\n",(char)abs(LastxValue -BBE.x));
			printf3((const char*)x);*/
			loop++;	
			if(loop ==10){
				LastxValue = BBE.x;
				LastyValue = BBE.y;
				loop =0;
			}
			
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
		else if(i>=6)
			ydata += pow(16,8-i)*ConvertData(data[i]);
	}
	if(!strcmp(LabelBuf,"CVT")){
		CVT.x = xdata;
		CVT.y = ydata;			//!! bias to correct the location-detection errors
	}
	/*else if(!strcmp(LabelBuf,"CVT")){
		CVT.x = xdata;
		CVT.y = ydata;
	}
	else if(!strcmp(LabelBuf,"CRD")){
		CRD.x = xdata;
		CRD.y = ydata;
	}*/
	else if(!strcmp(LabelBuf,"COG")){
		COG.x = xdata;
		COG.y = ydata;
	}
	else if(!strcmp(LabelBuf,"BBE")){
		BBE.x = xdata;
		BBE.y = ydata;
	}
	else{
		;
	}

}


void Update_PWM(){
	
	if(Ball_Order ==0 && BBE.x>500 && BBE.y >0){//BBE
		
		Forward.Theta_CC = atan2((COG.y-CVT.y),(COG.x - CVT.x)); 
		Forward.Theta_BC = atan2((COG.y-BBE.y),(COG.x - BBE.x)); 
		Forward.Error = (Forward.Theta_CC - Forward.Theta_BC )*180/pi;
		LEFT_PWM_Update(StartSpeed -PID_Update(Forward.Error));
		RIGHT_PWM_Update(StartSpeed + PID_Update(Forward.Error));//盖和球最长距离是30
		
		if( (CVT.x-BBE.x)<20 || CVT.x<500 ){		//compensator ?? unknown
			LastOrder = Ball_Order;
			Ball_Order =1;
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_RESET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_RESET);
			LEFT_PWM_Update(0);
			RIGHT_PWM_Update(0);
			DelayMs(100);
			PID_clear();
			//while(){}
			//LEFT_PWM_Update(StartSpeed1);
			//RIGHT_PWM_Update(StartSpeed1);
		}
		
	}
	else{		//Blue Front Cap, Orange Back Cap, Yellow Ball
		
		if(COG.x>880){
			GPIO_WriteBit(GPIOC,GPIO_Pin_15,Bit_SET);
			GPIO_WriteBit(GPIOA,GPIO_Pin_0,Bit_SET);
			LEFT_PWM_Update(0);
			RIGHT_PWM_Update(0);
			
			if((BBE.x>500) && abs(LastxValue -BBE.x)<1 && abs(LastyValue - BBE.y)<1){
				Ball_Order =LastOrder;
				PID_clear();
				DelayMs(100);
			}
			
		}
		/*code here*/
		//880,290
		else if (COG.x>500&&COG.x<880){
			Backward.Theta_CC = atan2((COG.y-CVT.y),(COG.x - CVT.x)); //theta两个盖子,theta原点后盖
			Backward.Theta_BC = atan2((StartyPosition-CVT.y),(StartxPosition - CVT.x)); 
			Backward.Error = (Backward.Theta_BC - Backward.Theta_CC )*180/pi;
			LEFT_PWM_Update(StartSpeed1 - PID_Update(Backward.Error));
			RIGHT_PWM_Update(StartSpeed1 + PID_Update(Backward.Error));//盖和球最长距离是30
		}
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
	msTicks = ms;  // Reload us value
	while (msTicks); // Wait until usTick reach zero
}

void SysTick_Handler()
{
	if (msTicks != 0)
	{
		msTicks--;
	}
}

