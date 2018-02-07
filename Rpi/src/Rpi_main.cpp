//============================================================================
// Name        : Rpi.cpp
// Author      : SDA
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "Rpi_main.h"


using namespace std;

#define RPI_DEBUG_MSG_EXCH 1

extern struct timeval initTime;
void* SocketHandler(void*);
void* AutoMoveThread(void*);
typedef struct {
	RPI_Robot *pRobot;
	int* csock;
}t_ThreadData;

int start_server(){
	int host_port= 2001;
	struct sockaddr_in my_addr;
	int hsock=-1;
	int * p_int ;
	socklen_t addr_size = 0;
	sockaddr_in sadr;
	pthread_t thread_id=0;
	RPI_Robot myRobot;
	t_ThreadData * pData=NULL;

retry_socket:
	if(pData){
		free(pData->csock);
		free(pData);
		pData=NULL;
	}
	if(hsock>=0) {
		close(hsock);
		shutdown(hsock,2);
		sleep(1);
	}
	RPI_TRACE_INFO(SERVICE_MAIN,"Server is starting");
	addr_size = 0;
	thread_id=0;
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		RPI_TRACE_ERROR(SERVICE_MAIN,"Error initializing socket %d\n", errno);
		return -1;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;

	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
	(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		RPI_TRACE_ERROR(SERVICE_MAIN,"Error setting options %d\n", errno);
		free(p_int);
		return -1;
	}
	free(p_int);

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;

	if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		RPI_TRACE_ERROR(SERVICE_MAIN,"Error binding to socket, make sure nothing else is listening on this port %d\n",errno);
		return -1;
	}
	if(listen( hsock, 2) == -1 ){
		RPI_TRACE_ERROR(SERVICE_MAIN, "Error listening %d\n",errno);
		return -1;
	}

	//Now lets do the server stuff

	do{
		addr_size = sizeof(sockaddr_in);

#if defined RPI_DEBUG_MSG_EXCH
		RPI_TRACE_DEBUG(SERVICE_MAIN,"\n SERVER is waiting for a new connection");
#endif
		pData = (t_ThreadData*)malloc(sizeof(t_ThreadData));
		pData->pRobot=&myRobot;
		pData->csock= (int*)malloc(sizeof(int));
		if((*(pData->csock) = accept( hsock, (sockaddr*)&sadr, &addr_size)) < 0){
			RPI_TRACE_ERROR(SERVICE_MAIN,"Error accepting %d\n", errno);
			goto retry_socket;
		}

#if defined RPI_DEBUG_MSG_EXCH
		RPI_TRACE_DEBUG(SERVICE_MAIN,"==> connection from %s",inet_ntoa(sadr.sin_addr));
#endif
		if(pthread_create(&thread_id,0,&SocketHandler, (void*)pData )<0) {
			RPI_TRACE_ERROR(SERVICE_MAIN,"Impossible to create a thread %d\n", errno);
			goto retry_socket;
		}
		pthread_detach(thread_id);
	}while(!myRobot.switchOffRequired());

	close(hsock);
	shutdown(hsock,2);
	return 1;
}

void* SocketHandler(void* lp){
	t_ThreadData * pData = (t_ThreadData *)lp;
	int *csock = pData->csock;
	RPI_Robot * pRobot = pData->pRobot;

	char buffer[MAX_COMMAND_STRING_LENGTH];
	int buffer_len = MAX_COMMAND_STRING_LENGTH;
	t_RPI_result result;
	int bytecount;
    memset(&result,0,sizeof(t_RPI_result));
	memset(buffer, 0, buffer_len);
	if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
		RPI_TRACE_ERROR(SERVICE_MAIN,"Error receiving data %d", errno);
		goto FINISH;
	}
#if defined RPI_DEBUG_MSG_EXCH
	RPI_TRACE_INFO(SERVICE_MAIN,"Command received: [%s]", buffer);
