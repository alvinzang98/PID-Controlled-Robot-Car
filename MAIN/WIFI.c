#include "WIFI.h"

void WIFI_Connect(char* ssid,char* password){

	WIFI_Printf("AT+CWJAP=\"%s\",\"%s\"\r\n",ssid,password);
	
}

void WIFI_ConnectServer(const char* ip_addr,unsigned short port){ 
  	
	WIFI_Printf("AT+CIPSTART=\"UDP\",\"%s\",0,3105,%d\r\n",ip_addr,port);
}

