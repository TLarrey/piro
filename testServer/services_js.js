var AnimOnGoing=0;
var DualMotorPosX=0;
var DualMotorPosY=0;
var DualMotorAngle=0;
var myDebugcanvas;
var DebugContext;
var myRobotcanvas;
var RobotContext;
var myEnvcanvas;
var EnvContext;
var myTraceRobotcanvas;
var TraceRobotContext;
var RobotTargetPosX;
var RobotTargetPosY;
var TargetLockY;
var TargetLockX;
var isLockedTarget;
var AutoMoveState;
var AutoMoveAlgoState;

function getXMLHttpRequest() {
	var xhr = null;
	
	if (window.XMLHttpRequest || window.ActiveXObject) {
		if (window.ActiveXObject) {
			try {
				xhr = new ActiveXObject("Msxml2.XMLHTTP");
			} catch(e) {
				xhr = new ActiveXObject("Microsoft.XMLHTTP");
			}
		} else {
			xhr = new XMLHttpRequest(); 
		}
	} else {
		alert("Votre navigateur ne supporte pas l'objet XMLHTTPRequest...");
		return null;
	}
	
	return xhr;
}

var info = {status: "0", msg: "", info1: "", info2: "", info3: "", info4: "", info5: ""};

function initInfo(info){
	info.status=-1;
	info.msg="Unknown error";
	info.info1="";
	info.info2="";
	info.info3="";
	info.info4="";
	info.info5="";
}



function decodeXMLResult(oData,info)  {
// <result>
//	<status> </status>
//	<msg> </msg>
//	<info1></info1>
//	<info2></info2>
//	<info3></info3>
//	<info4></info4>
//	<info5></info5>
// </result>
	var result = 0;
	initInfo(info);
	if(!oData) return -1;

	result = oData.getElementsByTagName("result");
	if(!result) {
		info.msg="No or invalid answer received";
		return -1;
	}
	info.status=oData.getElementsByTagName("status")[0].childNodes[0].nodeValue;
	info.msg=oData.getElementsByTagName("msg")[0].childNodes[0].nodeValue;
	if(info.status<0) {
		if(info.status==-2) {
			stopTracking();
			startServerTracking();
		}else {
			alert(info.msg);
		}
		return -1;
	}
	stopServerTracking();
	info.info1=oData.getElementsByTagName("info1")[0].childNodes[0].nodeValue;
	info.info2=oData.getElementsByTagName("info2")[0].childNodes[0].nodeValue;
	info.info3=oData.getElementsByTagName("info3")[0].childNodes[0].nodeValue;
	info.info4=oData.getElementsByTagName("info4")[0].childNodes[0].nodeValue;
	info.info5=oData.getElementsByTagName("info5")[0].childNodes[0].nodeValue;
	return 0;
}

var robotStatus = {  dualmotor_state: "0", dualmotor_speed: "0", dualmotor_x: "", dualmotor_y: "", dualmotor_angle: "", 
		     sensor_state: "0", sensor_res: "",
		     servo_state: "0", servo_angle: "", servo_speed: "",
		     am_state: "0", am_algostate:"", am_locked:"0", am_lockx:"0", am_locky:"0"  };

function initRobotStatus(robotStatus){
	robotStatus.dualmotor_state="0";
	robotStatus.dualmotor_speed="0";
	robotStatus.dualmotor_x="0";
	robotStatus.dualmotor_y="0";
	robotStatus.sensor_state="0";
	robotStatus.sensor_res="0";
	robotStatus.servo_state="0";
	robotStatus.servo_speed="0";
	robotStatus.am_state="0";
	robotStatus.am_algostate="0";
	robotStatus.am_locked="0";
	robotStatus.am_lockx="0";
	robotStatus.am_locky="0";
}

