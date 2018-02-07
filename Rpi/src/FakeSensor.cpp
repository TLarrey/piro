/*
 * FakeSensor.cpp
 *
 *  Created on: 05-Dec-2013
 *      Author: TLarrey
 */

#include "FakeSensor.h"

namespace std {

void BmpMap::displayMap(void){
	FILE *fp=fopen("./result.txt","wb");
	int i,j;
	bool gotoLine=false;
	if (fp == NULL) {
			RPI_TRACE_ERROR(SERVICE_BMPMAP,"Error opening Map snapshot");
			exit(1);
	}
	for(i=0;i<width;i++) {
		for(j=0;j<height;j++){
			if(myMap[i][j]>0) {
				fprintf(fp,"[%d][%d] ",i,j);
				gotoLine=true;
			}
		}
		if(gotoLine) {
			fprintf(fp,"\n");
		}
		gotoLine=false;
	}
	fclose(fp);
}

BmpMap::BmpMap() {
	RPI_TRACE_API(SERVICE_BMPMAP,">%s",__func__);
	FILE *fp;
	fp = fopen(MAP_FILE, "rb");
	if (fp == NULL) {
		RPI_TRACE_ERROR(SERVICE_BMPMAP,"Error opening Map source file");
		exit(1);
	}
	myMap=NULL;

	if(parseBMPFile(fp,myMap)>0) {
		RPI_TRACE_ERROR(SERVICE_BMPMAP,"Error parsing Map source file");
		exit(2);
	}
	//displayMap();
	RPI_TRACE_DEBUG(SERVICE_BMPMAP,"Map found (%ldx%ld)and parsed",width,height);
	RPI_TRACE_API(SERVICE_BMPMAP,"<%s",__func__);
}

BmpMap::~BmpMap() {
	RPI_TRACE_API(SERVICE_BMPMAP,">%s",__func__);
	// TODO Auto-generated destructor stub
	freeMap(myMap,height,width);
	RPI_TRACE_DEBUG(SERVICE_BMPMAP,"Map closed ; memory freed\n");
	RPI_TRACE_API(SERVICE_BMPMAP,"<%s",__func__);
}

// Check if at the distance from [x;y] there is an obstacle
// angle in degree
int BmpMap::calculatePos(int angle,t_map_position * current_pos, int distance,t_map_position * next_pos){

	int index=angle/=10;
	int boundary=FREE;
	next_pos->coord.x = (float)(current_pos->coord.x + (float)(distance*tab_cos_sin[index][0]));
	next_pos->coord.y = (float)(current_pos->coord.y + (float)(distance*tab_cos_sin[index][1]));
	if(next_pos->coord.x < 0) { next_pos->coord.x=0; boundary=WALL; }
	if(next_pos->coord.y < 0) { next_pos->coord.y=0; boundary=WALL; }
	if(next_pos->coord.x >= width) { next_pos->coord.x=(width-1); boundary=WALL; }
	if(next_pos->coord.y >= height) { next_pos->coord.y=(height-1); boundary=WALL; }
#if 0
	RPI_TRACE_INFO(SERVICE_BMPMAP,"(%dx%d)[%d;%d] + %d => [%d;%d] (boundary : %d)\n",width,height,
			current_pos->coord.x,
			current_pos->coord.y,
			distance,
			next_pos->coord.x,
			next_pos->coord.y,
			boundary);
#endif

	return boundary;
}
// Check if from [x;y] up to the distance there is an obstacle
int BmpMap::isThereObstacle(int angle,t_map_position * current_pos, int distance,t_map_position * obstacle_pos){

	int i=0;
	for(i=0;i<distance;i++){
		if(calculatePos(angle,current_pos,i,obstacle_pos)==WALL){
		//	RPI_TRACE_DEBUG(SERVICE_BMPMAP,"angle : %d, distance : %d => Obstacle detected in [%d,%d]",angle, i, obstacle_pos->coord.x,obstacle_pos->coord.y);
			return i;
		}
		if(myMap[obstacle_pos->coord.x][obstacle_pos->coord.y]==WALL) {
		//	RPI_TRACE_DEBUG(SERVICE_BMPMAP,"angle : %d, distance : %d => Obstacle detected in [%d,%d]",angle, i, obstacle_pos->coord.x,obstacle_pos->coord.y);
			return i;
		}
	}

	return 0;
}

int BmpMap::parseBMPFile(FILE *fp,int ** map){
	RPI_TRACE_API(SERVICE_BMPMAP,">%s",__func__);
	/*
	 * Read the first two bytes as little-endian to determine the file type.
	 * Preserve the file position.
	 */

	RGB **argbs=NULL;
    char **xorMasks, **andMasks;
    UINT32 *heights, *widths;
    int row, col;
    UINT16 fileType;
    long filePos;
    int numImages, i;
    int rc;

	filePos = ftell(fp);
	rc = endianConvert.readUINT16little(fp, &fileType);
	if (rc != 0)
	{
		perror("Error getting file type");
		RPI_TRACE_API(SERVICE_BMPMAP,"<%s",__func__);
		return 3;
	}
	fseek(fp, filePos, SEEK_SET);

	/*
	 * Read the images.
	 */
	switch (fileType) {
	case TYPE_ARRAY:
	/*
	 * If this is an array of images, read them.  All the arrays we need
	 * will be allocated by the reader function.
	 */
	rc = bmp.readMultipleImage(fp, &argbs, &xorMasks, &andMasks, &heights,
				   &widths, &numImages);
	break;
	case TYPE_BMP:
	case TYPE_ICO:
	case TYPE_ICO_COLOR:
	case TYPE_PTR:
	case TYPE_PTR_COLOR:
	/*
	 * If this is a single-image file, we've a little more work.  In order
	 * to make the output part of this test program easy to write, we're
	 * going to allocate dummy arrays that represent what
	 * readMultipleImage would have allocated.  We'll read the data into
	 * those arrays.
	 */
	argbs = (RGB **)calloc(1, sizeof(RGB *));
	if (argbs == NULL)
	{
		rc = 1005;
		break;
	}
	xorMasks = (char **)calloc(1, sizeof(char *));
	if (xorMasks == NULL)
	{
		free(argbs);
		rc = 1005;
		break;
	}
	andMasks = (char **)calloc(1, sizeof(char *));
	if (andMasks == NULL)
	{
		free(argbs);
		free(xorMasks);
		rc = 1005;
		break;
	}
	heights = (UINT32 *)calloc(1, sizeof(UINT32));
	if (heights == NULL)
	{
		free(argbs);
		free(xorMasks);
		free(andMasks);
		rc = 1005;
		break;
	}
	widths = (UINT32 *)calloc(1, sizeof(UINT32));
	if (widths == NULL)
	{
		free(argbs);
		free(xorMasks);
		free(andMasks);
		free(heights);
		rc = 1005;
		break;
	}
	numImages = 1;

	/*
	 * Now that we have our arrays allocted, read the image into them.
	 */
	switch (fileType) {
	case TYPE_BMP:
		rc = bmp.readSingleImageBMP(fp, argbs, widths, heights);
		break;
	case TYPE_ICO:
	case TYPE_PTR:
		rc = bmp.readSingleImageICOPTR(fp, xorMasks, andMasks, widths,
					   heights);
		break;
	case TYPE_ICO_COLOR:
	case TYPE_PTR_COLOR:
		rc = bmp.readSingleImageColorICOPTR(fp, argbs, xorMasks, andMasks,
						widths, heights);
		break;
	}
	break;
	default:
		rc = 1000;
		break;
	}

	/*
	 * At this point, everything's been read.  Display status messages based
	 * on the return values.
	 */
	switch (rc) {
	case 1000:
	case 1006:
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"File is not a valid bitmap file");
	break;
	case 1001:
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"Illegal information in an image");
	break;
	case 1002:
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"Legal information that I can't handle yet in an image");
	break;
	case 1003:
	case 1004:
	case 1005:
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"Ran out of memory");
	break;
	case 0:
	RPI_TRACE_DEBUG(SERVICE_BMPMAP,"Got good data from file, writing results");
	break;
	default:
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"Error reading file rc=%d", rc);
	RPI_TRACE_ERROR(SERVICE_BMPMAP,"Errno:");
	break;
	}

	/*
	 * If the return value wasn't 0, something went wrong.
	 */
	if (rc != 0)
	{
	if (rc != 1000 && rc != 1005)
	{
		for (i=0; i<numImages; i++)
		{
		if (argbs[i] != NULL)
			free(argbs[i]);
		if (andMasks[i] != NULL)
			free(andMasks[i]);
		if (xorMasks[i] != NULL)
			free(xorMasks[i]);
		}
		free(argbs);
		free(andMasks);
		free(xorMasks);
		free(widths);
		free(heights);
	}
	RPI_TRACE_API(SERVICE_BMPMAP,"<%s",__func__);
	return rc;
	}
	fclose(fp);
	height=heights[0];
	width=widths[0];
	myMap=allocateMap(width,height);
	if (argbs[0] != NULL)
	{
		for (row = 0; row < height; row++)
		{
			for (col = 0; col < width; col++)
			{
				if(	argbs[0][row * width + col].red == 0xff &&
					argbs[0][row * width + col].green == 0xFF &&
					argbs[0][row * width + col].blue == 0xFF ){
					myMap[col][row]=FREE;
				}
				else {
					myMap[col][row]=WALL;
				}
			}
		}
	}
	/*
	 * Dumping is complete.  Free all the arrays and quit
	 */
	for (i=0; i<numImages; i++)
	{
	if (argbs[0] != NULL)
		free(argbs[0]);
	if (andMasks[0] != NULL)
		free(andMasks[0]);
	if (xorMasks[0] != NULL)
		free(xorMasks[0]);
	}
	free(argbs);
	free(andMasks);
	free(xorMasks);
	free(widths);
	free(heights);
	RPI_TRACE_API(SERVICE_BMPMAP,"<%s",__func__);

	return 0;
}

