var xpos_robot=0;
var ypos_robot=0;
var previous_x_robot=0;
var previous_y_robot=0;
var previous_known=0;
var angleServoUpdated=0;

var arrow = [
    [ 2, 0 ],
    [ -5, -4 ],
    [ -5, 4]
];

var wheel = [
    [ 2, 12 ],
    [ 2, -12 ],
    [ -2, -12],
    [ -2, 12]
];

function drawRadar(centerX,centerY,angleRobot,angleServo)
{
    RobotContext.fillStyle = "#C66B1A";
    RobotContext.strokeStyle = "#C66B1A";    
    RobotContext.lineWidth = 2;

    RobotContext.beginPath();
    RobotContext.arc(centerX, centerY, 3, 0, 2 * Math.PI, false);
    RobotContext.fill();
    RobotContext.stroke();

    //pos=getAngle(centerX,centerY,angleRobot,20);
    angleServoUpdated=(parseInt(angleRobot)-90+parseInt(angleServo))%360;
   // RobotContext.fillText('AngleServoUpdated :'+angleServoUpdated,10,400);
    //drawLine(pos.x,pos.y,angleServoUpdated);
    drawLine(centerX,centerY,angleServoUpdated);    
}


function drawWheels(centerX,centerY,angle)
{
    RobotContext.fillStyle = "#733F0F";
    RobotContext.strokeStyle = "#733F0F";    
    RobotContext.lineWidth = 1;
    var angle_wheel_left = (parseInt(angle)+90)%360;

    pos_wheel_left=getAngle(centerX, centerY,angle_wheel_left,10);
    RobotContext.moveTo(pos_wheel_left.x,pos_wheel_left.y);
    drawFilledPolygon(RobotContext,translateShape(rotateShape(wheel,angle_wheel_left),pos_wheel_left.x, pos_wheel_left.y));	

    var angle_wheel_right = (parseInt(angle)-90)%360;
    pos_wheel_right=getAngle(centerX, centerY, angle_wheel_right,10);
    RobotContext.moveTo(pos_wheel_right.x,pos_wheel_right.y);
    drawFilledPolygon(RobotContext,translateShape(rotateShape(wheel,angle_wheel_right),pos_wheel_right.x, pos_wheel_right.y));	
}

function drawLine(x,y,angle) {
    var centerX = x ;
    var centerY = y ;
    RobotContext.beginPath();
    RobotContext.moveTo(centerX,centerY);
    pos=getAngle(centerX, centerY,angle,20);
    RobotContext.lineTo(pos.x, pos.y);
    RobotContext.stroke();
}

function drawLineArrow(x,y,angle) {
    var centerX = x ;
    var centerY = y ;
    RobotContext.beginPath();
    RobotContext.moveTo(centerX,centerY);
    pos=getAngle(centerX, centerY,angle,20);
    RobotContext.lineTo(pos.x, pos.y);
    RobotContext.stroke();

    drawFilledPolygon(RobotContext,translateShape(rotateShape(arrow,angle),pos.x, pos.y));
    RobotContext.moveTo(centerX,centerY);
};

function displayRobot(x,y,angle) {
    var radius = 10;
    var centerX = parseInt(x) ;
    var centerY = parseInt(y) ;
    RobotContext.beginPath();
    RobotContext.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
    RobotContext.fillStyle = "#537762";
    RobotContext.lineWidth = 1;
    RobotContext.strokeStyle = "#537762";
    RobotContext.fill();


    RobotContext.strokeStyle = "#733F0F"; 
    RobotContext.arc(centerX, centerY, radius/2, 0, 2 * Math.PI, false);
    RobotContext.lineWidth = 2;

    RobotContext.stroke();
    RobotContext.strokeStyle = "#537762";
    RobotContext.fillStyle = "#537762";
    RobotContext.lineWidth = 1;


    drawLineArrow(centerX, centerY,angle);
    drawWheels(centerX,centerY,angle);
    if(ServoState) {
    	drawRadar(centerX,centerY,angle,ServoAngle);
    }
    if(AutoMoveState!=0) {
    	displayRobotTrace(centerX,centerY);
    }
//    if(SensorState!=0 && SensorResult!=0) {
//	pos=getAngle(xpos_robot,ypos_robot,angleServoUpdated,SensorResult);
//	displayWall(pos.x,pos.y,angleServoUpdated);	
//    }
}

function CenterRobot(){
   xpos_robot = myRobotcanvas.width/2;
   ypos_robot = myRobotcanvas.height/2;
}

function displayTarget(){
    var centerX = parseInt(RobotTargetPosX) ;
    var centerY = parseInt(RobotTargetPosY) ;    
    RobotContext.beginPath();
    RobotContext.lineWidth = 1;
    RobotContext.fillStyle = "red";
    RobotContext.moveTo(centerX,centerY);
    RobotContext.lineTo(centerX-3, centerY+3);
    RobotContext.moveTo(centerX,centerY);
    RobotContext.lineTo(centerX-3, centerY-3);
    RobotContext.moveTo(centerX,centerY);
    RobotContext.lineTo(centerX+3, centerY+3);
    RobotContext.moveTo(centerX,centerY);
    RobotContext.lineTo(centerX+3, centerY-3);
    RobotContext.fill();
    RobotContext.stroke();
    
}

function displayLockedTarget(){
    if(isLockedTarget==1){
	var centerX = parseInt(TargetLockX) ;
	var centerY = parseInt(TargetLockY) ;    
	RobotContext.beginPath();
	RobotContext.lineWidth = 1;
	RobotContext.fillStyle = "red";
	RobotContext.moveTo(centerX,centerY);
	RobotContext.lineTo(centerX-3, centerY+3);
	RobotContext.moveTo(centerX,centerY);
	RobotContext.lineTo(centerX-3, centerY-3);
	RobotContext.moveTo(centerX,centerY);
	RobotContext.lineTo(centerX+3, centerY+3);
	RobotContext.moveTo(centerX,centerY);
	RobotContext.lineTo(centerX+3, centerY-3);
	RobotContext.fill();
	RobotContext.arc(centerX, centerY, 2, 0, 2 * Math.PI, false);
	RobotContext.fillStyle = "blue";
	RobotContext.lineWidth = 1;
	RobotContext.fill();
	RobotContext.stroke();
    }
}

function refreshRobotCanvas(){
	RobotContext.clearRect(0, 0, myRobotcanvas.width, myRobotcanvas.height);

	if(previous_known) {
		xpos_robot += DualMotorPosX - previous_x_robot;
		if(xpos_robot > myRobotcanvas.width) {
			xpos_robot = 0;
		}
		else if(xpos_robot < 0 ) {
			xpos_robot = myRobotcanvas.width;
		}
		ypos_robot += DualMotorPosY - previous_y_robot;
		if(ypos_robot > myRobotcanvas.height) {
			ypos_robot = 0;
		}
		else if(ypos_robot < 0 ) {
			ypos_robot = myRobotcanvas.height;
		}
	}
	else {
		xpos_robot = myRobotcanvas.width/2;
		ypos_robot = myRobotcanvas.height/2;
	}
	previous_x_robot = DualMotorPosX;
	previous_y_robot = DualMotorPosY;
	previous_known=true;	
	displayRobot(DualMotorPosX,DualMotorPosY,DualMotorAngle);	 
	if(isRobotTracked!=0) {
		displayTarget();
	}
	displayLockedTarget();
}


