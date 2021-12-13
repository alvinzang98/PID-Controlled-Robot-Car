#include "USART.h"


#ifndef __WIFI_H__
#define __WIFI_H__
#define WIFI_Printf printf2

void WIFI_Connect(char* ssid,char* password);
void WIFI_ConnectServer(const char* ip_addr,unsigned short port);

#endif
