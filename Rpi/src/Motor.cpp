/*
 * Motor.cpp
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */

#include "Motor.h"
#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <unistd.h>


Motor::Motor() {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	state=IDLE;
	current_speed=DEFAULT_MOTOR_SPEED;
	listActions = NULL;
	reverse=1;
	strcpy(name,"No_name");
	SEM_INIT(SERVICE_MOTOR,&api_protect,1,1);
	SEM_INIT(SERVICE_MOTOR,&list_protect,1,1);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor 0x%x] created",this);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
}

Motor::~Motor() {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	reset();
	SEM_WAIT(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor 0x%x] destructed",this);
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
}

void Motor::mainLoop(){

}

void * Motor::addAction(t_motor_action act,int param1,float param2) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	t_info_action * pList;
	t_info_action * pPrevious;

	t_info_action * newElt = (t_info_action *)malloc(sizeof(t_info_action));
	memset(newElt,0,sizeof(t_info_action));
	newElt->action=act;
	newElt->param1=param1;
	newElt->param2=param2;
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] Allocate and queue action %s params [%d,%f] ",name,convertActionToStr(newElt->action),newElt->param1,newElt->param2);

	SEM_WAIT(SERVICE_MOTOR,&list_protect);
	pList=listActions;
	pPrevious=NULL;
	while(pList) {
		pPrevious=pList;
		pList=pPrevious->next;
	}
	if(pPrevious) pPrevious->next=newElt;
	else listActions=newElt;
	SEM_POST(SERVICE_MOTOR,&list_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return newElt;
}

int Motor::reset(){
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	SEM_WAIT(SERVICE_MOTOR,&api_protect);
	ExecuteListedAction(0);
	if(state==STARTED) {
		drv_motor_stop();
		state=IDLE;
		current_speed=DEFAULT_MOTOR_SPEED;
	}
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}



int Motor::ExecuteListedAction(int execute) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	t_info_action * pList;
	t_info_action * pPrevious;

	SEM_WAIT(SERVICE_MOTOR,&list_protect);
	if(!listActions) goto out;
	pList=listActions;
	pPrevious=NULL;
	while(pList) {
		RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] %saction %s with params [%d ; %f]",name,(execute?"Execute ":"Unqueue "),convertActionToStr(pList->action),pList->param1,pList->param2);
		switch(pList->action) {
			case STOP:
				if(state==STARTED) {
					drv_motor_stop();
					current_speed=DEFAULT_MOTOR_SPEED;
					state=IDLE;
				}
				break;
			case START:
				if(execute && state==IDLE){
					drv_motor_start(current_speed);
					state=STARTED;
				}
				break;
			case SET_SPEED:
				if(execute) {
					res=0;
					if(state==STARTED) {
						res=drv_motor_set_speed(pList->param1);
					}
					if(res>=0){
						current_speed=pList->param2;
					}
				}
				break;
			case DELAY:
				if(execute) {
					// param in ms
					RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] Delay : %dms",name,pList->param1);
					usleep(pList->param1*1000);
				}
				break;
			case ROTATION:
				if(execute) {
					drv_motor_do_rotation(pList->param2);
				}
				break;
			default: break;
		}

		pPrevious=pList;
		pList=pPrevious->next;
		RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] Remove action %s ",name,convertActionToStr(pPrevious->action));
		free(pPrevious);
	}
	listActions=NULL;
