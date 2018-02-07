/*
 * map.cpp
 *
 *  Created on: 25-Nov-2013
 *      Author: TLarrey
 */

#include "map.h"
#include "debug.h"
#include "generic.h"
#include "string.h"
#include "./bmp/array2bmp.hpp"

static int ** myVirtualMap=NULL;
int startX;
int startY;
int goalX;
int goalY;
static int bmp_count=0;


int ** allocateMap(int row, int col){

  int ** t = new int* [ row ];
  for (int i=0; i < row; i++)
    t[i] = new int[ col ];

  /* Initialisation */
  for (int i=0; i < row; i++)
    for (int j=0; j < col; j++)
      t[i][j] = FREE;

  return t;
}

void freeMap(int ** tab,int row, int col){
	if(tab==NULL) return;
	for (int i=0; i < row; i++)
	    delete[] tab[i];
	  delete[] tab;
}


void displayMap(int ** t,int row, int col){
	return;
	char buffer[20];
	sprintf(buffer,"./map_%d.bmp",bmp_count++);
	intarray2bmp::intarray2bmp(buffer, t, row, col, -1, 0 );
}

bool checkPointCoherency(int ** t,unsigned int *X,unsigned int *Y){
	if(!t) return false;
	if(t[*X][*Y]!=0) {
		return false;
	}
	return true;
}
#if 0
void updateObstacleInVirtualMap(int ** t, int x, int y, int size, int width, int height){

	//enlarge obstacle area to avoid too small steps
	int squareTLx = x-size;
	int squareTLy = y-size;
	int squareBRx = x+size;
	int squareBRy = y+size;
	if(squareTLx<0) squareTLx=0;
	if(squareTLy<0) squareTLy=0;
	if(squareBRx>(width)) squareBRx = (width);
	if(squareBRy>(height)) squareBRy = (height);
	for(int col=squareTLx;col<squareBRx;col++){
		for(int row=squareTLy;row<squareBRy;row++){
			t[col][row]=-1;
		}
	}
}
#else

bool updateObstacleInVirtualMap(int ** t, unsigned int x, unsigned int y, unsigned int size, unsigned int width, unsigned int height){
	//enlarge obstacle area to avoid too small steps
	int squareTLx = x-size;
	int squareTLy = y-size;
	unsigned int squareBRx = x+size;
	unsigned int squareBRy = y+size;
	if(squareTLx<0) squareTLx=0;
	if(squareTLy<0) squareTLy=0;
	if(squareBRx>=(width)) squareBRx = (width);
	if(squareBRy>=(height)) squareBRy = (height);
	if(x<0) x=0;
	if(x>=width) x=width-1;
	if(y<0) y=0;
	if(y>=height) y=height-1;

    if(t[x][y]!=-1) {
    	RPI_TRACE_DEBUG(SERVICE_MAP,"New obstacle detected [%d;%d]",x,y);
		for(int col=squareTLx;col<squareBRx;col++){
			for(int row=squareTLy;row<squareBRy;row++){
				t[col][row]=-1;
			}
		}
		return true;
    }
    return false;
}
#endif


infoUpdateMap::infoUpdateMap()
	:stopRequested(false),UpdatedAction(MAP_UPDATE_NO_ACTION),
	 x(0),y(0),CellVal(0),SecuritySize(10){
    SEM_INIT(SERVICE_MAP,&Sync,1,0);
    SEM_INIT(SERVICE_MAP,&Done,1,0);
    SEM_INIT(SERVICE_MAP,&ApiProtect,1,1);
}

infoUpdateMap::~infoUpdateMap(){

}

void infoUpdateMap::reset(){
	stopRequested=false;
	UpdatedAction=MAP_UPDATE_NO_ACTION;
	x=0;
	y=0;
	CellVal=0;
}
void infoUpdateMap::StopThread() {
	SEM_WAIT(SERVICE_MAP,&ApiProtect);
	stopRequested=true;
	setEvent();
	waitAnswer();
	SEM_POST(SERVICE_MAP,&ApiProtect);
}

void infoUpdateMap::setNextAction(t_UpdateMapAction action,unsigned int valx, unsigned int valy, int val) {
	SEM_WAIT(SERVICE_MAP,&ApiProtect);
	UpdatedAction= action;
	x=valx;
	y=valy;
	CellVal=val;
	setEvent();
	waitAnswer();
	SEM_POST(SERVICE_MAP,&ApiProtect);
}

