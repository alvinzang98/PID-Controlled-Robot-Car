#include <stdlib.h>
float KP =0.6;//0.007
float KI =0.02;//0.0051
float KD =0;//0.12
float LastError=0;
float Error = 0;
float ErrorSum = 0;
float ErrorDiffern = 0;


float PID_Update(float CurrentSpeed1){//1st left, 2nd right
	Error=CurrentSpeed1;
	ErrorSum += Error ;
	ErrorDiffern = Error - LastError;
	LastError = Error;
	return (KP*Error +KI*ErrorSum +KD*ErrorDiffern);
}
void PID_clear(void){
	ErrorSum =0;
	ErrorDiffern =0;
	LastError =0;
}



