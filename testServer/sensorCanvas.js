
var wall = [
    [ 1, 1 ],
    [ 1, -1 ],
    [ -1, -1],
    [ -1, 1]
];

function displayWall(x,y,angle) {
    var centerX = x ;
    var centerY = y ;
    EnvContext.fillStyle = 'red';
    EnvContext.beginPath();
    EnvContext.moveTo(centerX,centerY);
    EnvContext.stroke();
    drawFilledPolygon(EnvContext,translateShape(rotateShape(wall,angle),centerX, centerY));	
}

function displayObstacle(x,y) {
    var centerX = x ;
    var centerY = y ;
    var radius = 1;
    EnvContext.fillStyle = 'red';
    EnvContext.beginPath();
    EnvContext.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
    EnvContext.stroke();
}

function displayRobotTrace(x,y) {
    var centerX = x ;
    var centerY = y ;
    var radius = 0.5;
    TraceRobotContext.fillStyle = 'red';
    TraceRobotContext.strokeStyle = 'red';
    TraceRobotContext.beginPath();
    TraceRobotContext.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
    TraceRobotContext.stroke();	
}

function refreshSensorCanvas(){
	//EnvContext.clearRect(0, 0, myEnvcanvas.width, myEnvcanvas.height);
	//if(SensorStateId) {
	//	pos=getAngle(xpos_robot,ypos_robot,DualMotorAngle,SensorResult);
	//	displayWall(pos.x,pos.y,DualMotorAngle);	
	//}
}