void infoUpdateMap::setEvent(){
	SEM_POST(SERVICE_MAP,&Sync);
}
void infoUpdateMap::waitEvent(){
	SEM_WAIT(SERVICE_MAP,&Sync);
}
void infoUpdateMap::waitAnswer(){
	SEM_WAIT(SERVICE_MAP,&Done);
}
void infoUpdateMap::setAnswer(){
	SEM_POST(SERVICE_MAP,&Done);
}
void infoUpdateMap::updateArea(Dstar * pDS,unsigned int xTile,unsigned int yTile,int val)
{
	//enlarge obstacle area to avoid too small steps
	int squareTLx = xTile-SecuritySize;
	int squareTLy = yTile-SecuritySize;
	int squareBRx = xTile+SecuritySize;
	int squareBRy = yTile+SecuritySize;
	/*
	if(squareTLx<0) squareTLx=0;
	if(squareTLy<0) squareTLy=0;
	if(squareBRx>(width/tileSizeInPixel)) squareBRx = (width/tileSizeInPixel)-1;
	if(squareBRy>(height/tileSizeInPixel)) squareBRy = (height/tileSizeInPixel)-1;*/
	for(int col=squareTLx;col<squareBRx;col++){
		for(int row=squareTLy;row<squareBRy;row++){
			pDS->updateCell(col,row,val);
		}
	}
}

void* MapUpdateThread(void* coockies){
	virtualMap * pMap=(virtualMap*)coockies;
	infoUpdateMap* info = pMap->getSharedInfoThread();
	Dstar * pDstar= &pMap->mypath;
	pDstar->init(0,0,1,1);
	pDstar->replan();
	do{
		info->waitEvent();
		switch(info->getNextAction()){
			case infoUpdateMap::MAP_UPDATE_GOAL:
				pDstar->updateGoal(info->getX(),info->getY());
				pDstar->replan();
				break;
			case infoUpdateMap::MAP_UPDATE_CELL:
				pDstar->updateCell(info->getX(),info->getY(),info->getCellVal());
				pDstar->replan();
				break;
			case infoUpdateMap::MAP_UPDATE_START:
				pDstar->updateStart(info->getX(),info->getY());
				pDstar->replan();
				break;
			case infoUpdateMap::MAP_UPDATE_AREA:
				info->updateArea(pDstar,info->getX(),info->getY(),info->getCellVal());
				pDstar->replan();
				break;
			default: break;
		}
		info->reset();
		info->setAnswer();
	}while(info->getStopState()==false);
	return 0;
}

virtualMap::virtualMap(unsigned long pWidth,unsigned long pHeight)
	:width(pWidth),height(pHeight),tileSizeInPixel(1){
	bmp_count=0;
	myVirtualMap = allocateMap(width/tileSizeInPixel,height/tileSizeInPixel);
	if(pthread_create(&ThId,0,&MapUpdateThread, (void*)this )<0) {
		RPI_TRACE_ERROR(SERVICE_MAP,"Impossible to create a thread\n");
		exit(-1);
	}
}

virtualMap::~virtualMap(){
	void * thRet;
	SharedInfoThread.StopThread();
	pthread_join(ThId,&thRet);
	freeMap(myVirtualMap,width/tileSizeInPixel,height/tileSizeInPixel);
}

unsigned int virtualMap::convertMapIndexToPixel(unsigned int index){
	return(index*tileSizeInPixel);
}

unsigned int virtualMap::convertMapPixelToIndex(unsigned int pixel){
	return(pixel/tileSizeInPixel);
}

int virtualMap::storeVirtualMap(char * filename_wo_ext){
	//return;
	strcat(filename_wo_ext,".bmp");
	remove(filename_wo_ext);
	intarray2bmp::intarray2bmp(filename_wo_ext,myVirtualMap,width/tileSizeInPixel,height/tileSizeInPixel, -1, 0 );

	return 0;
}