#endif
	/////////////////////////////////////////////////
	// Do treatment according to received data
	/////////////////////////////////////////////////

	pRobot->checkCommand(buffer,&result);
	formatResultInXML(&result);
	/////////////////////////////////////////////////
    if((bytecount = send(*csock, result.msg, strlen(result.msg), 0))== -1){
    	RPI_TRACE_ERROR(SERVICE_MAIN,"Error sending data %d", errno);
		goto FINISH;
	}
#if defined RPI_DEBUG_MSG_EXCH
    RPI_TRACE_DEBUG(SERVICE_MAIN,"Sent [%dbytes ] : %s", bytecount,result.msg);
#endif
FINISH:
	close(*csock);
	free(csock);
	free(pData);
	return 0;
}


int main() {
    gettimeofday(&initTime,0);
	start_server();
	return 0;
}


RPI_Robot::RPI_Robot():
		myMap(NULL){
	t_dualmotors_position MotorPos;
	switchOffRequested=0;
	t_map_position startPosition;
	int map_width;
	int map_height;

	RPI_TRACE_INFO(SERVICE_MAIN,"Robot is ready to run!!!");

	SEM_INIT(SERVICE_MAIN,&quitInfo,1,0);
	SEM_INIT(SERVICE_MAIN,&AutoMoveInfoProtect,1,1);

#if defined EMULATE_SENSOR
	mySensor.setServo(&myServo);
	mySensor.setMotors(&myMotors);
	mySensor.getMapInfo(&map_width,&map_height);
#else
	map_width=1024;
	map_height=1024;
#endif
	myMotors.set_initial_position(map_width>>1,map_height>>1);
	startPosition.coord.x=map_width>>1;
	startPosition.coord.y=map_height>>1;
	myMotors.get_position(&MotorPos);
	SEM_INIT(SERVICE_MAIN,&autoMoveInfo.AutoMoveSync,1,0);
	autoMoveInfo.AutoMoveThId=0;
	setAutoMoveStatus(AUTO_MODE_DISABLED);
	// So path is null and target is not reached
	autoMoveInfo.path=NULL;
	setEndOfPathStatus(false);
	setAutoMoveStopRequested(false);
	// Create a map to store obstacles
	myMap = new virtualMap(map_width,map_height);
	myMap->setStartPosition(&startPosition);
	myMap->setGoalPosition(&startPosition);
	autoMoveInfo.isLocked=false;
	autoMoveInfo.stateAlgo=AM_ALGO_COMPUTE_PATH;
}

int RPI_Robot::checkCommand(char* command,t_RPI_result *result){
	int i=0;
	char lowerCommand[MAX_COMMAND_STRING_LENGTH];
	result->status=-1;
	strncpy(result->msg,"",MAX_RESULT_STRING_LENGTH);
	char * p=lowerCommand;

	strncpy(lowerCommand,command,MAX_COMMAND_STRING_LENGTH);
    while ((*p = tolower( *p ))!=0) p++;

	// First thing to do : check if user want to close
	if(strncmp(lowerCommand,ConvertServ2String[RPI_SERV_QUIT],strlen(ConvertServ2String[RPI_SERV_QUIT]))==0){
		RPI_TRACE_INFO(SERVICE_MAIN,"Request to switch off the server has been received");
		switchOffRequested=1;
	}
	// Keep it here to free main loop
	SEM_POST(SERVICE_MAIN,&quitInfo);

	t_RPI_command * pSplittedCmd=(t_RPI_command *)malloc(sizeof(t_RPI_command));
    if(split_string(lowerCommand,pSplittedCmd)) {
    	for(i=0;i<RPI_SERV_NB_SERVICES;i++){
    		if(strcmp(pSplittedCmd->service,ConvertServ2String[i])==0){
    			break;
    		}
    	}
    	switch(i) {
    	  case RPI_SERV_ROBOT :
    		  treatCommand(pSplittedCmd,result);
    		  break;
    	  case RPI_SERV_DUAL_MOTOR :
    		  myMotors.treatCommand(pSplittedCmd,result);
    		  break;
    	  case RPI_SERV_ULTRASOUND :
			  mySensor.treatCommand(pSplittedCmd,result);
    		  break;
    	  case RPI_SERV_SERVO :
    		  myServo.treatCommand(pSplittedCmd,result);
    		  break;
    	  case RPI_SERV_QUIT :
    		  result->status=0;
    		  strncpy(result->msg,"Server stopped",MAX_RESULT_STRING_LENGTH);
    		  break;
    	  case RPI_SERV_CHECK :
    		  result->status=0;
    		  strncpy(result->msg,"OK",MAX_RESULT_STRING_LENGTH);
    		  break;
    	  default:
    		RPI_TRACE_ERROR(SERVICE_MAIN,"Unknown service requested");
    		strncpy(result->msg,"Unknown service requested",MAX_RESULT_STRING_LENGTH);
    		return 0;
    	}
    }
	return 1;
}

