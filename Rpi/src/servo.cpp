/*
 * servo.cpp
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */
#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <unistd.h>
#include "servo.h"



Servo::Servo(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	drv_servo_reset();
	info.currentAngle=0;
	info.speed=0;
	info.state=Servo::Idle;
	info.infiniteMoveRequested=0;
	pList=NULL;
	pListCB = NULL;
	SEM_INIT(SERVICE_SERVO,&dataProtect,1,1);
	SEM_INIT(SERVICE_SERVO,&api_protect,1,1);
	SEM_INIT(SERVICE_SERVO,&list_protect,1,1);
	SEM_INIT(SERVICE_SERVO,&endOfMoving,1,0);

	strncpy(ConvertCmd2String[CMD_SET_ANGLE],"set_angle",20);
	strncpy(ConvertCmd2String[CMD_GET_ANGLE],"get_angle",20);
	strncpy(ConvertCmd2String[CMD_START_MOVE],"start",20);
	strncpy(ConvertCmd2String[CMD_STOP_MOVE],"stop",20);
	setAngle(90,Servo::Synchronous);
	RPI_TRACE_DEBUG(SERVICE_SERVO,"[Servo 0x%x] created",this);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
}

Servo::~Servo(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	int vl_currentState,vl_stopNeeded;
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	vl_currentState=info.state;
	vl_stopNeeded=info.infiniteMoveRequested;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	if(vl_currentState==Servo::Moving && vl_stopNeeded) {
		stopInfiniteMove();
		info.state=Servo::Idle;
	}
	checkAndWaitPendingThread();
	SEM_WAIT(SERVICE_SERVO,&api_protect);
	drv_servo_reset();
	SEM_POST(SERVICE_SERVO,&api_protect);
	t_registerCB * listCB = pListCB;
	while(listCB) {
		t_registerCB * previous=listCB;
		free(listCB);
		listCB=previous->nextCB;
	}
	RPI_TRACE_DEBUG(SERVICE_SERVO,"[Servo 0x%x] deleted",this);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
}

int Servo::treatCommand(t_RPI_command* command,t_RPI_result * result){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_SERVO,"Command to treat : %s",command->param[0]);

	int i=0;
	char info[50];
	result->status =0;
	strncpy(result->msg,"Done",MAX_RESULT_STRING_LENGTH);
	for(i=0;i<CMD_NB_CMD;i++){
	  if(strcmp(command->param[0],ConvertCmd2String[i])==0){
		  break;
	  }
	}
	switch(i) {
			case CMD_SET_ANGLE :
				int angle,mode;
				result->status = -1;
				mode=Servo::Synchronous;
				if(strncmp(command->param[2],"asynchronous",strlen("asynchronous"))==0){
					mode=Servo::Asynchronous;
				}
				if((angle=atoi(command->param[1]))!=0) {
					result->status = setAngle(angle,mode);
				}
				if(result->status<0){
					strncpy(info,"Unable to move servo",50);
				}
				break;
			case CMD_GET_ANGLE :
				result->info[0] = getAngle();
				break;
			case CMD_START_MOVE :
				result->status = startInfiniteMove(10);
				if(result->status<0){
					strncpy(info,"Unable to set infinite move mode",50);
				}
				break;
			case CMD_STOP_MOVE :
				result->status = stopInfiniteMove();
				if(result->status<0){
					strncpy(info,"Unable to stop infinite move mode",50);
				}
				break;
			default:
				result->status = -1;
				strncpy(info,"Unknown command",50);
				break;
	}
	if(result->status<0){
		strncpy(result->msg,info,MAX_RESULT_STRING_LENGTH);
		RPI_TRACE_ERROR(SERVICE_SERVO,"%s",info);
	}


	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

extern "C" void * asyncSetAngle (void * arg) {
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	((Servo*)arg)->parseList();
	pthread_exit(0);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return(0);
}

void Servo::get_status(t_servoInfo * pInfo) {
	if(pInfo)
		memcpy(pInfo,(const void*)&info,sizeof(t_servoInfo));
}

