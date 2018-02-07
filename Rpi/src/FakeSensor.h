/*
 * FakeSensor.h
 *
 *  Created on: 05-Dec-2013
 *      Author: TLarrey
 */

#ifndef FAKESENSOR_H_
#define FAKESENSOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <math.h>
#include "./bmp/bmptypes.h"
#include "./bmp/endian.h"
#include "./bmp/bmpReader.h"
#include "Sensor.h"
#include "generic.h"

#define MAP_FILE "/home/TLarrey/map.bmp"
namespace std {


class BmpMap {
public:
	BmpMap();
	virtual ~BmpMap();
	int isThereObstacle(int angle,t_map_position * current_pos, int distance,t_map_position * obstacle_pos);
	long width;
	long height;
private:
	endian endianConvert;
	bmpReader bmp;
	void displayMap(void);
	int parseBMPFile(FILE *fp,int ** map);
	int ** myMap;
	int calculatePos(int angle,t_map_position * current_pos, int distance,t_map_position * next_pos);
};

class FakeSensor : public Sensor {
public:
	typedef int (*t_callback_fct)(t_map_position * obstacle_pos,int,void*) ;
	FakeSensor();
	virtual ~FakeSensor();
	void getMapInfo(int * width,int* height) { *width = RefMap.width; *height= RefMap.height; }
	void setServo(Servo * pServo) { myServo = pServo; }
	void setMotors(DualMotors * pMotors) {myMotors = pMotors; }
	int OverwriteWithMapCheck(int *pDist,t_map_position * obstacle_pos);
private:
	int get_results_sensor(t_sensor_result * results);
	int get_sync_results_sensor(t_sensor_result * results,int angleServo);
	int makeUniqueMeasure(int pAngleServo,t_sensor_result * results);
	Servo * myServo;
	DualMotors * myMotors;
	pthread_t USthId;
	BmpMap RefMap;
};

} /* namespace std */
#endif /* FAKESENSOR_H_ */