void* AutoMoveThread(void* lp){
	RPI_Robot * Robot = (RPI_Robot *)lp;
	t_RPI_Robot_measure measures;
	RPI_TRACE_DEBUG(SERVICE_MAIN,"THREAD AUTOMOVE CREATED\n");
	Robot->setAutoMoveStopRequested(false);
	Robot->setAutoMoveStatus(AUTO_MODE_ENABLED_RUNNING);
	Robot->AutoMove(&measures);
	RPI_TRACE_DEBUG(SERVICE_MAIN,"THREAD AUTOMOVE ENDED\n");
	return 0;
}

int  RPI_Robot::treatCommand(t_RPI_command* command,t_RPI_result * result){
	int i=0;
	result->status=0;
	for(i=0;i<CMD_ROBOT_NB_CMD;i++){
	  if(strcmp(command->param[0],ConvertCmd2String[i])==0){
			break;
	  }
	}
	switch(i) {
	  case CMD_ROBOT_INIT :
		  strncpy(result->msg,"Robot initialization Done",MAX_RESULT_STRING_LENGTH);
		  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
		  break;
	  case CMD_ROBOT_RESET :
		  strncpy(result->msg,"Robot reset Done",MAX_RESULT_STRING_LENGTH);
		  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
	  case CMD_ROBOT_AUTO_START :
		  result->status=startAutoMove();
		  if(result->status<0){
				strncpy(result->msg,"Robot : Impossible to switch to automatic mode",MAX_RESULT_STRING_LENGTH);
		  }
		  else {
			  strncpy(result->msg,"Robot : switch to automatic mode",MAX_RESULT_STRING_LENGTH);
		  }
		  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
		  break;
	  case CMD_ROBOT_AUTO_SET_GOAL:
		  result->status=updateAutoMoveGoal(atoi(command->param[1]),atoi(command->param[2]));
		  strncpy(result->msg,(result->status<0 ? "Goal not set":"New Goal set"),MAX_RESULT_STRING_LENGTH);
		  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
		  break;
	  case CMD_ROBOT_AUTO_STOP :
		  stopAutoMove();
		  strncpy(result->msg,"Robot switch to remote control mode",MAX_RESULT_STRING_LENGTH);
		  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
		  break;
	  case CMD_ROBOT_GET_STATUS:
		  t_dualmotors_status motor_status;
		  Servo::t_servoInfo servo_status;
		  int SensorStatus;
		  t_sensor_result sensor_results;

		  myMotors.get_status(&motor_status);
		  if(getAutoMoveStatus()==AUTO_MODE_DISABLED) {
			  mySensor.get_status(&SensorStatus);
			  //Sensor results are retrieved independently through Sensor service
			  myServo.get_status(&servo_status);
		  }
		  else {
			  SensorStatus=Sensor::STARTED;
			  mySensor.get_results(&sensor_results);
			  servo_status.currentAngle=myServo.getAngle();
			  servo_status.state=Servo::Moving;
		  }
		  sprintf(result->msg,"<robot_status><dualmotors><state>%d</state><speed>%d</speed><x>%d</x><y>%d</y><angle>%d</angle></dualmotors><sensor><state>%d</state><res>%d</res></sensor><servo><state>%d</state><angle>%d</angle><speed>%d</speed></servo><automove><state>%s</state><algostate>%s</algostate><locked>%d</locked><lockX>%d</lockX><lockY>%d</lockY></automove></robot_status>",
				  motor_status.state,
				  (int)motor_status.current_speed,
				  (int)motor_status.pos.x,(int)motor_status.pos.y,(int)motor_status.pos.angle,
				  SensorStatus,0,
				  servo_status.state,(int)servo_status.currentAngle,(int)servo_status.speed,
				  AutoMode2Str[autoMoveInfo.state],AutoModeAlgo2Str[autoMoveInfo.stateAlgo],
				  (autoMoveInfo.isLocked?1:0),autoMoveInfo.goal.coord.x,autoMoveInfo.goal.coord.y );
		  //printf("%s\n",result->msg);
		  break;
		  /*
	  case CMD_ROBOT_GENERATE_VIRTUAL_MAP:
		  result->status=-1;
		  if(myMap) {
			  char buffer[20]="./virtualMap"; // keep space for extension
			  FILE * fp=NULL;
			  int filesize=0;
			  myMap->storeVirtualMap(buffer); // update the filename with the extension
			  fp=fopen(buffer,"rb");
			  if(fp){
				  fseek(fp,0,SEEK_END);
				  filesize=ftell(fp);
				  result->info[0]=filesize;
				  result->status=0;
				  strncpy(result->msg,buffer,MAX_RESULT_STRING_LENGTH);
			  }
		  }
		  if(result->status==-1){
		  	  strncpy(result->msg,"File not generated",MAX_RESULT_STRING_LENGTH);
		  	  result->info[0]=0;
		  }
	  	  RPI_TRACE_INFO(SERVICE_MAIN,"%s",result->msg);
		  break;
		  */
	  default:
		result->status=-1;
		strncpy(result->msg,"Unknown command",MAX_RESULT_STRING_LENGTH);
		RPI_TRACE_ERROR(SERVICE_MAIN,"%s",result->msg);
		return 0;
	}
  return 1;
}