out:
	SEM_POST(SERVICE_MOTOR,&list_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::start(t_motor_sync_mode mode) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	SEM_WAIT(SERVICE_MOTOR,&api_protect);
	if(mode==WAIT_FOR_SYNC) {
		addAction(Motor::START,0,0);
	}
	else {
		if(state==STARTED) goto out;
		res=drv_motor_start(current_speed);
		state=STARTED;
	}
out:
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::stop(t_motor_sync_mode mode) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	SEM_WAIT(SERVICE_MOTOR,&api_protect);

	if(mode==WAIT_FOR_SYNC) {
		addAction(Motor::STOP,0,0);
	}
	else {
		if(state==IDLE) goto out;
		res=drv_motor_stop();
		current_speed=DEFAULT_MOTOR_SPEED;
		state=IDLE;
	}
out:
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::setSpeed(float speed,t_motor_sync_mode mode) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	// speed is a percent between -100 and 100 (reverse or forward)
	float vl_speed=0;

	vl_speed=(float)((float)MOTOR_RPS*(float)speed)/100;

	SEM_WAIT(SERVICE_MOTOR,&api_protect);

	if(mode==WAIT_FOR_SYNC) {
		addAction(Motor::SET_SPEED,0,vl_speed);
	}
	else {
		res=0;
		if(state==STARTED){
			res=drv_motor_set_speed(vl_speed);
		}
		if(res>=0) {
			current_speed=vl_speed;
		}
	}
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::setDelay(int delayMs,t_motor_sync_mode mode){
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	SEM_WAIT(SERVICE_MOTOR,&api_protect);

	if(mode==WAIT_FOR_SYNC) {
		addAction(Motor::DELAY,delayMs,0);
	}
	else {
		res=0;
		RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] Delay : %dms",name,delayMs);
		usleep(delayMs*1000);
	}
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}


int Motor::doRotation(float nbRotation,t_motor_sync_mode mode){
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	SEM_WAIT(SERVICE_MOTOR,&api_protect);

	if(mode==WAIT_FOR_SYNC) {
		addAction(Motor::ROTATION,0,nbRotation);
	}
	else {
		res=drv_motor_do_rotation(nbRotation);
	}
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::syncSignal(void) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int res=-1;
	SEM_WAIT(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[Motor %s] Sync requested",name);
	res=ExecuteListedAction(1);
	SEM_POST(SERVICE_MOTOR,&api_protect);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return res;
}

int Motor::setName(const char * pName){
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"Name is %s ",pName);
	strcpy(name,pName);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}

const char * Motor::convertActionToStr(t_motor_action act) {
	switch(act) {
		case STOP: return "STOP";break;
		case START: return "START";break;
		case SET_SPEED:return "SET SPEED";break;
		case DELAY: return "SET DELAY";break;
		case ROTATION: return "DO ROTATION";break;
		default: return "Unknown";break;
	}
	return "UNKNOWN";
}


int Motor::drv_motor_start(float speed) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[DRV %s] started at speed : %03.02f rps",name,speed);
	if(speed<0){
		reverse=-1;
	}
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}

int Motor::drv_motor_stop(void) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[DRV %s] stopped",name);
	reverse=1;
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}

int Motor::drv_motor_set_speed(float speed) {
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[DRV %s] speed updated to %03.02f rps",name,speed);
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}
int Motor::drv_motor_do_rotation(float nbRotation){
	RPI_TRACE_API(SERVICE_MOTOR,">%s",__func__);
	int delay=0;
	int direction=1;
	float vl_rotation=nbRotation;
	if(state==STARTED) {
		drv_motor_stop();
	}
	if(nbRotation<0) {
		direction=-1;
		vl_rotation=(-1)*nbRotation;
	}
	//Force motor speed to half
	delay=(int)(vl_rotation*1000000/(MOTOR_RPS>>1));
	RPI_TRACE_DEBUG(SERVICE_MOTOR,"[DRV %s] do %f rotation(s) => rotate %s during %d us",name,vl_rotation,(direction==1?"forward":"backward"),delay);
	// 1sec = 1000000us
	drv_motor_start(direction*(MOTOR_RPS>>1));
	usleep(delay);
	drv_motor_stop();

	if(state==STARTED) {
		drv_motor_start(current_speed);
	}
	RPI_TRACE_API(SERVICE_MOTOR,"<%s",__func__);
	return 0;
}
//=======================================