void Servo::parseList(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	SEM_WAIT(SERVICE_SERVO,&list_protect);
	t_AngleSet * pl_list=pList;
	SEM_POST(SERVICE_SERVO,&list_protect);

	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	if(info.state==Servo::Moving) {
		SEM_POST(SERVICE_SERVO,&dataProtect);
		SEM_WAIT(SERVICE_SERVO,&endOfMoving);
		SEM_WAIT(SERVICE_SERVO,&dataProtect);
	}
	info.state=Servo::Moving;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	while(pl_list) {
		if(!internalSetAngle(pl_list->angle)){
			RPI_TRACE_DEBUG(SERVICE_SERVO,"Angle set to %d degree",pl_list->angle);
		}
		SEM_WAIT(SERVICE_SERVO,&list_protect);
		pl_list=removeFromThreadList(pl_list);
		SEM_POST(SERVICE_SERVO,&list_protect);
	}
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	info.state=Servo::Idle;
	// Just to wake up thread if pending tasks exist
	if(info.infiniteMoveRequested){
		SEM_POST(SERVICE_SERVO,&endOfMoving);
	}
	SEM_POST(SERVICE_SERVO,&dataProtect);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
}

void Servo::checkAndWaitPendingThread(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	void *status;
	SEM_WAIT(SERVICE_SERVO,&list_protect);
	t_AngleSet * pl_list=pList;
	SEM_POST(SERVICE_SERVO,&list_protect);

	if(pl_list) {
		RPI_TRACE_INFO(SERVICE_SERVO,"Wait for end of treatments queued in thread");
		if(pthread_join ((unsigned long int)thId, &status)){
			RPI_TRACE_ERROR(SERVICE_SERVO,"!!!ERROR in joining");
			RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
			exit(-1);
		}
	}
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
}

struct s_AngleSet * Servo::removeFromThreadList(t_AngleSet * pElt){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	t_AngleSet * pprevious=NULL;
	t_AngleSet * pcurrent=pList;
	while(pcurrent){
		if(pcurrent==pElt){
			free(pElt);
			if(pprevious) {
				pprevious->next=pcurrent->next;
			}
			else {
				pList=pcurrent->next;
			}
			break;
		}
		else {
			pprevious=pcurrent;
			pcurrent=pprevious->next;
		}
	}
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return(pList);
}

struct s_AngleSet * Servo::QueueEndofThreadList(t_AngleSet * pElt){
	t_AngleSet * list = pList;
	t_AngleSet * previous = NULL;
	while(list) {
		previous=list;
		list=previous->next;
	}
	if(previous){
		previous->next=pElt;
	}
	else {
		pList=pElt;
	}
	return pList;
}

int Servo::setAngle(int angle,int mode){
	RPI_TRACE_API(SERVICE_SERVO,">%s [angle:%d][mode:%d]",__func__,angle,mode);
	int res=-1;
	int vl_currentState;
	SEM_WAIT(SERVICE_SERVO,&api_protect);
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	vl_currentState=info.state;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	if(angle<=180) {
		if(mode==Servo::Synchronous){
			if(vl_currentState==Servo::Moving) {
				RPI_TRACE_ERROR(SERVICE_SERVO,"setAngle : request synchronous service when servo is already moving");
				SEM_POST(SERVICE_SERVO,&api_protect);
				RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
				return -1;
			}
			res=internalSetAngle(angle);
			if(!res)
				RPI_TRACE_DEBUG(SERVICE_SERVO,"Angle set to %d degree",info.currentAngle);
		}
		else {
			SEM_WAIT(SERVICE_SERVO,&list_protect);
			t_AngleSet * info = (t_AngleSet*)malloc(sizeof(t_AngleSet));
			t_AngleSet * list=pList;
			info->angle=angle;
			info->next=NULL;
			// Add new task at the beginning of the list
			pList=QueueEndofThreadList(info);
			RPI_TRACE_INFO(SERVICE_SERVO,"Queue new angle : %d (List %s null)",angle,(list==NULL?"was":"wasn't"));
			if( !list){
				RPI_TRACE_DEBUG(SERVICE_SERVO,"Create thread");
				if (pthread_create (&thId, NULL, asyncSetAngle, (void*)this) < 0) {
					RPI_TRACE_ERROR(SERVICE_SERVO,"pthread_create error for thread 2");
					SEM_POST(SERVICE_SERVO,&api_protect);
					SEM_POST(SERVICE_SERVO,&list_protect);
					RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
					exit (1);
				}
			}
			SEM_POST(SERVICE_SERVO,&list_protect);
		}
	}
	SEM_POST(SERVICE_SERVO,&api_protect);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return res;
}