int ServoUpdated_CB(int angle,void* cookies){
	RPI_Robot * rpi = (RPI_Robot *)cookies;
	t_map_position v_objectPosition;
	t_sensor_result results;
	(rpi->getSensor())->get_results(&results,angle);
	if(results.AvgDistance>0) {
		v_objectPosition.coord.x=results.coord.x;
		v_objectPosition.coord.y=results.coord.y;
		(rpi->getMap())->SetMapObstacle(&v_objectPosition,results.AvgDistance);
	}
	return 0;
}

int RPI_Robot::CheckCloseEnvironment(){
	int angle=0;
	int pos=0; // 0 mean forward else backward (robot made a UTurn)
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	void * handle=myServo.registerPositionSet(ServoUpdated_CB,this);
	for(angle=0;angle<360;angle++) {
		if(pos==0 && angle>180) {
			myMotors.turn(180);
			pos=1;
		}
		if(angle>180){
			myServo.setAngle(360-angle,Servo::Synchronous);
		}
		else
			myServo.setAngle(angle,Servo::Synchronous);
	}
	myServo.unregisterPositionSet(handle);
	if(pos==1) myMotors.turn(180);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
	return 0;
}

int RPI_Robot::CheckCloseEnvironment(int nbStep,int * angle){
	int index=0;
	int pos=0; // 0 mean forward else backward (robot made a UTurn)
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	void * handle=myServo.registerPositionSet(ServoUpdated_CB,this);
	for(index=0;index<nbStep;index++) {
		if(pos==0 && angle[index]>180) {
			myMotors.turn(180);
			pos=1;
		}
		if(angle[index]>180){
			myServo.setAngle(360-angle[index],Servo::Synchronous);
		}
		else
			myServo.setAngle(angle[index],Servo::Synchronous);
	}
	myServo.unregisterPositionSet(handle);
	if(pos==1) myMotors.turn(180);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
	return 0;
}

