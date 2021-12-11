#include <stdlib.h>
//            L  ,R  , 
float KP =11.615;//11.615
float KI =0;//0.0051
float KD =0;//0.0013
//float LastError=0;
float Error = 0;
//float ErrorSum = 0;
//float ErrorDiffern = 0;


float PID_Update(float Error){//1st left, 2nd right
	//Error=CurrentSpeed1 - SetSpeed;
	//ErrorSum += Error ;
	//ErrorDiffern = Error - LastError;
	//LastError = Error;
	return (KP*Error +KD*Error);
}