int virtualMap::SetMapObstacle( t_map_position * objectPosition,int radian) {
	RPI_TRACE_API(SERVICE_MAP,"> %s",__func__);
    unsigned int x=objectPosition->coord.x;
    unsigned int y=objectPosition->coord.y;
    unsigned int xTile,yTile;
    int res=0;
    if (x < 0) x=0;
    if( x >= width) x=width-1;
    if (y < 0) y=0;
    if(y  >= height) y=height-1;

    xTile=convertMapPixelToIndex(x);
    yTile=convertMapPixelToIndex(y);
#if 0
    if(myVirtualMap[xTile][yTile]!=-1){
    	RPI_TRACE_INFO(SERVICE_MAP,"New obstacle detected pix[%d;%d] otil[%d,%d] stil[%d,%d] gtil[%d,%d]",
    			x,y,
    			xTile,yTile,
    			startX,startY,
    			goalX,goalY);
    	//SharedInfoThread.setNextAction(infoUpdateMap::MAP_UPDATE_CELL,xTile,yTile,-1);
    	SharedInfoThread.setNextAction(infoUpdateMap::MAP_UPDATE_AREA,xTile,yTile,-1);
    	updateObstacleInVirtualMap(myVirtualMap, xTile,yTile,10,width/tileSizeInPixel,height/tileSizeInPixel);
    	res=1;
    }
#else
    if(updateObstacleInVirtualMap(myVirtualMap, xTile,yTile,10,width/tileSizeInPixel,height/tileSizeInPixel)){
    	SharedInfoThread.setNextAction(infoUpdateMap::MAP_UPDATE_AREA,xTile,yTile,-1);
    	res=1;
    }
#endif
    RPI_TRACE_API(SERVICE_MAP,"< %s",__func__);
    return res;
}

bool virtualMap::setStartPosition( t_map_position * objectPosition ) {
	RPI_TRACE_API(SERVICE_MAP,"> %s",__func__);
    unsigned int x=objectPosition->coord.x;
    unsigned int y=objectPosition->coord.y;
    unsigned int xTile,yTile;
    bool status=true;

    if (x < 0) x=0;
    if( x >= width) x=width-1;
    if (y < 0) y=0;
    if(y  >= height) y=height-1;

    xTile=convertMapPixelToIndex(x);
    yTile=convertMapPixelToIndex(y);
	displayMap(myVirtualMap,width/tileSizeInPixel,height/tileSizeInPixel);
	if(checkPointCoherency(myVirtualMap,&xTile,&yTile)){
	    startX = xTile;startY=yTile;
		SharedInfoThread.setNextAction(infoUpdateMap::MAP_UPDATE_START,xTile,yTile,0);
		RPI_TRACE_DEBUG(SERVICE_MAP,"Start point updated [%d;%d]",x,y);
	}
	else {
		// it occurs due to the latency between obstacle detection and motor stop
		RPI_TRACE_DEBUG(SERVICE_MAP,"Unable to set start point to [%d;%d] : Obstacle detected",x,y);
	    status=false;
	}
    RPI_TRACE_API(SERVICE_MAP,"< %s",__func__);
    return status;
}

bool virtualMap::setGoalPosition( t_map_position * objectPosition ) {
	RPI_TRACE_API(SERVICE_MAP,"> %s",__func__);
    unsigned int x=objectPosition->coord.x;
    unsigned int y=objectPosition->coord.y;
    unsigned int xTile,yTile;
    bool status=true;

    if (x < 0) x=0;
    if( x >= width) x=width-1;
    if (y < 0) y=0;
    if(y  >= height) y=height-1;

    xTile=convertMapPixelToIndex(x);
    yTile=convertMapPixelToIndex(y);

	displayMap(myVirtualMap,width/tileSizeInPixel,height/tileSizeInPixel);
	if(checkPointCoherency(myVirtualMap,&xTile,&yTile)){
	    goalX = xTile;goalY=yTile;
		SharedInfoThread.setNextAction(infoUpdateMap::MAP_UPDATE_GOAL,xTile,yTile,0);
		RPI_TRACE_INFO(SERVICE_MAP,"Target point updated [%d;%d]",x,y);
	}
	else{
		// it occurs due to the latency between obstacle detection and motor stop
		RPI_TRACE_WARNING(SERVICE_MAP,"Impossible to set goal point to [%d;%d] : Obstacle detected",x,y);
		status=false;
	}
    RPI_TRACE_API(SERVICE_MAP,"< %s",__func__);
    return status;
}

void virtualMap::displayPath(t_map_position * list){
	t_map_position * pList=list;

	while(pList){
		RPI_TRACE_DEBUG(SERVICE_MAP,"at [x:%d][y:%d] set angle to %d degree to reach next point ",pList->coord.x,pList->coord.y,pList->angleToNextPoint);
		pList=pList->p_next;
	}
	RPI_TRACE_DEBUG(SERVICE_MAP,"NULL");
}

