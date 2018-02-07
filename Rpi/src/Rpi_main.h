/*
 * Rpi_main.h
 *
 *  Created on: 13-Nov-2013
 *      Author: TLarrey
 */

#ifndef RPI_MAIN_H_
#define RPI_MAIN_H_



#include "Sensor.h"
#include "servo.h"
#include "Motor.h"
#include "debug.h"
#include "generic.h"
#include <semaphore.h>


typedef struct {
	t_dualmotors_position RobotPosition;
	int angle;
	int distance;
}t_RPI_Robot_measure;

enum e_RPI_service{
	RPI_SERV_ROBOT=0,
	RPI_SERV_DUAL_MOTOR,
	RPI_SERV_ULTRASOUND,
	RPI_SERV_SERVO,
	RPI_SERV_QUIT,
	RPI_SERV_CHECK,
	RPI_SERV_NB_SERVICES,
	RPI_SERV_UNKNOWN
};

enum e_RPI_robot_command{
	CMD_ROBOT_INIT,
	CMD_ROBOT_RESET,
	CMD_ROBOT_AUTO_START,
	CMD_ROBOT_AUTO_SET_GOAL,
	CMD_ROBOT_AUTO_STOP,
	CMD_ROBOT_GET_STATUS,
	CMD_ROBOT_NB_CMD,
	CMD_ROBOT_UNKNOWN
};

enum e_RPI_robot_state{
	AUTO_MODE_DISABLED=0,
	AUTO_MODE_ENABLED=1,
	AUTO_MODE_ENABLED_RUNNING=2,
	AUTO_MODE_NB_STATE,
	UNKNOWN_STATE
};

const char AutoMode2Str[AUTO_MODE_NB_STATE][10]={
		"disabled",
		"enabled",
		"running"
};

enum e_RPI_robot_algostate {
	AM_ALGO_COMPUTE_PATH = 0,
	AM_ALGO_RUNNING,
	AM_ALGO_BLOCKED,
	AM_ALGO_TARGET_REACH,
	AM_ALGO_WAIT_AFTER_TARGET,
	AM_ALGO_WAIT_AFTER_BLOCKED,
	AM_NB_ALGO_STATE
};

const char AutoModeAlgo2Str[AM_NB_ALGO_STATE][20]={
		"Computing path",
		"Running",
		"Blocked",
		"Target reached",
		"Target reached",
		"Blocked"
};

char ConvertCmd2String[CMD_ROBOT_NB_CMD][20]={
	"init","reset","start_auto","set_goal","stop_auto","get_status"
};
char ConvertServ2String[RPI_SERV_NB_SERVICES][20]={
	"robot","dualmotors","ultrasound","servo","quit","check"
};

int ServoUpdated_CB(int,void*);

class RPI_Robot {
public:
	RPI_Robot();
	virtual ~RPI_Robot() {RPI_TRACE_INFO(SERVICE_MAIN,"Bye Bye...");}
	int switchOffRequired();
	void UnlockOnError();
	int checkCommand(char*,t_RPI_result*);
	int AutoMove(t_RPI_Robot_measure * measures);
	void wakeUpAutoMoveThread();
	void waitForNextEvent();
	void post_TreatmentDone();
	void wait_TreatmentDone();
	bool getEndOfPathStatus();
	void setEndOfPathStatus(bool status);
	bool getAutoMoveStopRequested();
	void setAutoMoveStopRequested(bool status);
	e_RPI_robot_state getAutoMoveStatus();
	void setAutoMoveStatus(e_RPI_robot_state status);
	void getTargetPosition(int *x, int *y);
	void setTargetPosition(int x, int y);
	Sensor * getSensor() { return &mySensor; }
	virtualMap * getMap() { return myMap; }
	DualMotors * getMotor() { return &myMotors; }
private :
	int treatCommand(t_RPI_command* command,t_RPI_result * result);
	int CheckCloseEnvironment(int degreeStep,int * angle);
	int CheckCloseEnvironment();
	int updateAutoMoveGoal(int x, int y);
	int stopAutoMove();
	int startAutoMove();
	int trySetStartPosition();
	sem_t quitInfo;
	int switchOffRequested;
	Servo myServo;
	DualMotors myMotors;
#if defined EMULATE_SENSOR
	FakeSensor mySensor;
#else
	UltraSound mySensor;
#endif
	virtualMap * myMap;
	sem_t AutoMoveInfoProtect;

	typedef struct s_autoMoveInfo {
		bool EndOfPathReached;
		bool stopAutoModeEvent;
		bool isLocked;
		t_map_position goal;
		t_map_position * path;
		pthread_t AutoMoveThId;
		sem_t AutoMoveSync;
		e_RPI_robot_state state;
		e_RPI_robot_algostate stateAlgo;
	}t_autoMoveInfo;

	t_autoMoveInfo autoMoveInfo;
};


#endif /* RPI_MAIN_H_ */
