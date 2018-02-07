/*
 * generic.h
 *
 *  Created on: 05-Dec-2013
 *      Author: TLarrey
 */

#ifndef GENERIC_H_
#define GENERIC_H_

#include <math.h>

#define PI 3.1415926535
#define MAX_SIZE_COMMAND_PARAM 20
#define MAX_NB_PARAM 4
#define MAX_RESULT_STRING_LENGTH	1024
#define MAX_COMMAND_STRING_LENGTH	1024
#define WALL 1
#define NOT_ALLOWED 2
#define FREE 0
#define NB_ULTRASOUND_SAMPLES_PER_MEASURE 10

typedef struct s_coord{
	unsigned int x;
	unsigned int y;
}t_coord;

typedef struct s_RPI_command {
	char service[MAX_SIZE_COMMAND_PARAM];
	char param[MAX_NB_PARAM][MAX_SIZE_COMMAND_PARAM];
}t_RPI_command;

typedef struct s_RPI_result {
	int status; // 0 : OK ; else error
	int info[5]; // to be use according to service
	char msg[MAX_RESULT_STRING_LENGTH]; // final string to forward to client : XML format
}t_RPI_result;


typedef struct s_map_position{
	t_coord coord;
	struct s_map_position * p_next;
	int angleToNextPoint;
}t_map_position;



int split_string(char * str,t_RPI_command *);
int formatResultInXML(t_RPI_result * result);
float computeAngleForNextStep(int Ax, int Ay, int Bx, int By);
float distance(int Ax, int Ay, int Bx, int By);

typedef int (*t_callback_servo_position_fct)(int angle,void*) ;



#endif /* GENERIC_H_ */