void virtualMap::deletePath(t_map_position *path){
	t_map_position * lPath=path;
	t_map_position * elt;
	while(lPath){
		elt=lPath;
		lPath=lPath->p_next;
		free(elt);
	}
}

void virtualMap::convertInPixel(t_map_position * list){
	t_map_position * pList=list;
	while(pList){
		pList->coord.x = convertMapIndexToPixel(pList->coord.x);
		pList->coord.y = convertMapIndexToPixel(pList->coord.y);
		pList=pList->p_next;
	}
}

t_map_position * virtualMap::computePath(void){
	list<state> path=mypath.getPath();
	list<state>::iterator iter;
	t_map_position * FirstElt = NULL;
	t_map_position * LastElt=NULL;
	t_map_position * currentElt=NULL;
	t_map_position previousElt;
	float totalAngle=0;
	float previousAngle=-9999;
	float computedAngle=0;

	RPI_TRACE_API(SERVICE_MAP,"> %s",__func__);

	for(iter = path.begin();iter != path.end();iter++)
	{
		if(iter == path.begin()){
			FirstElt = (t_map_position *) malloc(sizeof(t_map_position));
			FirstElt->coord.x=iter->x;
			FirstElt->coord.y=iter->y;
			FirstElt->p_next=NULL;
			previousElt.coord.x=iter->x;
			previousElt.coord.y=iter->y;
			LastElt=FirstElt;
		}
		else {
			if(currentElt==NULL) {
				currentElt=(t_map_position *) malloc(sizeof(t_map_position));
				previousElt.coord.x = LastElt->coord.x;
				previousElt.coord.y = LastElt->coord.y;
			}
			else {
				previousElt.coord.x = currentElt->coord.x;
				previousElt.coord.y = currentElt->coord.y;
			}

			if(currentElt) {
				currentElt->coord.x = iter->x;
				currentElt->coord.y = iter->y;
			}

			computedAngle=computeAngleForNextStep(previousElt.coord.x,previousElt.coord.y,iter->x,iter->y);
			totalAngle+=computedAngle;
			//printf("%f [%d;%d] -> [%d;%d] => Update =>\n",computedAngle,previousElt.coord.x,previousElt.coord.y,node->x,node->y);


			if(previousAngle==-9999) {
				FirstElt->angleToNextPoint=computedAngle;
				previousAngle=computedAngle;
			}
			else if(computedAngle!=previousAngle){
#if 0
			printf("\n\tAngle changed from %f to %f",previousAngle,computedAngle);
			printf("\n\tPrevious Position was [%d,%d]",previousElt.coord.x,previousElt.coord.y);
			printf("\n\tCurrent Position is [%d,%d]",currentElt->coord.x,currentElt->coord.y);
			printf("\n\tLast Elt Position is [%d,%d]",LastElt->coord.x,LastElt->coord.y);
			printf("\n");
#endif
				// create Elt 1
				t_map_position * Elt1 = (t_map_position *) malloc(sizeof(t_map_position));
				if(Elt1){
					Elt1->coord.x=previousElt.coord.x;
					Elt1->coord.y=previousElt.coord.y;
					Elt1->angleToNextPoint= computedAngle;
				}
				if(LastElt){
					LastElt->p_next =Elt1;
					LastElt=Elt1;
				}
				previousAngle=computedAngle;
			}
#if 0
			printf("AFTER First(0x%x) [%d;%d;0x%x] / LastElt(0x%x) [%d;%d;0x%x] / Previous(0x%x) [%d;%d;0x%x] Current(0x%x) [%d;%d;0x%x]\n",
					FirstElt,(FirstElt?FirstElt->coord.x:0),(FirstElt?FirstElt->coord.y:0),(FirstElt?FirstElt->p_next:0),
					LastElt,(LastElt?LastElt->coord.x:0),(LastElt?LastElt->coord.y:0),(LastElt?LastElt->p_next:0),
					&previousElt,previousElt.coord.x,previousElt.coord.y,previousElt.p_next,
					currentElt,(currentElt?currentElt->coord.x:0),(currentElt?currentElt->coord.y:0),(currentElt?currentElt->p_next:0)
					);
#endif
		}
	}
	if(currentElt) {
		currentElt->p_next=NULL;
		currentElt->angleToNextPoint=-9999;
		if(LastElt)
			LastElt->p_next=currentElt;
	}
	convertInPixel(FirstElt);
	RPI_TRACE_API(SERVICE_MAP,"< %s",__func__);
	return FirstElt;
}