DualMotors::DualMotors(int x, int y){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"instance created");
	motor[LEFT].setName("LEFT");
	motor[RIGHT].setName("RIGHT");
	motor[LEFT].reset();
	motor[RIGHT].reset();
	state=DualMotors::STOPPED;
	pathToFollow = NULL;
	setSpeed(50);
	CTurnCWheelRatio[LEFT]=(float)((float)DIST_BETWEEN_WHEELS_CM)/((float)WHEELS_DIAMETRE_CM);
	CTurnCWheelRatio[RIGHT]=(float)((float)DIST_BETWEEN_WHEELS_CM)/((float)WHEELS_DIAMETRE_CM);

	strncpy(ConvertCmd2String[CMD_INIT],"init",20);
	strncpy(ConvertCmd2String[CMD_RESET],"reset",20);
	strncpy(ConvertCmd2String[CMD_MOVE_FORWARD],"move_forward",20);
	strncpy(ConvertCmd2String[CMD_MOVE_BACKWARD],"move_backward",20);
	strncpy(ConvertCmd2String[CMD_STOP],"stop",20);
	strncpy(ConvertCmd2String[CMD_GET_SPEED],"get_speed",20);
	strncpy(ConvertCmd2String[CMD_SET_SPEED],"set_speed",20);
	strncpy(ConvertCmd2String[CMD_TURN],"turn",20);
	strncpy(ConvertCmd2String[CMD_GET_STATE],"get_state",20);
	strncpy(ConvertCmd2String[CMD_GET_POSITION],"get_position",20);

	pos.x=x;
	pos.y=y;
	updateCurrentTime(&pos);
	pos.angle=0;
	PositionUpdateOnGoing=false;
	fct_PathCB=NULL;
	coockiesCB=NULL;
	SEM_INIT(SERVICE_DUALMOTOR,&updatePositionProtect,1,1);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"L:%f / R:%f",CTurnCWheelRatio[LEFT],CTurnCWheelRatio[RIGHT]);
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
}

DualMotors::~DualMotors(){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	stop();
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"instance deleted");
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
}

int DualMotors::treatCommand(t_RPI_command* command,t_RPI_result * result){
	int i=0;
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Command to treat : %s",command->param[0]);
	result->status =0;
	strncpy(result->msg,"Done",MAX_RESULT_STRING_LENGTH);
	for(i=0;i<CMD_NB_CMD;i++){
	  if(strcmp(command->param[0],ConvertCmd2String[i])==0){
	    			break;
	  }
    }
	switch(i) {
		case CMD_INIT :	break;
		case CMD_RESET : break;
		case CMD_MOVE_FORWARD :	move_forward();	break;
		case CMD_MOVE_BACKWARD : move_backward();break;
		case CMD_STOP : stop();break;
		case CMD_GET_SPEED :
			result->info[0]=getSpeed();
			break;
		case CMD_SET_SPEED :
			int speed;
			result->status = -1;
			if((speed=atoi(command->param[1]))!=0) {
				result->status=setSpeed(speed);
			}
			if(result->status<0){
				char info[50]="Unable to set the speed";
				strncpy(result->msg,info,MAX_RESULT_STRING_LENGTH);
				RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"%s",info);
			}
			break;
		case CMD_TURN :
			int angle;
			result->status = -1;
			if((angle=atoi(command->param[1]))!=0) {
				result->status=turn(angle);
			}
			if(result->status<0){
				char info[50]="Unable to turn";
				strncpy(result->msg,info,MAX_RESULT_STRING_LENGTH);
				RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"%s",info);
			}
			break;
		case CMD_GET_STATE :
			result->info[0]=state;
			result->info[1]=(int)current_speed;
			SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
			result->info[2]= pos.x;
			result->info[3]= pos.y;
			result->info[4]= pos.angle;
			SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
			break;
		case CMD_GET_POSITION :
			result->info[0]=pos.x;
			result->info[1]=pos.y;
			result->info[2]=pos.angle;
			break;
	default:
		result->status = -1;
		strncpy(result->msg,"Unknown command",MAX_RESULT_STRING_LENGTH);
		break;
	}
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}