#define LIMIT_OBSTACLE 10
#define THRESHOLD_OBSTACLE 2



void RPI_Robot::wakeUpAutoMoveThread(){
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_POST(SERVICE_MAIN,&autoMoveInfo.AutoMoveSync);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}

void RPI_Robot::waitForNextEvent(){
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&autoMoveInfo.AutoMoveSync);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}
bool RPI_Robot::getEndOfPathStatus(){
	bool EndOfPath;
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	EndOfPath=autoMoveInfo.EndOfPathReached;
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
	return(EndOfPath);
}
void RPI_Robot::setEndOfPathStatus(bool status)
{
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	autoMoveInfo.EndOfPathReached=status;
	RPI_TRACE_DEBUG(SERVICE_MAIN,"[AutoMode End of path is %s]",(status==true?"reached":"not reached"));
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}

bool RPI_Robot::getAutoMoveStopRequested(){
	bool stopStatus;
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	stopStatus=autoMoveInfo.stopAutoModeEvent;
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
	return(stopStatus);
}
void RPI_Robot::setAutoMoveStopRequested(bool status)
{
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	if(autoMoveInfo.stopAutoModeEvent!=status){
		RPI_TRACE_DEBUG(SERVICE_MAIN,"[AutoMode stop request is %s]",(status==true?"true":"false"));
		autoMoveInfo.stopAutoModeEvent=status;
		if(status){
			wakeUpAutoMoveThread();
		}
	}
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}

e_RPI_robot_state RPI_Robot::getAutoMoveStatus(){
	e_RPI_robot_state Status;
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	Status=autoMoveInfo.state;
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
	return(Status);
}

void RPI_Robot::setAutoMoveStatus(e_RPI_robot_state status)
{
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_DEBUG(SERVICE_MAIN,"[AutoMode status is %s]",AutoMode2Str[status]);
	autoMoveInfo.state=status;
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}

void RPI_Robot::getTargetPosition(int *x, int *y){
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	*x=autoMoveInfo.goal.coord.x;
	*y=autoMoveInfo.goal.coord.y;
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
}

void RPI_Robot::setTargetPosition(int x, int y){
	SEM_WAIT(SERVICE_MAIN,&AutoMoveInfoProtect);
	autoMoveInfo.goal.coord.x=x;
	autoMoveInfo.goal.coord.y=y;
	if(myMap) {
		myMap->setGoalPosition(&autoMoveInfo.goal);
		RPI_TRACE_DEBUG(SERVICE_MAIN,"[Goal updated to [x:%d, y:%d] ]",x,y);
		autoMoveInfo.isLocked=true;
	}
	SEM_POST(SERVICE_MAIN,&AutoMoveInfoProtect);
}


int RPI_Robot::updateAutoMoveGoal(int x, int y){

	switch(getAutoMoveStatus()) {
		case AUTO_MODE_ENABLED:
			break;
		case AUTO_MODE_ENABLED_RUNNING:
			stopAutoMove();
			break;
		case AUTO_MODE_DISABLED:
			setTargetPosition(x,y);
			return 0;
			break;
		default:
			return -1;
			break;
	}
	setTargetPosition(x,y);
	return startAutoMove();
}

