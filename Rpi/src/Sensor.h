/*
 * Sensor.h
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */

#ifndef SENSOR_H_
#define SENSOR_H_

#include <pthread.h>
#include <semaphore.h>
#include "debug.h"
#include "map.h"
#include "servo.h"

#include "Motor.h"

#define MAX_NB_RESULT_STORED 30

typedef struct s_sensor_result{
		int tabDistance[NB_ULTRASOUND_SAMPLES_PER_MEASURE];
		int AvgDistance;
		t_coord coord;
		struct s_sensor_result * nextRes;
	}t_sensor_result;

typedef int (*t_callback_fct)(t_sensor_result*,void*) ;

class Sensor {
public:
	Sensor();
	virtual ~Sensor();
	// ENUM
	enum {
		enabled,
		disabled
	};
	typedef enum e_sensor_cmd {
		EXIT=0,
		START_MEASURES,
		STOP_MEASURES,
		GET_RESULTS,
		NB_CMD
	}t_sensor_cmd;
	typedef enum e_sensor_state {
		IDLE=0,
		STARTED,
		NB_STATE
	}t_sensor_state;

	//API
	void start(void);
	void start(int period,t_callback_fct cb,void *);
	void stop(void);
	void get_results(void);
	int get_results(t_sensor_result * results,int angle=-1);
	void get_status(int * pStatus) {
		SEM_WAIT(SERVICE_SENSOR,&infoProtect);
		*pStatus=info.state;
		SEM_POST(SERVICE_SENSOR,&infoProtect);
	}
	int treatCommand(t_RPI_command* command,t_RPI_result * result);
	// DO NOT USE THIS API : just here to be able to use pthread_create
	virtual void mainLoop();
	void makeMeasures();
private:
	pthread_t thId;
	pthread_t USthId;
	t_sensor_cmd event;
	int activate_sensor();
	int deactivate_sensor();
	int set_event(t_sensor_cmd event);
	int wait_event();
	void flushResults(t_callback_fct pCB,void * coockies);
	int FormatResultsXML(FILE * fo,unsigned int * bytes);
	t_sensor_result * listLastMeasures;
	sem_t sem_thread_start;
	sem_t sem_event_sent;
	sem_t sem_event_ack;
	sem_t api_protect;
	sem_t syncStop;
	sem_t listMeasuresProtect;
	char ConvertCmd2String[NB_CMD][20];
	t_sensor_result * getAllResults();
	t_sensor_result * getLastResult();
	virtual int get_results_sensor(t_sensor_result * results)=0;
	virtual int get_sync_results_sensor(t_sensor_result * results,int angleServo) { return get_results_sensor(results);}
	virtual void getMapInfo(int * width,int* height) { *width = -1; *height= -1; }
	virtual void setServo(Servo * pServo) { return; }
	virtual void setMotors(DualMotors * pMotors) { return; }
	virtual int makeUniqueMeasure(int pAngleServo,t_sensor_result * results)=0;
protected:
	typedef struct {
		int period; // milliseconds
		int cmdStop;
		t_sensor_state state;
	}t_SensorInfo;
	t_callback_fct callbackMeasure;
	void * coockies;
	t_SensorInfo info;
	sem_t infoProtect;
	void pushResult(t_sensor_result * results);
};

class UltraSound : public Sensor {
public:
	UltraSound();
	virtual ~UltraSound();
private:
	int drv_state;
	int get_results_sensor(t_sensor_result * results);
	int get_sync_results_sensor(t_sensor_result * results,int angleServo);
	int makeUniqueMeasure(int pAngleServo,t_sensor_result * results);
	int drv_ultrasound_measure(int * value);
	int drv_ultrasound_prepare();
	int drv_ultrasound_reset();
};

// After SENSOR Class declaration
#if defined EMULATE_SENSOR
#include "FakeSensor.h"
#endif
#endif /* SENSOR_H_ */