int DualMotors::get_status(t_dualmotors_status * pStatus){
	if(!pStatus) return -1;
	pStatus->current_speed = current_speed;
	SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
	pStatus->pos.angle = pos.angle;
	pStatus->pos.x = pos.x;
	pStatus->pos.y = pos.y;
	SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
	pStatus->state = state;
	return 0;
}

// speed between 0 and 100 in percent
int DualMotors::setSpeed(int speed){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	int vl_speed=speed;
	if(vl_speed<0) vl_speed=-1*speed;
	if(vl_speed>100) vl_speed=100;
	RPI_TRACE_INFO(SERVICE_DUALMOTOR,"setSpeed(%d)",speed);
	motor[LEFT].setSpeed(adaptSpeed(LEFT,(float)vl_speed),Motor::IMMEDIATE);
	motor[RIGHT].setSpeed(adaptSpeed(RIGHT,(float)vl_speed),Motor::IMMEDIATE);
	current_speed=vl_speed;
	updatePosition();
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}

float DualMotors::adaptSpeed(t_dualmotors_pos pos,float speed){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	// adapt speed according to the motors
	float updatedSpeed=0;
	if(pos==LEFT) {
		updatedSpeed = speed+(speed*MOTORL_CORRECTION);
		RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Speed updated for LEFT motor : %02.02f%% (request : %02.0f%%)",updatedSpeed,speed);
	}
	else if(pos==RIGHT) {
		updatedSpeed = speed+(speed*MOTORR_CORRECTION);
		RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Speed updated for RIGHT motor : %02.02f%% (request : %02.0f%%)",updatedSpeed,speed);
	}
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return(updatedSpeed);
}

int DualMotors::move_forward(){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"move_forward");
	if(state==DualMotors::STARTED_FORWARD) {
		RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s Already moving forward",__func__);
		return 0;
	}
	motor[LEFT].setSpeed(adaptSpeed(LEFT,current_speed),Motor::IMMEDIATE);
	motor[RIGHT].setSpeed(adaptSpeed(RIGHT,current_speed),Motor::IMMEDIATE);
	motor[LEFT].start(Motor::IMMEDIATE);
	motor[RIGHT].start(Motor::IMMEDIATE);
	state=DualMotors::STARTED_FORWARD;
	StartPositionUpdate(true);
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}

int DualMotors::move_backward(){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"move_backward");
	if(state==DualMotors::STARTED_BACKWARD) {
		RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
		return 0;
	}
	motor[LEFT].setSpeed(adaptSpeed(LEFT,-1*current_speed),Motor::IMMEDIATE);
	motor[RIGHT].setSpeed(adaptSpeed(RIGHT,-1*current_speed),Motor::IMMEDIATE);
	motor[LEFT].start(Motor::IMMEDIATE);
	motor[RIGHT].start(Motor::IMMEDIATE);
	state=DualMotors::STARTED_BACKWARD;
	StartPositionUpdate(true);
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}

int DualMotors::stop(){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"stop()");
	if(state!=DualMotors::STOPPED) {
		motor[LEFT].stop(Motor::IMMEDIATE);
		motor[RIGHT].stop(Motor::IMMEDIATE);
	}
	else {
		motor[LEFT].reset();
		motor[RIGHT].reset();
	}
	state=DualMotors::STOPPED;
	StopPositionUpdate();
	pathToFollow = NULL;
	fct_PathCB=NULL;
	coockiesCB=NULL;
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}



/*
* b : distance entre les roues
* Cturn : circonference complete si le robot fait un tour sur lui-meme
	Cturn = b x π
* Ang : Desired turn angle
* dCT : Distance along Cturn
* dw : Wheel Diameter
* Cwheel Wheel Circumference
 	Cwheel = dw x π
* MR : Motor Rotation 360° degrees around a circle
*
* Number of Rotations to Program = (Cturn) / (Cwheel) x (Ang) / (360°)
* Number of Degrees to Program = (Cturn) / (Cwheel) x (Ang)
*
*  For a rotation of X deg of the robot in one direction, both wheels have to be program
*  with the same values except the sign : one is moving in one direction, the other in the other
*  direction
*/

