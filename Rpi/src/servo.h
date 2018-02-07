/*
 * servo.h
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */

#ifndef SERVO_H_
#define SERVO_H_


#include <pthread.h>
#include <semaphore.h>
#include "debug.h"
#include "generic.h"


#define ANGLE_SPEED_DEG_PER_SECOND_ASC  90 // degree per second from left to right
#define ANGLE_SPEED_DEG_PER_SECOND_DESC  90 // degree per second from right to left

typedef struct s_AngleSet{
	int angle;
	struct s_AngleSet * next;
}t_AngleSet;

class Servo {
public:
	Servo();
	~Servo();
	enum {
		Asynchronous,
		Synchronous
	};

	typedef struct {
		int currentAngle;
		int speed;
		int stopRequested;
		int state;
		int infiniteMoveRequested;
	}t_servoInfo;

	enum {
		Idle,
		Moving
	};
	int treatCommand(t_RPI_command* command,t_RPI_result * result);
	int setAngle(int angle,int mode);
	int getAngle();
	void* registerPositionSet(t_callback_servo_position_fct pCB,void * coockies);
	int unregisterPositionSet(void* handle);
	int startInfiniteMove(int speed);
	int stopInfiniteMove();
	int move();
	int internalSetAngle(int angle);
	struct s_AngleSet * removeFromThreadList(t_AngleSet * pElt);
	struct s_AngleSet * getListThread() { return pList; };
	struct s_AngleSet * QueueEndofThreadList(t_AngleSet * pElt);
	void checkAndWaitPendingThread();
	void parseList();
	void get_status(t_servoInfo * pInfo);
private:
	enum {
		CMD_SET_ANGLE,
		CMD_GET_ANGLE,
		CMD_START_MOVE,
		CMD_STOP_MOVE,
		CMD_NB_CMD,
		CMD_UNKNOWN
	}e_servoCommand;


	enum {
		LEFT,
		RIGHT
	};
	typedef struct s_registerCB {
		t_callback_servo_position_fct callback;
		void * coockies;
		struct s_registerCB * nextCB;
	}t_registerCB;

	char ConvertCmd2String[CMD_NB_CMD][20];
	pthread_t thIdInfinite;
	pthread_t thId;
	sem_t dataProtect;
	sem_t api_protect;
	sem_t list_protect;
	sem_t endOfMoving;
	t_registerCB * pListCB;
	t_servoInfo info;
	struct s_AngleSet * pList;
	int InformPositionSet(int angle);
	int drv_servo_prepare();
	int drv_servo_reset();
	int drv_servo_move(int direction,int delay);
};



#endif /* SERVO_H_ */