int RPI_Robot::startAutoMove(){
	t_dualmotors_position MotorPos;
	int x,y;
	switch(getAutoMoveStatus()) {
		case AUTO_MODE_ENABLED_RUNNING:
			return -1;
			break;
		case AUTO_MODE_ENABLED:
		case AUTO_MODE_DISABLED:
			myMotors.get_position(&MotorPos);
			setAutoMoveStatus(AUTO_MODE_ENABLED);
			getTargetPosition(&x,&y);
			if(	x == MotorPos.x &&
				y == MotorPos.y ) {
				RPI_TRACE_WARNING(SERVICE_MAIN,"Target is similar to starting point");
				return 0;
			}
			if(pthread_create(&autoMoveInfo.AutoMoveThId,0,&AutoMoveThread, (void*)this )<0) {
				RPI_TRACE_ERROR(SERVICE_MAIN,"Impossible to create a thread %d\n", errno);
				return -1;
			}
			break;
		default:
			return -1;
			break;
	}
	return 0;
}
int RPI_Robot::stopAutoMove(){
	void * thRet;
	printf("STOP REQUESTED current state is %s\n",AutoMode2Str[getAutoMoveStatus()]);
	switch(getAutoMoveStatus()) {
		case AUTO_MODE_DISABLED:
			return 0;
			break;
		case AUTO_MODE_ENABLED:
			break;
		case AUTO_MODE_ENABLED_RUNNING:
			setAutoMoveStopRequested(true);
			printf("WaitFor Thread Join\n");
			pthread_join(autoMoveInfo.AutoMoveThId,&thRet);
			printf("Thread Joined\n");
			autoMoveInfo.AutoMoveThId=0;
			break;
		default:
			return -1;
			break;
	}
	setAutoMoveStatus(AUTO_MODE_DISABLED);
	return 0;
}

int SensorAutoMoveCallback(t_sensor_result * results,void *coockies) {
	RPI_Robot * robot = (RPI_Robot *)coockies;
	t_map_position v_objectPosition;
	int res=-1;
	/*	bool EndOfPath;
	bool stopEventReceived;
    EndOfPath=robot->getEndOfPathStatus();
	stopEventReceived=robot->getAutoMoveStopRequested();
	if(EndOfPath || stopEventReceived) {
		robot->wakeUpAutoMoveThread();
		return -1; // to indicate that CB is nomore usefull
	}*/

	if(results->AvgDistance==0) return 0;
	// store the obstacle if not yet known
	v_objectPosition.coord.x=results->coord.x;
	v_objectPosition.coord.y=results->coord.y;
	res=(robot->getMap())->SetMapObstacle(&v_objectPosition,20);
	//if(results->AvgDistance > (LIMIT_OBSTACLE+2)) return 0;

	if(	res==1 /*||
		(res==0 && results->AvgDistance <= (LIMIT_OBSTACLE+2))*/ )
	{ 	// new obstacle so not yet taken into account
		// Or known obstacle but potentially not taken into account because
		// discover as too far during the move
		// in both cases, robot is too close to this obstacle so stop it
		robot->getMotor()->stop();
		// wake up the thread waiting for update
		robot->wakeUpAutoMoveThread();
	}
	return 1;
}

int MotorEndOfPathCallback(void * coockies){
	RPI_Robot * robot = (RPI_Robot *)coockies;
	printf("END OF THE PATH\n");
	robot->setEndOfPathStatus(true);
	robot->wakeUpAutoMoveThread();
	return 0;
}

int RPI_Robot::trySetStartPosition(){
	t_map_position robotPosition;
	t_dualmotors_position motorPosition;
	int count=10;
	do {
		if(count<10){
			//RPI_TRACE_WARNING(SERVICE_MAIN,"Incoherency detected between robot position and obstacle ; reverse then try again");
			myMotors.move_backward();
			usleep(300000);
			myMotors.stop();
		}
		myMotors.get_position(&motorPosition);
		robotPosition.coord.x=motorPosition.x;
		robotPosition.coord.y=motorPosition.y;
		count--;
	}while (!myMap->setStartPosition(&robotPosition) && count);
	if(count) return true;
	return false;
}

