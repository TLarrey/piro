/*
 * debug.h
 *
 *  Created on: 22-Oct-2013
 *      Author: TLarrey
 */

#ifndef DEBUG_H_
#define DEBUG_H_
#include <sys/time.h>

#define SERVICE_MAP  		"   MAP"
#define SERVICE_BMPMAP		"BMPMAP"
#define SERVICE_FAKE 		"FKSENS"
#define SERVICE_MOTOR 		" MOTOR"
#define SERVICE_DUALMOTOR 	" DUALM"
#define SERVICE_MAIN 		"  MAIN"
#define SERVICE_SENSOR 		"SENSOR"
#define SERVICE_ULTRASOUND 	"USOUND"
#define SERVICE_SERVO 		" SERVO"

//#define LOG_DEBUG 1
//#define LOG_THREAD_ID 1
//#define LOG_API 1
//#define SEMAPHORE_DEBUG 1
//#define TIMESTAMP_DEBUG 1

#define EMULATE_SENSOR 1

char * GetTimeStamp();

#if defined LOG_THREAD_ID
#include <pthread.h>
#if defined TIMESTAMP_DEBUG
#define RPI_TRACE(serv, type, fmt, args...) printf("%s [0x%x]\t%s/%s :\t" fmt "\n", GetTimeStamp(), pthread_self(), serv, type,  ## args);
#else
#define RPI_TRACE(serv, type, fmt, args...) printf("[0x%x]\t%s/%s :\t" fmt "\n", pthread_self(), serv, type,  ## args);
#endif
#else
#if defined TIMESTAMP_DEBUG
#define RPI_TRACE(serv, type, fmt, args...) printf("%s %s/%s :\t" fmt "\n", GetTimeStamp(), serv, type,  ## args);
#else
#define RPI_TRACE(serv, type, fmt, args...) printf("%s/%s :\t" fmt "\n", serv, type,  ## args);
#endif
#endif

#define RPI_TRACE_ERROR(serv,fmt, args...)    RPI_TRACE(serv,"E",fmt, ## args)
#define RPI_TRACE_WARNING(serv,fmt, args...)    RPI_TRACE(serv,"W",fmt, ## args)
#define RPI_TRACE_INFO(serv,fmt, args...)    RPI_TRACE(serv,"I",fmt, ## args)
#if defined LOG_DEBUG
#define RPI_TRACE_DEBUG(serv, fmt, args...)  RPI_TRACE(serv,"D",fmt, ## args)
#else
#define RPI_TRACE_DEBUG(serv, fmt, args...)
#endif
#if defined LOG_API
#define RPI_TRACE_API(serv, fmt, args...)  RPI_TRACE(serv,"A",fmt, ## args)
#else
#define RPI_TRACE_API(serv, fmt, args...)
#endif


#if defined SEMAPHORE_DEBUG
#define SEM_INIT(serv,sem,init1,init2) { do { RPI_TRACE(serv,"S","semaphore 0x%x init", sem);sem_init(sem,init1,init2);}while(0);}
#define SEM_WAIT(serv,sem) { do { RPI_TRACE(serv,"S","sem_wait(0x%x)", sem);sem_wait(sem);}while(0);}
#define SEM_POST(serv,sem) { do { RPI_TRACE(serv,"S","sem_post(0x%x)", sem);sem_post(sem);}while(0);}
#else
#define SEM_INIT(serv,sem,init1,init2) { sem_init(sem,init1,init2);}
#define SEM_WAIT(serv,sem) { sem_wait(sem);}
#define SEM_POST(serv,sem) { sem_post(sem);}
#endif
#endif /* DEBUG_H_ */