function decodeXMLRobotStatus(oData,robotStatus,info)  {
// <result>
//	<status> </status>
//	<msg>
//		<robot_status>
//			<dualmotors>
//				<state>%d</state>
//				<speed>%d</speed>
//				<x>%d</x>
//				<y>%d</y>
//				<angle>%d</angle>
//			</dualmotors>
//			<sensor>
//				<state>%d</state>
//				<res>%d</res>
//			</sensor>
//			<servo>
//				<state>%d</state>
//				<angle>%d</angle>
//				<speed>%d</speed>
//			</servo>
//			<automove>
//				<state>%d</state>
//				<algostate>%d</algostate>
//				<locked>%d</locked>
//				<lockX>%d</lockX>
//				<lockY>%d</lockY>
//			</automove>
//		</robot_status>
//	</msg>
//	<info1></info1>
//	<info2></info2>
//	<info3></info3>
//	<info4></info4>
//	<info5></info5>
// </result>
	var result = 0;
	initRobotStatus(robotStatus);
	initInfo(info);
	if(!oData) return -1;
	
	result = oData.getElementsByTagName("result");
	if(!result) {
		info.msg="No or invalid answer received";
		return -1;
	}
	info.status=oData.getElementsByTagName("status")[0].childNodes[0].nodeValue;
	info.msg="OK";
	if(info.status<0) {
		if(info.status==-2) {
			stopTracking();
			startServerTracking();
		}else {
			alert("Impossible to retrieve current status");
		}
		return -1;
	}
	stopServerTracking();
	info.info1=oData.getElementsByTagName("info1")[0].childNodes[0].nodeValue;
	info.info2=oData.getElementsByTagName("info2")[0].childNodes[0].nodeValue;
	info.info3=oData.getElementsByTagName("info3")[0].childNodes[0].nodeValue;
	info.info4=oData.getElementsByTagName("info4")[0].childNodes[0].nodeValue;
	info.info5=oData.getElementsByTagName("info5")[0].childNodes[0].nodeValue;
	
	var dualmotor=oData.getElementsByTagName("dualmotors");
	for(var i=0;i<dualmotor[0].children.length;i++){
		
		switch(dualmotor[0].childNodes[i].tagName) {
			case "state" : robotStatus.dualmotor_state=dualmotor[0].childNodes[i].textContent;break;
			case "speed" : robotStatus.dualmotor_speed=dualmotor[0].childNodes[i].textContent;break;
			case "x" : robotStatus.dualmotor_x=dualmotor[0].childNodes[i].textContent;break;
			case "y" : robotStatus.dualmotor_y=dualmotor[0].childNodes[i].textContent;break;
			case "angle" : robotStatus.dualmotor_angle=dualmotor[0].childNodes[i].textContent;break;
			default:;
		}
	}
	var sensor=oData.getElementsByTagName("sensor");
	for(var i=0;i<sensor[0].children.length;i++){
		switch(sensor[0].childNodes[i].tagName) {
			case "state" : robotStatus.sensor_state=sensor[0].childNodes[i].textContent;break;
			case "res" : robotStatus.sensor_res=sensor[0].childNodes[i].textContent;break;
			default:;
		}
	}
	var servo=oData.getElementsByTagName("servo");
	for(var i=0;i<servo[0].children.length;i++){
		switch(servo[0].childNodes[i].tagName) {
			case "state" : robotStatus.servo_state=servo[0].childNodes[i].textContent;break;
			case "speed" : robotStatus.servo_speed=servo[0].childNodes[i].textContent;break;
			case "angle" : robotStatus.servo_angle=servo[0].childNodes[i].textContent;break;
			default:;
		}
	}

	var automove=oData.getElementsByTagName("automove");
	for(var i=0;i<automove[0].children.length;i++){
		switch(automove[0].childNodes[i].tagName) {
			case "state" : robotStatus.am_state=automove[0].childNodes[i].textContent;break;
			case "algostate" : robotStatus.am_algostate=automove[0].childNodes[i].textContent;break;
			case "locked" : robotStatus.am_locked=automove[0].childNodes[i].textContent;break;
			case "lockX" : robotStatus.am_lockx=automove[0].childNodes[i].textContent;break;
			case "lockY" : robotStatus.am_locky=automove[0].childNodes[i].textContent;break;
			default:;
		}
	}
	return 0;

}

