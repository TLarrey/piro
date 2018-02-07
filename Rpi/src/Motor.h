/*
 * Motor.h
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */

#ifndef MOTOR_H_
#define MOTOR_H_
#include <semaphore.h>
#include <time.h>
#include <math.h>
#include "debug.h"
#include "generic.h"
#include <pthread.h>
#include "map.h"

#define MOTOR_RPS 4 //revolutions per second
#define MIN_MOTOR_SPEED (-1*MOTOR_RPS)
#define MAX_MOTOR_SPEED (MOTOR_RPS)
#define DEFAULT_MOTOR_SPEED 0
#define DIST_BETWEEN_WHEELS_CM  15
#define WHEELS_DIAMETRE_CM  6
#define WHEELS_CIRC  (WHEELS_DIAMETRE_CM*PI)
#define FULL_ROTATION 360
#define CTURN (DIST_BETWEEN_WHEELS_CM*PI)

typedef int (*t_motor_callback_fct)(void*);


class Motor {
public:
	// ENUM
	typedef enum e_motor_sync_mode {
		IMMEDIATE=0,
		WAIT_FOR_SYNC,
		NB_SYNC_MODE
	}t_motor_sync_mode;

	typedef enum e_motor_state {
		IDLE=0,
		STARTED,
		NB_STATE
	}t_motor_state;

	typedef enum e_motor_action {
			STOP=0,
			START,
			SET_SPEED,
			DELAY,
			ROTATION,
			NB_ACTION
		}t_motor_action;

	typedef struct s_info_action{
		t_motor_action action;
		int param1;
		float param2;
		struct s_info_action * next;
	}t_info_action;

	// SERVICES
	Motor();
	virtual ~Motor();
	// return <0 if error else 0 if ok (han
	int start(t_motor_sync_mode mode);
	// return <0 if error else return handle in case of asynch or 0 if ok
	int stop(t_motor_sync_mode mode);
	// return <0 if error else return handle in case of asynch or 0 if ok
	int setSpeed(float speed,t_motor_sync_mode mode);
	int setDelay(int delayMs,t_motor_sync_mode mode);
	int doRotation(float nbRotation,t_motor_sync_mode mode);
	// return <0 if error or 0 if ok
	int syncSignal(void);
	int getCurrentSpeed() { return current_speed; }
	void getSpeedRange(int *min,int*max) { *min=MIN_MOTOR_SPEED;*max=MAX_MOTOR_SPEED;}
	float getCWheel() { return (WHEELS_DIAMETRE_CM*PI); }
	int reset(void);
	int setName(const char *name);
	const char * getName() { return name;}
	void mainLoop();
private:
	pthread_t thId;
	const char * convertActionToStr(t_motor_action act);
	void * addAction(t_motor_action act,int param1,float param2);
	int ExecuteListedAction(int execute);
	int drv_motor_start(float speed);
	int drv_motor_stop();
	int drv_motor_set_speed(float speed);
	int drv_motor_do_rotation(float nbRotation);
	char name[20];
	sem_t api_protect;
	sem_t list_protect;
	int reverse;
	int state;
	float current_speed;
	t_info_action * listActions;
};


#define MOTORL_CORRECTION (-0.02)
#define MOTORR_CORRECTION (0.01)

typedef struct {
	float x;
	float y;
	int angle;
	struct timespec timing;
	bool stopRequested;
}t_dualmotors_position;

typedef struct {
	float current_speed;
	int state;
	t_dualmotors_position pos;
}t_dualmotors_status;

class DualMotors {
public:
	// Services
	DualMotors(int x=0,int y=0);
	virtual ~DualMotors();
	int treatCommand(t_RPI_command* command,t_RPI_result * result);
	int setSpeed(int speed);
	int turn(int angle);
	void get_position(t_dualmotors_position * pPos) { pPos->x=pos.x;pPos->y=pos.y; pPos->angle=pos.angle; }
	void set_initial_position(int x, int y) { pos.x=x;pos.y=y; }
	int move_forward();
	int move_backward();
	int get_state(void) { return state; }
	int get_status(t_dualmotors_status * pStatus);
	int stop();
	int getSpeed() { return current_speed;}
	int followThisPath(t_map_position * path,t_motor_callback_fct pCB,void* coockies);
	int PositionLoop();
	enum {
		CMD_INIT,
		CMD_RESET,
		CMD_MOVE_FORWARD,
		CMD_MOVE_BACKWARD,
		CMD_STOP,
		CMD_GET_SPEED,
		CMD_SET_SPEED,
		CMD_TURN,
		CMD_GET_STATE,
		CMD_GET_POSITION,
		CMD_NB_CMD,
		CMD_UNKNOWN
	}e_DualMotorscommand;
	typedef enum e_dualmotors_state {
		STOPPED=0,
		STARTED_FORWARD,
		STARTED_BACKWARD,
		ROTATION_ON_GOING,
		NB_DUALMOTOR_STATE
	}t_dualmotors_state;

private:
	char ConvertCmd2String[CMD_NB_CMD][20];
	typedef enum e_dualmotors_pos {
		LEFT=0,
		RIGHT,
		NB_MOTORS
	}t_dualmotors_pos;
	sem_t updatePositionProtect;
	bool PositionUpdateOnGoing;
	t_motor_callback_fct fct_PathCB;
	void * coockiesCB;
	pthread_t thId;
	float current_speed;
	Motor motor[NB_MOTORS];
	float CTurnCWheelRatio[NB_MOTORS];
	Motor * getLeftMotor() { return &motor[LEFT]; }
	Motor * getRightMotor(){ return &motor[RIGHT]; }
	bool RobotHasToChangeDirection(int x,int y,t_map_position * pathToFollow);
	bool updatePosition();
	int StartPositionUpdate(bool updateTiming);
	int StopPositionUpdate();
	double elapsedTime(struct timespec begin);
	void updateCurrentTime(t_dualmotors_position * pos);
	float adaptSpeed(t_dualmotors_pos, float);
//	int computeAngleForNextStep();
	t_dualmotors_state state;
	t_dualmotors_position pos;
	t_map_position * pathToFollow;
};


#endif /* MOTOR_H_ */