int DualMotors::turn(int angle){
	RPI_TRACE_API(SERVICE_DUALMOTOR,">%s Turn(%d deg)",__func__,angle);
	// angle is the circular move from the current direction
	float vl_angleRatio[NB_MOTORS];
	float vl_NbRotation[NB_MOTORS];

	t_dualmotors_state previousState=state;

	if(state==DualMotors::ROTATION_ON_GOING){
		RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
		return -1;
	}
	angle=(angle+360)%360;

	if (!angle || angle==360) {
		RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
		return 0;
	}
	state=DualMotors::ROTATION_ON_GOING;
	if(angle <= 180 ){
		RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Turn right (%d deg)",angle);
		//left motor forward / right motor backward
		vl_angleRatio[LEFT] = (float)((float)angle/(float)FULL_ROTATION);
		vl_angleRatio[RIGHT] = vl_angleRatio[LEFT]*(-1);
	}
	else {
		RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Turn left (%d deg)",(360-angle));
		//left motor backward / right motor forward
		vl_angleRatio[RIGHT] = (float)((360-(float)angle)/(float)FULL_ROTATION);
		vl_angleRatio[LEFT] = vl_angleRatio[RIGHT]*(-1);
	}
	vl_NbRotation[LEFT]=CTurnCWheelRatio[LEFT]*vl_angleRatio[LEFT];
	vl_NbRotation[RIGHT]=CTurnCWheelRatio[RIGHT]*vl_angleRatio[RIGHT];
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"[DualMotor] NbRotation requested Left:%f Right:%f",vl_NbRotation[LEFT],vl_NbRotation[RIGHT]);
	motor[LEFT].doRotation(vl_NbRotation[LEFT],Motor::IMMEDIATE);
	motor[RIGHT].doRotation(vl_NbRotation[RIGHT],Motor::IMMEDIATE);
	state=previousState;

	pos.angle = (pos.angle+angle)%360;
	updateCurrentTime(&pos);
	updatePosition();
	RPI_TRACE_API(SERVICE_DUALMOTOR,"<%s",__func__);
	return 0;
}


int DualMotors::PositionLoop(){
	bool stopRequested=false;
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Update robot position thread started");
	do {
		usleep(50000);
		stopRequested=updatePosition();
	}while(!stopRequested);
	RPI_TRACE_DEBUG(SERVICE_DUALMOTOR,"Update robot position thread stopped");
	return 0;
}

