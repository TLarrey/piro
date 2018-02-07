/*
 * Sensor.cpp
 *
 *  Created on: 11-Oct-2013
 *      Author: TLarrey
 */
#include "stdio.h"
#include "stdlib.h"
#include "Sensor.h"
#include <unistd.h>
#include <string.h>
#include <semaphore.h>



extern "C" void * my_sensor_thread (void * arg)
{
   ((Sensor*)arg)->mainLoop();
   pthread_exit(0);
  return(0);
}

extern "C" void * sensor_measure (void * arg) {
	((Sensor*)arg)->makeMeasures();
	pthread_exit(0);
	return(0);
}

Sensor::Sensor() {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	// Mutex to protect reentrance
	SEM_INIT(SERVICE_SENSOR,&api_protect,1,1);
	SEM_INIT(SERVICE_SENSOR,&sem_thread_start,1,0);
	SEM_INIT(SERVICE_SENSOR,&sem_event_sent,1,0);
	SEM_INIT(SERVICE_SENSOR,&sem_event_ack,1,0);
	SEM_INIT(SERVICE_SENSOR,&syncStop,1,0);
	SEM_INIT(SERVICE_SENSOR,&infoProtect,1,1);
	SEM_INIT(SERVICE_SENSOR,&listMeasuresProtect,1,1);

	event=NB_CMD;
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"[Sensor 0x%x] construct Sensor",this);
	if (pthread_create (&thId, NULL, my_sensor_thread, (void*)this) < 0) {
			RPI_TRACE_ERROR(SERVICE_SENSOR,"pthread_create error for thread 2");
			exit (1);
	}

	strncpy(ConvertCmd2String[EXIT],"exit",20);
	strncpy(ConvertCmd2String[START_MEASURES],"start",20);
	strncpy(ConvertCmd2String[STOP_MEASURES],"stop",20);
	strncpy(ConvertCmd2String[GET_RESULTS],"get_results",20);

	callbackMeasure=NULL;
	coockies=NULL;
	memset(&info,0,sizeof(t_SensorInfo));
	info.state=IDLE;
	info.cmdStop=-1;
	listLastMeasures=NULL;
	// Wait for thread start
	SEM_WAIT(SERVICE_SENSOR,&sem_thread_start);
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"[Sensor 0x%x] created ",this);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

Sensor::~Sensor() {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	SEM_WAIT(SERVICE_SENSOR,&api_protect);
	void *status;

	deactivate_sensor();
	set_event(Sensor::EXIT);
	if(pthread_join ((unsigned long int)thId, &status)){
			RPI_TRACE_ERROR(SERVICE_SENSOR,"!!!ERROR in joining");
			exit(-1);
	}
	SEM_POST(SERVICE_SENSOR,&api_protect);
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"[Sensor 0x%x] deleted ",this);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