FakeSensor::FakeSensor(){
	RPI_TRACE_API(SERVICE_FAKE,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_FAKE,"[FakeSensor 0x%x] created ",this);
	RPI_TRACE_API(SERVICE_FAKE,"<%s",__func__);
}

FakeSensor::~FakeSensor(){
	RPI_TRACE_API(SERVICE_FAKE,">%s",__func__);
	RPI_TRACE_DEBUG(SERVICE_FAKE,"[FakeSensor 0x%x] deleted ",this);
	RPI_TRACE_API(SERVICE_FAKE,"<%s",__func__);

}

int FakeSensor::makeUniqueMeasure(int pAngleServo,t_sensor_result * results){
	RPI_TRACE_API(SERVICE_FAKE,">%s",__func__);
	int angleServo = pAngleServo;
	int angleRobot = 0;
	int angle=0;
	t_map_position obstacle_pos;
	t_map_position MapPosition;
    t_dualmotors_position RobotPosition;

    if(angleServo<0) {
    	angleServo=myServo->getAngle();
    }
    myMotors->get_position(&RobotPosition);
    angleRobot = RobotPosition.angle;
	MapPosition.coord.x = (int)RobotPosition.x;
	MapPosition.coord.y = (int)RobotPosition.y;
	angle = (angleRobot-90+angleServo)%360;
	if(angle<0) angle+=360;
	results->AvgDistance=RefMap.isThereObstacle(angle,&MapPosition,100,&obstacle_pos);

	for(int i=0;i<NB_ULTRASOUND_SAMPLES_PER_MEASURE;i++){
		results->tabDistance[i]=results->AvgDistance;
	}
	results->coord.x=obstacle_pos.coord.x;
	results->coord.y=obstacle_pos.coord.y;
	pushResult(results);

	if(!info.cmdStop) {
		if(callbackMeasure && info.period && results->AvgDistance>0) {
			if(callbackMeasure(results,coockies)<0){
				// if callback returns -1, it means no more need of the callback
				callbackMeasure=NULL;
			}
		}
	}

	RPI_TRACE_API(SERVICE_FAKE,"<%s",__func__);
	return results->AvgDistance;
}

int FakeSensor::get_sync_results_sensor(t_sensor_result * results,int angleServo) {
	t_sensor_state currentState;
	RPI_TRACE_API(SERVICE_FAKE,">%s",__func__);
	if(!results) return -1;

	SEM_WAIT(SERVICE_ULTRASOUND,&infoProtect);
	currentState = info.state;
	SEM_POST(SERVICE_ULTRASOUND,&infoProtect);

	if(currentState==IDLE){
		makeUniqueMeasure(angleServo,results);
	}
	RPI_TRACE_API(SERVICE_FAKE,"<%s",__func__);
	return 0;
}

int FakeSensor::get_results_sensor(t_sensor_result * results) {
	RPI_TRACE_API(SERVICE_FAKE,">%s",__func__);
	get_sync_results_sensor(results,-1);
	RPI_TRACE_API(SERVICE_FAKE,"<%s",__func__);
	return 0;
}


} /* namespace std */