static const float tab_cos[72][2] = {
		{1.0000000000 , 0.0000000000 },
		{0.9961946981 , 0.0871557427 },
		{0.9848077530 , 0.1736481777 },
		{0.9659258263 , 0.2588190451 },
		{0.9396926208 , 0.3420201433 },
		{0.9063077870 , 0.4226182617 },
		{0.8660254038 , 0.5000000000 },
		{0.8191520443 , 0.5735764364 },
		{0.7660444431 , 0.6427876097 },
		{0.7071067812 , 0.7071067812 },
		{0.6427876097 , 0.7660444431 },
		{0.5735764364 , 0.8191520443 },
		{0.5000000000 , 0.8660254038 },
		{0.4226182617 , 0.9063077870 },
		{0.3420201433 , 0.9396926208 },
		{0.2588190451 , 0.9659258263 },
		{0.1736481777 , 0.9848077530 },
		{0.0871557427 , 0.9961946981 },
		{0.0000000000 , 1.0000000000 },
		{-0.0871557427 , 0.9961946981 },
		{-0.1736481777 , 0.9848077530 },
		{-0.2588190451 , 0.9659258263 },
		{-0.3420201433 , 0.9396926208 },
		{-0.4226182617 , 0.9063077870 },
		{-0.5000000000 , 0.8660254038 },
		{-0.5735764364 , 0.8191520443 },
		{-0.6427876097 , 0.7660444431 },
		{-0.7071067812 , 0.7071067812 },
		{-0.7660444431 , 0.6427876097 },
		{-0.8191520443 , 0.5735764364 },
		{-0.8660254038 , 0.5000000000 },
		{-0.9063077870 , 0.4226182617 },
		{-0.9396926208 , 0.3420201433 },
		{-0.9659258263 , 0.2588190451 },
		{-0.9848077530 , 0.1736481777 },
		{-0.9961946981 , 0.0871557427 },
		{-1.0000000000 , 0.0000000000 },
		{-0.9961946981 , -0.0871557427 },
		{-0.9848077530 , -0.1736481777 },
		{-0.9659258263 , -0.2588190451 },
		{-0.9396926208 , -0.3420201433 },
		{-0.9063077870 , -0.4226182617 },
		{-0.8660254038 , -0.5000000000 },
		{-0.8191520443 , -0.5735764364 },
		{-0.7660444431 , -0.6427876097 },
		{-0.7071067812 , -0.7071067812 },
		{-0.6427876097 , -0.7660444431 },
		{-0.5735764364 , -0.8191520443 },
		{-0.5000000000 , -0.8660254038 },
		{-0.4226182617 , -0.9063077870 },
		{-0.3420201433 , -0.9396926208 },
		{-0.2588190451 , -0.9659258263 },
		{-0.1736481777 , -0.9848077530 },
		{-0.0871557427 , -0.9961946981 },
		{0.0000000000 , -1.0000000000 },
		{0.0871557427 , -0.9961946981 },
		{0.1736481777 , -0.9848077530 },
		{0.2588190451 , -0.9659258263 },
		{0.3420201433 , -0.9396926208 },
		{0.4226182617 , -0.9063077870 },
		{0.5000000000 , -0.8660254038 },
		{0.5735764364 , -0.8191520443 },
		{0.6427876097 , -0.7660444431 },
		{0.7071067812 , -0.7071067812 },
		{0.7660444431 , -0.6427876097 },
		{0.8191520443 , -0.5735764364 },
		{0.8660254038 , -0.5000000000 },
		{0.9063077870 , -0.4226182617 },
		{0.9396926208 , -0.3420201433 },
		{0.9659258263 , -0.2588190451 },
		{0.9848077530 , -0.1736481777 },
		{0.9961946981 , -0.0871557427 }
};
bool DualMotors::RobotHasToChangeDirection(int x,int y,t_map_position * pathToFollow) {
	float distanceAlreadyDone = 0;
	float distanceToReachNextPoint = 0;
	t_map_position * TargetPoint;

	if(pathToFollow==NULL) return true;
	distanceAlreadyDone = distance(pathToFollow->coord.x, pathToFollow->coord.y,x,y);

	TargetPoint=pathToFollow->p_next;
	if(TargetPoint==NULL) return true;

	distanceToReachNextPoint = distance(pathToFollow->coord.x, pathToFollow->coord.y,TargetPoint->coord.x,TargetPoint->coord.y);

	//printf("RobotPos [%d,%d,%d] distanceToReachNextPoint: %f => distanceAlreadyDone %f\n",x,y,pos.angle,distanceToReachNextPoint,distanceAlreadyDone);

	return(distanceAlreadyDone>=distanceToReachNextPoint);

}
bool DualMotors::updatePosition(){
	bool stopRequested=false;
	// check if we're moving or not
	double seconds;
	float distance=0;
	float x,y;
	int currentAngle;
	if(state!=DualMotors::STOPPED && state!=DualMotors::ROTATION_ON_GOING){
		SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
		seconds = elapsedTime(pos.timing);
		updateCurrentTime(&pos);
		float rps= (float)((float)(WHEELS_CIRC *current_speed)/100);
		distance = (float)(seconds * rps);
		distance *= (state==DualMotors::STARTED_FORWARD ? 1 : -1);
		int index = pos.angle/5;
		x = (float)(pos.x + (float)(distance*tab_cos[index][0]));
		y = (float)(pos.y + (float)(distance*tab_cos[index][1]));
		pos.x=x;
		pos.y=y;
		currentAngle=pos.angle;
		/*printf("[delta: %f][distance : %f][angle: %d][x=%f][y=%f]\n",
			 seconds
			,distance
			,pos.angle
			,pos.x,pos.y);*/
		SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
		bool informUser=false;
		if(pathToFollow!=NULL){
			 if(RobotHasToChangeDirection(x,y,pathToFollow)){
				pathToFollow=pathToFollow->p_next;
				if(pathToFollow){
					if(pathToFollow->angleToNextPoint!=-9999) {
						int angle=(360-((-1)*pathToFollow->angleToNextPoint))%360;
						turn(angle-currentAngle);
					}
					else {
						informUser=true;
					}
				}
			}
		}
		if(fct_PathCB && informUser){
			fct_PathCB(coockiesCB);
			// callback valide only one time
			fct_PathCB=NULL;
			coockiesCB=NULL;
		}
	}
    stopRequested=pos.stopRequested;

	return stopRequested;
}