int RPI_Robot::AutoMove(t_RPI_Robot_measure * measures){
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	int nbBlocked=5;
	autoMoveInfo.stateAlgo=AM_ALGO_COMPUTE_PATH;
	autoMoveInfo.path=NULL;
	myMotors.stop();
	myServo.stopInfiniteMove();
	mySensor.stop();
    while(getAutoMoveStopRequested()==false) {
	  switch(autoMoveInfo.stateAlgo) {
		case AM_ALGO_COMPUTE_PATH : // Init
			// Reset everything
			RPI_TRACE_INFO(SERVICE_MAIN,"Try to find a new path");
			SEM_INIT(SERVICE_MAIN,&autoMoveInfo.AutoMoveSync,1,0);
			//CheckCloseEnvironment();
			if(trySetStartPosition()==false) {
				RPI_TRACE_WARNING(SERVICE_MAIN,"Impossible to set a coherent start position");
				autoMoveInfo.stateAlgo=AM_ALGO_BLOCKED;
				break;
			}
			myMap->deletePath(autoMoveInfo.path);
			autoMoveInfo.path=myMap->computePath();
			if(autoMoveInfo.path==NULL){
				RPI_TRACE_WARNING(SERVICE_MAIN,"Unable to compute path");
				autoMoveInfo.stateAlgo=AM_ALGO_BLOCKED;
				break;
			}
			RPI_TRACE_INFO(SERVICE_MAIN,"Path found...");
			setEndOfPathStatus(false);
			myMap->displayPath(autoMoveInfo.path);
			myServo.setAngle(90,Servo::Synchronous);
			if(!getAutoMoveStopRequested()){
				myMotors.followThisPath(autoMoveInfo.path,MotorEndOfPathCallback,this);
				mySensor.start(250,SensorAutoMoveCallback,this);
				RPI_TRACE_INFO(SERVICE_MAIN,"Progression on going...\n");
				autoMoveInfo.stateAlgo=AM_ALGO_RUNNING;
			}
			break;
		case AM_ALGO_RUNNING:
			nbBlocked=5;
			waitForNextEvent();
			mySensor.stop();
			if(getEndOfPathStatus()){
				myMotors.stop();
				autoMoveInfo.stateAlgo=AM_ALGO_TARGET_REACH;
			}
			else {
				autoMoveInfo.stateAlgo=AM_ALGO_COMPUTE_PATH;
			}
			break;
		case AM_ALGO_BLOCKED:
			if(nbBlocked){
				myMotors.move_backward();
				usleep(300000);
				myMotors.stop();
				autoMoveInfo.stateAlgo=AM_ALGO_COMPUTE_PATH;
				nbBlocked--;
			}
			else {
				RPI_TRACE_WARNING(SERVICE_MAIN,"Robot is unable to continue : BLOCKED");
				autoMoveInfo.stateAlgo=AM_ALGO_WAIT_AFTER_BLOCKED;
			}
			break;
		case AM_ALGO_TARGET_REACH:
			RPI_TRACE_WARNING(SERVICE_MAIN,"Robot has reached its target or has been stopped");
			autoMoveInfo.stateAlgo=AM_ALGO_WAIT_AFTER_TARGET;
			break;
		case AM_ALGO_WAIT_AFTER_TARGET:
		case AM_ALGO_WAIT_AFTER_BLOCKED:
			sleep(1);
			break;
		default:
			break;
		}
    }
    myMotors.stop();
	myServo.stopInfiniteMove();
	mySensor.stop();
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
    return 0;
}


int RPI_Robot::switchOffRequired() {
	SEM_WAIT(SERVICE_MAIN,&quitInfo);
	return(switchOffRequested);
}
void RPI_Robot::UnlockOnError() {
	RPI_TRACE_API(SERVICE_MAIN,"> %s",__func__);
	SEM_POST(SERVICE_MAIN,&quitInfo);
	RPI_TRACE_API(SERVICE_MAIN,"< %s",__func__);
}