void Sensor::start(void){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	SEM_WAIT(SERVICE_SENSOR,&api_protect);
	set_event(Sensor::START_MEASURES);
	SEM_POST(SERVICE_SENSOR,&api_protect);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

void Sensor::stop(void){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	SEM_WAIT(SERVICE_SENSOR,&api_protect);
	set_event(Sensor::STOP_MEASURES);
	SEM_POST(SERVICE_SENSOR,&api_protect);
	SEM_WAIT(SERVICE_SENSOR,&syncStop);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

void Sensor::get_results(void){
	SEM_WAIT(SERVICE_SENSOR,&api_protect);
	set_event(Sensor::GET_RESULTS);
	SEM_POST(SERVICE_SENSOR,&api_protect);
}

int Sensor::set_event(t_sensor_cmd pEvent) {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	event=pEvent;
    SEM_POST(SERVICE_SENSOR,&sem_event_sent);
    // WAIT ack
	SEM_WAIT(SERVICE_SENSOR,&sem_event_ack);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
    return 1;
}

int Sensor::wait_event() {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
    // Lock mutex and then wait for signal to release mutex
	SEM_WAIT(SERVICE_SENSOR,&sem_event_sent);
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"[Sensor 0x%x] event %d received",this,event);
    // ACK treatment
	SEM_POST(SERVICE_SENSOR,&sem_event_ack);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
    return event;
}

void Sensor::mainLoop(){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	  t_sensor_cmd l_event;
	  // Sync to inform that thread is started
	  SEM_POST(SERVICE_SENSOR,&sem_thread_start);
	  // then infinite loop up to EXIT event
	  while((l_event=(t_sensor_cmd)wait_event())!=EXIT){
		switch(l_event) {
		  case START_MEASURES:
			  RPI_TRACE_DEBUG(SERVICE_SENSOR,"Start measures");
			  activate_sensor();
			  break;
		  case STOP_MEASURES:
			  RPI_TRACE_DEBUG(SERVICE_SENSOR,"Stop measures (state=%d)",info.state);
			  deactivate_sensor();
			  SEM_POST(SERVICE_SENSOR,&syncStop);
			  break;
		  case GET_RESULTS:
			  RPI_TRACE_DEBUG(SERVICE_SENSOR,"Get measures");
			  RPI_TRACE_ERROR(SERVICE_SENSOR,"NOT IMPLEMENTED");
			  break;
		  default:
			  RPI_TRACE_WARNING(SERVICE_SENSOR,"ERROR Unknown command");
			  break;
		}
	  }
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

int Sensor::treatCommand(t_RPI_command * command,t_RPI_result * result){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	//RPI_TRACE_DEBUG(SERVICE_SENSOR,"Command to treat : %s",command->param[0]);
	unsigned int nbWrittenBytes=0;
	FILE * fres=NULL;
	int i=0;
	char info[MAX_RESULT_STRING_LENGTH];
	result->status =0;
	strncpy(result->msg,"Done",MAX_RESULT_STRING_LENGTH);
	for(i=0;i<NB_CMD;i++){
	  if(strcmp(command->param[0],ConvertCmd2String[i])==0){
		  break;
	  }
	}
	switch(i) {
		case EXIT :	break;
		case START_MEASURES :
			int period;
			result->status = -1;
			if((period=atoi(command->param[1]))!=0) {
				result->status = 0;
				start(period,NULL,NULL);
			}
			if(result->status<0){
				strncpy(info,"Unable to start sensor",MAX_RESULT_STRING_LENGTH);
			}
			break;
		case STOP_MEASURES :
			result->status = 0;
			stop();
			break;
		case GET_RESULTS :
			// Results are those who have not been reported since last call of this service
			// It means that there is a limitation to the number of results at a time.
#if 0
			t_sensor_result results;
			result->status = get_results(&results);
			if(result->status<0){
				strncpy(info,"Unable to get results from sensor",MAX_RESULT_STRING_LENGTH);
			}
			else {
				result->info[0]=results.AvgDistance;
			}

#else

			nbWrittenBytes=0;
			remove("./resultSensor.xml");
			fres=fopen("./resultSensor.xml","wb");
			if(!fres){
				result->status = -1;
				strncpy(info,"Unable to create file",MAX_RESULT_STRING_LENGTH);
				break;
			}
			result->status = FormatResultsXML(fres,&nbWrittenBytes);
			if(result->status <0){
				strncpy(info,"Unable to format results from sensor",MAX_RESULT_STRING_LENGTH);
				fclose(fres);
				break;
			}
			if(nbWrittenBytes<MAX_RESULT_STRING_LENGTH ){
				fclose(fres);
				fres=fopen("./resultSensor.xml","rb");
				if(!fres){
					result->status = -1;
					strncpy(info,"Unable to open file",MAX_RESULT_STRING_LENGTH);
					break;
				}
				memset(result->msg,0,MAX_RESULT_STRING_LENGTH*sizeof(char));
				if(fread(result->msg,1,nbWrittenBytes,fres)!=nbWrittenBytes){
					result->status = -1;
					strncpy(info,"Unable to send results : file read error",MAX_RESULT_STRING_LENGTH);
					fclose(fres);
					break;
				}
			}
			else{
				result->status = -1;
				strncpy(info,"Unable to copy results : not enough space",MAX_RESULT_STRING_LENGTH);
				fclose(fres);
				break;
			}
			fclose(fres);
#endif
			break;
		default:
			result->status = -1;
			strncpy(info,"Unknown command",MAX_RESULT_STRING_LENGTH);
			break;
	}
	if(result->status<0){
		strncpy(result->msg,info,MAX_RESULT_STRING_LENGTH);
		RPI_TRACE_ERROR(SERVICE_SENSOR,"%s",info);
	}
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
	return 0;
}

void Sensor::start(int pPeriod,t_callback_fct cb,void * pCoockies) {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"[Sensor 0x%x]Sensor start",this);
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	info.period=pPeriod;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	callbackMeasure=cb;
	coockies=pCoockies;
	Sensor::start();
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

int Sensor::activate_sensor() {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"UltraSound::activate_sensor");
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	if(info.state!=IDLE) {
		SEM_POST(SERVICE_SENSOR,&infoProtect);
		RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
		return 0;
	}
	info.cmdStop=0;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	if (pthread_create (&USthId, NULL, sensor_measure, (void*)this) < 0) {
		RPI_TRACE_ERROR(SERVICE_SENSOR,"pthread_create error for thread 2");
		exit (1);
	}
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	info.state=STARTED;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
	return 0;
}

int Sensor::deactivate_sensor() {
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	void *status;
	RPI_TRACE_DEBUG(SERVICE_SENSOR,"UltraSound::deactivate_sensor");
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	if(info.state!=STARTED){
		SEM_POST(SERVICE_SENSOR,&infoProtect);
		RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
		return 0;
	}
	info.cmdStop=1;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	if(pthread_join ((unsigned long int)USthId, &status)){
			RPI_TRACE_ERROR(SERVICE_SENSOR,"!!!ERROR in joining");
			RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
			exit(1);
	}
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	info.state=IDLE;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
	return 0;
}


int Sensor::get_results(t_sensor_result * results,int angle){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	int res=0;
	if(angle<0)
		res=get_results_sensor(results);
	else {
		res=get_sync_results_sensor(results,angle);
	}
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
	return(res);
}

typedef struct s_XmlFile {
	FILE *fo;
}t_XmlFile;

int storeResult(t_sensor_result* result,void* coockies){
	char buffer[1024];
	t_XmlFile * info = (t_XmlFile*)coockies;
	sprintf(buffer,"<res><x>%d</x><y>%d</y></res>",result->coord.x,result->coord.y);
	fwrite(buffer,1,sizeof(char)*strlen(buffer),info->fo);
	return 0;
}

int Sensor::FormatResultsXML(FILE * fo,unsigned int * bytes){
	t_XmlFile infoXMLFile;
	if(!fo)
		return -1;
	infoXMLFile.fo=fo;
	flushResults(storeResult,(void*)&infoXMLFile);
	fseek(fo,0,SEEK_END);
	*bytes = ftell(fo);
	fseek(fo,0,SEEK_SET);
	return 0;
}

void Sensor::pushResult(t_sensor_result * results){
	t_sensor_result * list;
	t_sensor_result * previous=NULL;
	t_sensor_result * elt;
	unsigned int count=MAX_NB_RESULT_STORED;

	if(results->AvgDistance==0) {
		return;
	}
	elt = (t_sensor_result *)malloc(sizeof(t_sensor_result));
	if(elt==NULL) {
		RPI_TRACE_ERROR(SERVICE_SENSOR,"Impossible to store sensor results");
		return;
	}
	memcpy(elt,results,sizeof(t_sensor_result));
	elt->nextRes=NULL;

	SEM_WAIT(SERVICE_SENSOR,&listMeasuresProtect);
	list = listLastMeasures;
	while(list && count){
		if(		list->coord.x==elt->coord.x &&
				list->coord.y==elt->coord.y ){
			free(elt);
			SEM_POST(SERVICE_SENSOR,&listMeasuresProtect);
			return;
		}
		previous=list;
		list=previous->nextRes;
		count--;
	}
	if(!count) {
		RPI_TRACE_WARNING(SERVICE_SENSOR,"First sensor result lost");
		//Insert it a the beginning of the list
		list=listLastMeasures;
		elt->nextRes=listLastMeasures->nextRes;
		listLastMeasures=elt;
		free(list);
	}
	else {
		if(previous) previous->nextRes=elt;
		else listLastMeasures=elt;
	}
	SEM_POST(SERVICE_SENSOR,&listMeasuresProtect);
}

t_sensor_result * Sensor::getAllResults(){
	return listLastMeasures;
}

void Sensor::flushResults(t_callback_fct pCB,void * coockies){
	t_sensor_result * list;
	t_sensor_result * previous=NULL;
	SEM_WAIT(SERVICE_SENSOR,&listMeasuresProtect);
	list = listLastMeasures;

	while(list){
		previous=list;
		if(pCB){
			pCB(previous,coockies);
		}
		list=previous->nextRes;
		free(previous);
	}
	listLastMeasures=NULL;
	SEM_POST(SERVICE_SENSOR,&listMeasuresProtect);
}

void Sensor::makeMeasures(){
	RPI_TRACE_API(SERVICE_SENSOR,">%s",__func__);
	int stopReceived=0;
	unsigned long period = 0;
	SEM_WAIT(SERVICE_SENSOR,&infoProtect);
	stopReceived = info.cmdStop;
	period = info.period;
	if(!info.period) period=500*1000;
	SEM_POST(SERVICE_SENSOR,&infoProtect);
	// Prepare the driver
	while(stopReceived==0) {
		t_sensor_result results;
		makeUniqueMeasure(-1,&results);

		usleep(period);
		SEM_WAIT(SERVICE_SENSOR,&infoProtect);
		stopReceived = info.cmdStop;
		SEM_POST(SERVICE_SENSOR,&infoProtect);
	}
	RPI_TRACE_API(SERVICE_SENSOR,"<%s",__func__);
}

UltraSound::UltraSound() {
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	drv_state=UltraSound::disabled;
	RPI_TRACE_DEBUG(SERVICE_ULTRASOUND,"[UltraSound 0x%x] created ",this);
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
}

UltraSound::~UltraSound() {
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_ULTRASOUND,"[UltraSound 0x%x] deleted ",this);
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
}

