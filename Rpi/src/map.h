/*
 * map.h
 *
 *  Created on: 25-Nov-2013
 *      Author: TLarrey
 */

#ifndef MAP_H_
#define MAP_H_

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <iostream>
#include <math.h>
#include "generic.h"
#include "dstar.h"

int ** allocateMap(int row, int col);
void freeMap(int ** tab,int row, int col);

static const float tab_cos_sin[36][2] = {
		{ 1 , 0 } // 0
		,{ 0.984807753 , 0.173648178 }  // 10
		,{ 0.939692621 , 0.342020143 }  // 20
		,{ 0.866025404 , 0.5 }   // 30 ...
		,{ 0.766044443 , 0.64278761 }
		,{ 0.64278761 , 0.766044443 }
		,{ 0.5 , 0.866025404 }
		,{ 0.342020143 , 0.939692621 }
		,{ 0.173648178 , 0.984807753 }
		,{ 0 , 1 }
		,{ -0.173648178 , 0.984807753 }
		,{ -0.342020143 , 0.939692621 }
		,{ -0.5 , 0.866025404 }
		,{ -0.64278761 , 0.766044443 }
		,{ -0.766044443 , 0.64278761 }
		,{ -0.866025404 , 0.5 }
		,{ -0.939692621 , 0.342020143 }
		,{ -0.984807753 , 0.173648178 }
		,{ -1 , 0 }
		,{ -0.984807753 , -0.173648178 }
		,{ -0.939692621 , -0.342020143 }
		,{ -0.866025404 , -0.5 }
		,{ -0.766044443 , -0.64278761 }
		,{ -0.64278761 , -0.766044443 }
		,{ -0.5 , -0.866025404 }
		,{ -0.342020143 , -0.939692621 }
		,{ -0.173648178 , -0.984807753 }
		,{ 0 , -1 }
		,{ 0.173648178 , -0.984807753 }
		,{ 0.342020143 , -0.939692621 }
		,{ 0.5 , -0.866025404 }
		,{ 0.64278761 , -0.766044443 }
		,{ 0.766044443 , -0.64278761 }
		,{ 0.866025404 , -0.5 }
		,{ 0.939692621 , -0.342020143 }
		,{ 0.984807753 , -0.173648178 }
};

class infoUpdateMap {
public :
	typedef enum e_UpdateMapAction {
		MAP_UPDATE_NO_ACTION,
		MAP_UPDATE_START,
		MAP_UPDATE_GOAL,
		MAP_UPDATE_CELL,
		MAP_UPDATE_AREA,
		MAP_UPDATE_NB
	}t_UpdateMapAction;

	infoUpdateMap();
	~infoUpdateMap();
	void StopThread();
	bool getStopState() { return stopRequested; }
	void setNextAction(t_UpdateMapAction action,unsigned int valx, unsigned int valy, int val);
	t_UpdateMapAction getNextAction(void) { return UpdatedAction;}
	unsigned int getX() { return x; }
	unsigned int getY() { return y; }
	int getCellVal() { return CellVal; }
	void setEvent();
	void waitEvent();
	void waitAnswer();
	void setAnswer();
	void updateArea(Dstar * pDS,unsigned int xTile,unsigned int yTile,int val);
	void reset();
private :
	bool stopRequested;
	t_UpdateMapAction UpdatedAction;
	unsigned int x;
	unsigned int y;
	int CellVal;
	int SecuritySize;
	sem_t Sync;
	sem_t Done;
	sem_t ApiProtect;
};

class virtualMap {
public:
	virtualMap(unsigned long pWidth,unsigned long pHeight);
	virtual ~virtualMap();
	void setTileSize(int sizeInPixel) { tileSizeInPixel=sizeInPixel;}
	bool setStartPosition(t_map_position * objectPosition );
	bool setGoalPosition( t_map_position * objectPosition );
	int SetMapObstacle( t_map_position * objectPosition,int radian);
	t_map_position * computePath(void);
	void deletePath(t_map_position *path);
	void displayPath(t_map_position * list);
	infoUpdateMap * getSharedInfoThread() { return &SharedInfoThread; }
	int storeVirtualMap(char * filename_wo_ext);
	Dstar mypath;
private:
	void convertInPixel(t_map_position * list);
	unsigned int convertMapIndexToPixel(unsigned int index);
	unsigned int convertMapPixelToIndex(unsigned int pixel);
	unsigned long width; //in pixels
	unsigned long height; //in pixels
	infoUpdateMap SharedInfoThread;
	int tileSizeInPixel;
	sem_t DataProtect;
	pthread_t ThId;
};



#endif /* MAP_H_ */