function decodeXMLSensorResult(oData,info)  {
// <result>
//	<status> </status>
//	<msg>
//		<res><x>%d</x><y>%d</y></res>
//		...
//		<res><x>%d</x><y>%d</y></res>
//	</msg>
//	<info1></info1>
//	<info2></info2>
//	<info3></info3>
//	<info4></info4>
//	<info5></info5>
// </result>
	var result = 0;
	initInfo(info);
	if(!oData) return -1;
	
	result = oData.getElementsByTagName("result");
	if(!result) {
		info.msg="No or invalid answer received";
		return -1;
	}
	info.status=oData.getElementsByTagName("status")[0].childNodes[0].nodeValue;
	info.msg="OK";
	if(info.status<0) {
		if(info.status==-2) {
			stopTracking();
			startServerTracking();
		}else {
			alert("Impossible to retrieve current status");
		}
		return -1;
	}
	stopServerTracking();
	info.info1=oData.getElementsByTagName("info1")[0].childNodes[0].nodeValue;
	info.info2=oData.getElementsByTagName("info2")[0].childNodes[0].nodeValue;
	info.info3=oData.getElementsByTagName("info3")[0].childNodes[0].nodeValue;
	info.info4=oData.getElementsByTagName("info4")[0].childNodes[0].nodeValue;
	info.info5=oData.getElementsByTagName("info5")[0].childNodes[0].nodeValue;
	var results=oData.getElementsByTagName("res");
	if(results){
		for(var i=0;i<results.length;i++){
			if(results[i].childNodes[0].tagName=="x"){
				var x=results[i].childNodes[0].textContent;
			}
			if(results[i].childNodes[1].tagName=="y"){
				var y=results[i].childNodes[1].textContent;
			}
			displayObstacle(x,y);		
		}
	}
	return 0;
}

function generic_shutdownServer(callback){
//	if(trackingDualMotorId) stopTracking();
	xhr1=getXMLHttpRequest();
	xhr1.onreadystatechange = function() {
        	if (xhr1.readyState == 4 && (xhr1.status == 200 || xhr1.status == 0)) {
            		callback(xhr1.responseXML);
        	}
    	};  
	xhr1.open("GET", "generic.php?service=quit", true);
    	xhr1.send(null);  
}

function CB_shutdownServer(oData){
	if(decodeXMLResult(oData,info)<0) return;
	startServerTracking();
	alert(info.msg);
}



function AutoMoveRefreshDisplay(){
	if(AutoMoveState!="disabled") {
		document.getElementById("AutoMoveId").value="Stop auto mode";
	}
	else {
		document.getElementById("AutoMoveId").value="Start auto mode";
		document.getElementById('EnableUltraSoundId').disabled = false;
		document.getElementById('ServoEnableId').disabled = false;
	}
}

function CB_AutoMove(oData) {
	if(decodeXMLResult(oData,info)<0) return;
}

function AutoMoveMode() {
	if(AutoMoveState=="disabled") {
		cmd="start_auto";
	}
	else {
		cmd="stop_auto";
	}
	xhr_AutoMove=getXMLHttpRequest();
	xhr_AutoMove.onreadystatechange = function() {
		if (xhr_AutoMove.readyState == 4 && (xhr_AutoMove.status == 200 || xhr_AutoMove.status == 0)) {
	    		CB_AutoMove(xhr_AutoMove.responseXML);
		}
	};  
	xhr_AutoMove.open("GET", "generic.php?module=robot&service="+cmd, true);
	xhr_AutoMove.send(null);  
}


function AutoMoveSetTarget(callback) {
	xhr_AMST=getXMLHttpRequest();
	xhr_AMST.onreadystatechange = function() {
        	if (xhr_AMST.readyState == 4 && (xhr_AMST.status == 200 || xhr_AMST.status == 0)) {
            		callback(xhr_AMST.responseXML);
        	}
    	};
    	xhr_AMST.open("GET", "generic.php?module=robot&service=set_goal&param1="+RobotTargetPosX+"&param2="+RobotTargetPosY, true);
    	xhr_AMST.send(null);
}

function CB_AutoMoveSetTarget(oData) {
	if(decodeXMLResult(oData,info)<0) return;
	resetCanvas(myTraceRobotcanvas,TraceRobotContext);
}

