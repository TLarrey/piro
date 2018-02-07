
function refreshDebugCanvas(){
	DebugContext.clearRect(0, 0, myDebugcanvas.width, myDebugcanvas.height);
	DebugContext.fillStyle = 'blue';
	DebugContext.fillText('Robot status :',5,10);
	DebugContext.fillText('State : '+ RobotState,20,20);
	DebugContext.fillText('Speed : '+ RobotSpeed+' %',20,30);
	DebugContext.fillText('Direction : '+ DualMotorAngle+' degrees',20,40);
	DebugContext.fillText('Position : [ '+ DualMotorPosX + ' ; ' + DualMotorPosY + ' ]', 20,50);
	DebugContext.fillText('Sensor status :',5,60);
	if(SensorState!="0") {
//		DebugContext.fillText('last measure : '+ SensorResult +' cm',20,70);
		DebugContext.fillText('State : activated',20,70);
	}
	else {
		DebugContext.fillText('State : deactivated',20,70);
	}
	DebugContext.fillText('Servo status :',5,80);
	if(ServoState!="0") {
		DebugContext.fillText('last position : '+ ServoAngle +' degree',20,90);
	}
	else {
		DebugContext.fillText('State : deactivated',20,90);
	}
	DebugContext.fillText('Automatic mode : ',5,100);
	DebugContext.fillText('State : '+ AutoMoveState, 20,110);
	DebugContext.fillText('Next target : [ '+ RobotTargetPosX + ' ; ' + RobotTargetPosY + ' ]', 20,120);
	if(isLockedTarget==1){
		DebugContext.fillText('Target LOCKED : [ '+ TargetLockX + ' ; ' + TargetLockY + ' ]', 20,130);
	}
	else {
		DebugContext.fillText('Target NOT Locked', 20,130);
	}
	if(AutoMoveState=="running") {
		DebugContext.fillText('Algo state : '+ AutoMoveAlgoState, 20,140);
	}	
}

