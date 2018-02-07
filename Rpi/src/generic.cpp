/*
 * generic.cpp
 *
 *  Created on: 05-Dec-2013
 *      Author: TLarrey
 */
#include "generic.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

float computeAngleForNextStep(int Ax, int Ay, int Bx, int By){
		// Move A to (0,0)
		// Bx becomes B'x [ Bx-Ax ] and By becomes B'y[ By-Ay ]
		float B_x= (Bx-Ax);
		float B_y= (By-Ay);
		return (atan2(B_y,B_x)*180/PI);
	return 0;
}

float distance(int Ax, int Ay, int Bx, int By)
{
    int dx = Ax - Bx;
    int dy = Ay - By;
    return sqrt(dx*dx + dy*dy);
}


int split_string(char * str,t_RPI_command * command){
  char * pch=NULL;
  int i=0;
  pch = strtok (str," ");
  if(pch==NULL) return 0;
  strcpy(command->service,pch);
  while (pch != NULL && i<MAX_NB_PARAM)
  {
    pch = strtok (NULL, " ");
    if(pch)
      strncpy(command->param[i],pch,MAX_SIZE_COMMAND_PARAM);
    else
      strncpy(command->param[i],"",MAX_SIZE_COMMAND_PARAM);
    i++;
  }
  return 1;
}



int formatResultInXML(t_RPI_result * result) {
	char oriResult[MAX_RESULT_STRING_LENGTH];
	strncpy(oriResult,result->msg,MAX_RESULT_STRING_LENGTH);
	result->msg[0]='\0';
	sprintf(result->msg,"<result><status>%d</status><msg>%s</msg><info1>%d</info1><info2>%d</info2><info3>%d</info3><info4>%d</info4><info5>%d</info5></result>",
					result->status,
					oriResult,
					result->info[0],result->info[1],result->info[2],result->info[3],result->info[4]);
	return 0;
}