int UltraSound::makeUniqueMeasure(int pAngleServo,t_sensor_result * results){
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	int sum=0;
	int res=-1;
	int stopReceived=0;

	if(drv_state==disabled){
		res=drv_ultrasound_prepare();
	}
	for(int i=0;i<NB_ULTRASOUND_SAMPLES_PER_MEASURE;i++){
		res=drv_ultrasound_measure(&(results->tabDistance[i]));
		sum+=results->tabDistance[i];
	}
	results->AvgDistance =sum/NB_ULTRASOUND_SAMPLES_PER_MEASURE;

	SEM_WAIT(SERVICE_ULTRASOUND,&infoProtect);
	stopReceived = info.cmdStop;
	SEM_POST(SERVICE_ULTRASOUND,&infoProtect);
	pushResult(results);

	if(stopReceived==0) {
		if(callbackMeasure && info.period && results->AvgDistance>0) {
			if(callbackMeasure(results,coockies)<0){
				// if callback returns -1, it means no more need of the callback
				callbackMeasure=NULL;
			}
		}
	}
	if(drv_state==enabled && stopReceived) drv_ultrasound_reset();
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
	return res;
}

int UltraSound::get_sync_results_sensor(t_sensor_result * results,int angleServo){
	return(get_results_sensor(results));
}

int UltraSound::get_results_sensor(t_sensor_result * results) {
	t_sensor_state currentState;
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	if(!results) return -1;

	SEM_WAIT(SERVICE_ULTRASOUND,&infoProtect);
	currentState = info.state;
	SEM_POST(SERVICE_ULTRASOUND,&infoProtect);

	if(currentState==IDLE && drv_state==UltraSound::disabled){
		makeUniqueMeasure(-1,results);
	}
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
	return 0;
}

int UltraSound::drv_ultrasound_prepare(){
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	drv_state=UltraSound::enabled;
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
	return 0;
}

int UltraSound::drv_ultrasound_reset(){
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	drv_state=UltraSound::disabled;
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
	return 0;
}

int UltraSound::drv_ultrasound_measure(int * value){
	RPI_TRACE_API(SERVICE_ULTRASOUND,">%s",__func__);
	// Do a measurement (in our test case random value)
	*value=100;
	RPI_TRACE_API(SERVICE_ULTRASOUND,"<%s",__func__);
	return 0;
}