int Servo::internalSetAngle(int angle){
	RPI_TRACE_API(SERVICE_SERVO,">%s [angle:%d]",__func__,angle);
	int delay=0;
	if(angle==info.currentAngle) return 0;
	if(angle<info.currentAngle){
		// move from right to left
		delay=(info.currentAngle-angle)*1000/ANGLE_SPEED_DEG_PER_SECOND_DESC; // in ms
		if(!drv_servo_move(LEFT,delay)) {
			SEM_WAIT(SERVICE_SERVO,&dataProtect);
			info.currentAngle=angle;
			InformPositionSet(angle);
			SEM_POST(SERVICE_SERVO,&dataProtect);
		}
	}
	else{
		// move from left to right
		delay=(angle-info.currentAngle)*1000/ANGLE_SPEED_DEG_PER_SECOND_ASC; // in ms
		if(!drv_servo_move(RIGHT,delay)) {
			SEM_WAIT(SERVICE_SERVO,&dataProtect);
			info.currentAngle=angle;
			InformPositionSet(angle);
			SEM_POST(SERVICE_SERVO,&dataProtect);
		}
	}
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}


int Servo::getAngle(){
	int angle;
	SEM_WAIT(SERVICE_SERVO,&api_protect);
	angle=info.currentAngle;
	SEM_POST(SERVICE_SERVO,&api_protect);
	return angle;
}

void * Servo::registerPositionSet(t_callback_servo_position_fct pCB,void * coockies){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	RPI_TRACE_INFO(SERVICE_SERVO,"Callback registered for servo update");
	t_registerCB * pElt=(t_registerCB *)malloc(sizeof(t_registerCB));
	t_registerCB * list = pListCB;
	t_registerCB * previous=NULL;
	pElt->callback = pCB;
	pElt->coockies = coockies;
	pElt->nextCB = NULL;
	while(list) { previous = list; list=list->nextCB;}
	if(previous) previous->nextCB=pElt;
	else {
		pListCB = pElt;
	}
	RPI_TRACE_INFO(SERVICE_SERVO,"return handle : 0x%x",pElt);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return (pElt);
}

int Servo::unregisterPositionSet(void * handle){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	t_registerCB * list = pListCB;
	t_registerCB * previous=NULL;
	t_registerCB * pElt = (t_registerCB *)handle;

	while(list) {
		if(list == pElt){
			if(previous) {
				previous->nextCB = list->nextCB;
			}
			else {
				pListCB = list->nextCB;
			}
			free(pElt);
			RPI_TRACE_INFO(SERVICE_SERVO,"Callback 0x%x unregistered",handle);
			RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
			return 0;
		}
		previous = list;
		list=list->nextCB;
	}
	RPI_TRACE_WARNING(SERVICE_SERVO,"Callback 0x%x not found",handle);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return -1;
}