extern "C" void * my_position_thread (void * arg)
{
  ((DualMotors*)arg)->PositionLoop();
  pthread_exit(0);
  return(0);
}

int DualMotors::StartPositionUpdate(bool updateTiming){

	if(updateTiming) {
		SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
		updateCurrentTime(&pos);
		SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
	}
	if(PositionUpdateOnGoing) return 0;
	PositionUpdateOnGoing=true;
	SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
	pos.stopRequested=false;
	SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
	if (pthread_create (&thId, NULL, my_position_thread, (void*)this) < 0) {
				RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"pthread_create error for thread PositionCheck");
				exit (1);
	}
	return 0;
}

int DualMotors::StopPositionUpdate(){
	void *status;
	if(!PositionUpdateOnGoing) return 0;
	SEM_WAIT(SERVICE_DUALMOTOR,&updatePositionProtect);
	pos.stopRequested=true;
	SEM_POST(SERVICE_DUALMOTOR,&updatePositionProtect);
	if(pthread_join ((unsigned long int)thId, &status)){
				RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"!!!ERROR in joining");
	}
	PositionUpdateOnGoing=false;
	return 0;
}

void DualMotors::updateCurrentTime(t_dualmotors_position * pos){
	  clock_gettime(CLOCK_REALTIME, &(pos->timing));
}

double DualMotors::elapsedTime(struct timespec begin) {
  struct timespec end;
  double elapsed_secs=0;
  clock_gettime(CLOCK_REALTIME, &end);

  elapsed_secs = (end.tv_sec - begin.tv_sec) + (double)(end.tv_nsec - begin.tv_nsec) / (double)(1000000000);

  return elapsed_secs;
}

int DualMotors::followThisPath(t_map_position * path,t_motor_callback_fct pFctCB,void* coockies) {
	if(!path) {
		RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"No path");
		return -1;
	}
	fct_PathCB=pFctCB;
	coockiesCB = coockies;
	if(state!=DualMotors::STOPPED) {
		RPI_TRACE_ERROR(SERVICE_DUALMOTOR,"Robot is already moving : impossible to set the path");
		return -1;
	}
	pathToFollow = path;
    if(pathToFollow){
    	//int angle=(int)computeAngleForNextStep(pos.x,pos.y,pathToFollow->x,pathToFollow->y);

    	if(path->angleToNextPoint!=-9999) {
    		int angle=(360-((-1)*pathToFollow->angleToNextPoint))%360;
    		turn(angle-pos.angle);
    	}
    	move_forward();
    }
    return 0;
}

