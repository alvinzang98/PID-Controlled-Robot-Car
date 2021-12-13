#include "USART.h"

int USART2_RxFlag = 0;
char USART2_RxBuf[50]={'\0'};
char NullStr[52]={'\0'};
char ch;
bool USART2_State=0;
bool PrintfState = 1;
bool ProcessAllow = false;
static unsigned short USART_RxCnt=0;
static char Rx_err = 0;//如果接收出错或者缓冲区溢出 就在此等待接收到0x0D 0x0A
static bool RecState = 0;
extern bool RecAllow;
USART_TypeDef* printf_dest = USART2;
//extern int TimerCount;
#pragma import(__use_no_semihosting) //avoid using microlib
struct __FILE 
{ 
	int handle; 
};FILE __stdout;

void _sys_exit(int x)//define _sys_exit(), in order to avoid semi-hosting mode
{
	x = x; 
}

int fputc(int ch, FILE *f)//redefine printf
{ 	
	while((printf_dest->SR & 0x40)==0);  
	printf_dest->DR = (unsigned char)ch;
	 // while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	//USART_SendData(USART2, (unsigned char)ch);
	return ch;
}

void USART2_WIFI_Init(){
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	
	//USART2 ST-LINK USB
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	
	USART_InitTypeDef USART_InitStructure;
	//USART_ClockInitTypeDef USART_ClockInitStructure; 
	
	USART_InitStructure.USART_BaudRate = 115200;
  	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
 	USART_InitStructure.USART_StopBits = USART_StopBits_1;
  	USART_InitStructure.USART_Parity = USART_Parity_No;
  	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	
	USART_Init(USART2, &USART_InitStructure);
	USART_Cmd(USART2, ENABLE);
	
	
	NVIC_InitTypeDef NVIC_InitStructure;
	// Enable the USART2 RX Interrupt
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE );
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//GPIO_InitTypeDef GPIO_InitStructure;//PB10 for SCL
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	
	//USART_InitTypeDef USART_InitStructure;
	//USART_ClockInitTypeDef USART_ClockInitStructure; 
	
	USART_InitStructure.USART_BaudRate = 115200;
  	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
 	USART_InitStructure.USART_StopBits = USART_StopBits_1;
  	USART_InitStructure.USART_Parity = USART_Parity_No;
  	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  	USART_InitStructure.USART_Mode =  USART_Mode_Tx; //only transmit
	
	USART_Init(USART3, &USART_InitStructure);
	USART_Cmd(USART3, ENABLE);

}

//RECV: +IPD,49:BBExxxxxx \r\n BRDxxxxxx \r\n BOExxxxxx \r\n CMDxxxxxx xxx \r\r\n
void USART2_IRQHandler() {
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		if(!USART2_State){
			if(USART2_RxFlag){//if data haven't been received last time
				USART2_RxFlag = 0;//reset bit to receive
				USART_RxCnt = 0;
			}
			
			USART2_RxBuf[USART_RxCnt] = USART2->DR;
			
			if(Rx_err){
				if(USART2_RxBuf[USART_RxCnt] == 0x0A)
					Rx_err = 0;
			}
			else{
				if(USART2_RxBuf[USART_RxCnt] == 0x0A && USART_RxCnt >= 1){//got \n
					if(USART2_RxBuf[USART_RxCnt - 1] == 0x0D){
						USART2_RxBuf[USART_RxCnt - 1] = 0;//mark the location of \r
						USART2_RxBuf[USART_RxCnt] = 0;//clear \n 
						USART2_RxFlag = 1;
						USART_RxCnt = 0;
						//Wifi_CallBack();
						//printf3("\r\n");
						//printf3((const char*)USART2_RxBuf);
						if(!strcmp((const char*)USART2_RxBuf,"WIFI CONNECTED")){//connection	
							GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_SET);	
						}
						else if(!strcmp((const char*)USART2_RxBuf,"WIFI GOT IP")){//connection	
							
							//USART2_RxBuf[0]=0;// how to reset a character string ???
						}
						else if(!strcmp((const char*)USART2_RxBuf,"OK")){//connection stable
							USART2_State=1;
							USART_RxCnt=0;						
						}
						else if (!strcmp((const char*)USART2_RxBuf,"WIFI DISCONNECT"))
							GPIO_WriteBit(GPIOB,GPIO_Pin_7,Bit_RESET);
					}
					else{
						USART2_RxFlag = 0;
						USART_RxCnt = 0;
						Rx_err = 1;
					}
				}
				else if(USART_RxCnt >= 50-1){//exceed the buffer size
					USART2_RxBuf[USART_RxCnt] = 0;//标记字符串结尾
					//USART2_RxFlag = 1;
					USART_RxCnt = 0;
					Rx_err = 1;
				}
				else{//well-received			
					USART_RxCnt ++;
				}
			}
		}
		else{
			if(RecAllow){
				ch = USART2->DR;
				if(RecState){
					if(ch != '\r' && ch != '\n' && ch != '+' )
						USART2_RxBuf[USART_RxCnt++]=ch;
					if(USART2_RxBuf[USART_RxCnt-1]=='M'){
						//printf3("\r\n");
						USART2_RxBuf[USART_RxCnt-1]='\0';
						//USART2_RxBuf[USART_RxCnt-2]='\0';
						//strcpy(USART2_RxBuf, NullStr);
						//printf3((const char*)USART2_RxBuf);
						USART_RxCnt=0;
						RecState=0;
						RecAllow = false;
						ProcessAllow = true;
					}
					/*else if(ch =='+'){
						RecState = 0;
						USART2_RxBuf[USART_RxCnt++]='\t';
					}*/
				}
				if(ch==':')
					RecState=1;
			}
		}
		
	}	
}