var isRobotTracked=0;
function RobotTracking() {
	if(isRobotTracked==0) {
		document.getElementById("trackingId").style.visibility = 'visible';
		isRobotTracked=setInterval('robot_get_state(CB_robot_get_state);', 250); //call test every 1 seconds.
		document.getElementById("EnableDualTracking").value="Stop tracking";
		AnimOnGoing=1;	animate();
	}
	else {
		clearInterval(isRobotTracked);
		document.getElementById("EnableDualTracking").value="Start tracking";
		isRobotTracked=0;
		AnimOnGoing=0;
		resetCanvas(myDebugcanvas,DebugContext);
	}
}

function stopTracking() {
	if(isRobotTracked)
		RobotTracking();
}

function robot_get_state(callback) {
	xhr_RGS=getXMLHttpRequest();
	xhr_RGS.onreadystatechange = function() {
        	if (xhr_RGS.readyState == 4 && (xhr_RGS.status == 200 || xhr_RGS.status == 0)) {
            		callback(xhr_RGS.responseXML);
        	}
    	};    
    	xhr_RGS.open("GET", "generic.php?module=robot&service=get_status", true);
    	xhr_RGS.send(null);
}

function CB_robot_get_state(oData) {
	var statusMove="";
	if(decodeXMLRobotStatus(oData,robotStatus,info)<0) return;

	switch(robotStatus.dualmotor_state) {
	    case "0" : //STOPPED=0
		RobotState="is stopped";
		break;
	    case "1" : //STARTED_FORWARD
		RobotState="is moving forward";
		break;
	    case "2" : //STARTED_BACKWARD
		RobotState="is moving backward";
		break;
	    case "3" : //ROTATION_ON_GOING
		RobotState="is rotating";
		break;
	    default:;
	}
	RobotSpeed=robotStatus.dualmotor_speed;
	document.getElementById("DualMotorSpeedRge").value=RobotSpeed;
	var list="";
	list += RobotSpeed;
	list += "% of maximum speed";
	document.getElementById("speedJaugeId").innerHTML=list;

	DualMotorPosX=robotStatus.dualmotor_x;
	DualMotorPosY=robotStatus.dualmotor_y;
	DualMotorAngle=robotStatus.dualmotor_angle;
	ServoAngle=robotStatus.servo_angle;
	ServoState=robotStatus.servo_state;
	SensorState=robotStatus.sensor_state;

	isLockedTarget=robotStatus.am_locked;
	TargetLockY=robotStatus.am_locky;
	TargetLockX=robotStatus.am_lockx;
	AutoMoveState=robotStatus.am_state;
	AutoMoveAlgoState=robotStatus.am_algostate;
	
	UltraSoundRefreshDisplay(SensorState);
	ServoRefreshDisplay(ServoState);
	AutoMoveRefreshDisplay();

	if(SensorState=="1")
	  UltraSound_getResult(CB_UltraSound_getResult);
}



var serverTracking=0;
var serverAvailable=0;
function startServerTracking() {
	if(serverTracking!=0) return;
	updateFrameServerState(-1);	
	serverTracking=setInterval('checkServerAvailability(CB_checkServerAvailability);', 1000); //call test every 1 seconds.
}

function stopServerTracking() {
	if(serverTracking==0) return;
	clearInterval(serverTracking);
	serverTracking=0;
}

function ResetTracking() {
   resetCanvas(myDebugcanvas,DebugContext);
   resetCanvas(myRobotcanvas,RobotContext);
   resetCanvas(myEnvcanvas,EnvContext);
   resetCanvas(myTraceRobotcanvas,TraceRobotContext);
   CenterRobot();
}