int Servo::InformPositionSet(int angle){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	t_registerCB * list = pListCB;
	while(list) {
		if(list->callback) {
			list->callback(angle,list->coockies);
		}
		list=list->nextCB;
	}
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

extern "C" void * servo_move (void * arg) {
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	((Servo*)arg)->move();
	pthread_exit(0);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return(0);
}

int Servo::startInfiniteMove(int pSpeed){
	RPI_TRACE_INFO(SERVICE_SERVO,">%s",__func__);
	SEM_WAIT(SERVICE_SERVO,&api_protect);
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	if(info.infiniteMoveRequested){
		SEM_POST(SERVICE_SERVO,&dataProtect);
		SEM_POST(SERVICE_SERVO,&api_protect);
		RPI_TRACE_WARNING(SERVICE_SERVO,"!!! Servo already configured in Infinite Move");
		RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
		return 0;
	}
	info.infiniteMoveRequested=1;
	if(info.state==Servo::Moving) {
		SEM_POST(SERVICE_SERVO,&dataProtect);
		SEM_POST(SERVICE_SERVO,&api_protect);
		RPI_TRACE_INFO(SERVICE_SERVO,"startInfiniteMove : Wait for end of previous treatment");
		// wait for end of previous move
		SEM_WAIT(SERVICE_SERVO,&endOfMoving);
	}
	info.speed=pSpeed;
	info.stopRequested=0;
	info.state=Servo::Moving;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	if (pthread_create (&thIdInfinite, NULL, servo_move, (void*)this) < 0) {
		RPI_TRACE_ERROR(SERVICE_SERVO,"pthread_create error for thread 2");
		SEM_POST(SERVICE_SERVO,&api_protect);
		RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
		exit (1);
	}
	SEM_POST(SERVICE_SERVO,&api_protect);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

int Servo::stopInfiniteMove(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	SEM_WAIT(SERVICE_SERVO,&api_protect);
	void *status;
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	if(!info.infiniteMoveRequested) {
		SEM_POST(SERVICE_SERVO,&dataProtect);
		SEM_POST(SERVICE_SERVO,&api_protect);
		RPI_TRACE_WARNING(SERVICE_SERVO,"stopInfiniteMove !!! Servo NOT configured in Infinite Move");
		RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
		return 0;
	}
	info.stopRequested=1;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	if(pthread_join ((unsigned long int)thIdInfinite, &status)){
		RPI_TRACE_ERROR(SERVICE_SERVO,"stopInfiniteMove !!!ERROR in joining infinite mode thread");
		SEM_POST(SERVICE_SERVO,&api_protect);
		RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
		exit(-1);
	}
	SEM_WAIT(SERVICE_SERVO,&dataProtect);
	info.infiniteMoveRequested=0;
	SEM_POST(SERVICE_SERVO,&dataProtect);
	SEM_WAIT(SERVICE_SERVO,&list_protect);
	if(pList!=NULL) {
		// Just to wake up thread if pending tasks exist
		SEM_POST(SERVICE_SERVO,&endOfMoving);
	}
	SEM_POST(SERVICE_SERVO,&list_protect);

	SEM_POST(SERVICE_SERVO,&api_protect);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

int Servo::move(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
    int angle=info.currentAngle;
    int target=180;
    int vlStopReq;
    SEM_WAIT(SERVICE_SERVO,&dataProtect);
    vlStopReq=info.stopRequested;
    SEM_POST(SERVICE_SERVO,&dataProtect);
    RPI_TRACE_DEBUG(SERVICE_SERVO,"[DRV] Start infinite move from %d degree",angle);
    while(!vlStopReq) {
    	if(angle<target) angle++;
		else if(angle==target){
			if(target==180) {
				target=0;angle--;
				RPI_TRACE_DEBUG(SERVICE_SERVO,"[DRV] Right side reached");
			}
			else {
				target=180;angle++;
				RPI_TRACE_DEBUG(SERVICE_SERVO,"[DRV] Left side reached");
			}
		}
		else angle--;

    	internalSetAngle(angle);
    	SEM_WAIT(SERVICE_SERVO,&dataProtect);
    	vlStopReq=info.stopRequested;
    	SEM_POST(SERVICE_SERVO,&dataProtect);
    }
	info.state=Servo::Idle;
    RPI_TRACE_DEBUG(SERVICE_SERVO,"[DRV] Stop infinite move to %d degree",info.currentAngle);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

int Servo::drv_servo_prepare(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

int Servo::drv_servo_reset(){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	// move sensor up to maximum let position = 0 degree
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

int Servo::drv_servo_move(int direction,int delay){
	RPI_TRACE_API(SERVICE_SERVO,">%s",__func__);
	//RPI_TRACE_DEBUG(SERVICE_SERVO,"[DRV] Move to %s during %d ms",(direction==LEFT?"left":"right"),delay);
	usleep(delay*1000);
	RPI_TRACE_API(SERVICE_SERVO,"<%s",__func__);
	return 0;
}