function updateFrameServerState(status) {
	if(status >= 0) {		
		document.getElementById('serverStatusId').src="./images/greenStatus.png";
		document.getElementById('KillServerBtn').disabled = false;
		document.getElementById('EnableDualTracking').disabled = false;
		document.getElementById('ResetTracking').disabled = false;
		document.getElementById("AutoMoveId").disabled = false;
		document.getElementById("AutoMoveTargetId").disabled = false;
		document.getElementById("DualMotorCtrl").style.visibility = 'visible';
		document.getElementById("SensorCtrl").style.visibility = 'visible';
		document.getElementById("ServoCtrl").style.visibility = 'visible';
		//dualMotor_get_state(dualMotor_get_State_CB);
	}
	else {		
		document.getElementById('serverStatusId').src="./images/redStatus.png";
		document.getElementById('KillServerBtn').disabled = true;
		document.getElementById('EnableDualTracking').disabled = true;
		document.getElementById('ResetTracking').disabled = true;
		document.getElementById("AutoMoveId").disabled = true;
		document.getElementById("AutoMoveTargetId").disabled = true;
		document.getElementById("DualMotorSpeedRge").value=0;
		document.getElementById("DualMotorCtrl").style.visibility = 'hidden';
		document.getElementById("SensorCtrl").style.visibility = 'hidden';
		document.getElementById("ServoCtrl").style.visibility = 'hidden';
		document.getElementById("trackingId").style.visibility = 'hidden';
	}
}

function checkServerAvailability(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};  
	xhr.open("GET", "generic.php?service=check", true);
    	xhr.send(null);  
}
function CB_checkServerAvailability(oData) {
	if(decodeXMLResult(oData,info)<0) return;
	updateFrameServerState(info.status);
	serverAvailable=info.status;
	//DualMotorGetPosition(CB_DualMotorGetPosition);
}


function getAngle( x, y, angle, h) {
    var radians = angle * (Math.PI / 180);
    return { x: x + h * Math.cos(radians), y: y + h * Math.sin(radians) };
}

function drawFilledPolygon(context,shape) {
    context.beginPath();
    context.moveTo(shape[0][0],shape[0][1]);

    for(p in shape)
        if (p > 0) context.lineTo(shape[p][0],shape[p][1]);

    context.lineTo(shape[0][0],shape[0][1]);
    context.fill();
    RobotContext.stroke();
};

function translateShape(shape,x,y) {
    var rv = [];
    for(p in shape)
        rv.push([ shape[p][0] + x, shape[p][1] + y ]);
    return rv;
};

function rotateShape(shape,ang) {
    var rv = [];
    var radians = ang * (Math.PI / 180);
    for(p in shape)
        rv.push(rotatePoint(radians,shape[p][0],shape[p][1]));
    return rv;
};

function rotatePoint(ang,x,y) {
    return [
        (x * Math.cos(ang)) - (y * Math.sin(ang)),
        (x * Math.sin(ang)) + (y * Math.cos(ang))
    ];
};

function animate() {
        refreshRobotCanvas();
	refreshDebugCanvas();
	//refreshSensorCanvas();
	if(AnimOnGoing==1) {
        	requestAnimFrame(function() {
		animate();
		});
	}
}

function initAnimation() {
    animate();
}

function requestAnimFrame (callback) {
  setTimeout (callback, 1000/30);
}

function resetCanvas(myCanvas,myContext) {
	myContext.clearRect(0, 0, myCanvas.width, myCanvas.height);
}

function myMouseUp(evt){
    if(isRobotTracked!=0) {
	    var rect = myRobotcanvas.getBoundingClientRect();
	    RobotTargetPosX= evt.clientX - rect.left;
	    RobotTargetPosY= evt.clientY - rect.top;
   }
}

function initPage() {
   startServerTracking();
   myDebugcanvas = document.getElementById('layerDebug');
   DebugContext = myDebugcanvas.getContext('2d');
   resetCanvas(myDebugcanvas,DebugContext);
   myRobotcanvas = document.getElementById('layerRobot');
   RobotContext = myRobotcanvas.getContext('2d');
   myRobotcanvas.onmouseup = myMouseUp;
   resetCanvas(myRobotcanvas,RobotContext);
   myEnvcanvas = document.getElementById('layerEnv');
   EnvContext = myEnvcanvas.getContext('2d');
   resetCanvas(myEnvcanvas,EnvContext);
   myTraceRobotcanvas = document.getElementById('layerTraceRobot');
   TraceRobotContext = myTraceRobotcanvas.getContext('2d');
   resetCanvas(myTraceRobotcanvas,TraceRobotContext);
   AutoMoveState="disabled";
   previous_known=0;
   RobotTargetPosX=myRobotcanvas.width/2;
   RobotTargetPosY=myRobotcanvas.height/2;
}



