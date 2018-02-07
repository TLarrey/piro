var RobotState="is stopped"
var RobotSpeed=0

function DEPRECATED_dualMotor_get_state(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};    
    	xhr.open("GET", "generic.php?module=dualmotors&service=get_state", true);
    	xhr.send(null);
}

function DEPRECATED_dualMotor_get_State_CB(oData) {
	var statusMove="";
	if(decodeXMLResult(oData,info)<0) return;
	
	switch(info.info1) {
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
	RobotSpeed=info.info2;
	document.getElementById("DualMotorSpeedRge").value=info.info2;
	var list="";
	list += info.info2;
	list += "% of maximum speed";
	document.getElementById("speedJaugeId").innerHTML=list;

	DualMotorPosX=info.info3;
	DualMotorPosY=info.info4;
	DualMotorAngle=info.info5;
}

function DEPRECATED_DualMotorGetPosition(callback) {
	return;
	xhr_GetPosition=getXMLHttpRequest();
	xhr_GetPosition.onreadystatechange = function() {
        	if (xhr_GetPosition.readyState == 4 && (xhr_GetPosition.status == 200 || xhr_GetPosition.status == 0)) {
            		callback(xhr_GetPosition.responseXML);
        	}
    	};
    	xhr_GetPosition.open("GET", "generic.php?module=dualmotors&service=get_position", true);
    	xhr_GetPosition.send(null);
}

function DEPRECATED_CB_DualMotorGetPosition(oData) {
	
	if(decodeXMLResult(oData,info)<0) return;
	
	DualMotorPosX=info.info1;
	DualMotorPosY=info.info2;
	DualMotorAngle=info.info3;
}


function DualMotorSetSpeed(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
	var RangeVal = document.getElementById("DualMotorSpeedRge").value;
    	xhr.open("GET", "generic.php?module=dualmotors&service=set_speed&param1=" + RangeVal, true);
    	xhr.send(null);
}

function CB_DualMotorSetSpeed(oData) {
	
	if(decodeXMLResult(oData,info)<0) return;
	//if(trackingDualMotorId==0) {
	//   dualMotor_get_state(dualMotor_get_State_CB);
	//}
}

function DualMotorSetAngle(callback) {
	xhrSetAngle=getXMLHttpRequest();
	xhrSetAngle.onreadystatechange = function() {
        	if (xhrSetAngle.readyState == 4 && (xhrSetAngle.status == 200 || xhrSetAngle.status == 0)) {
            		callback(xhrSetAngle.responseXML);
        	}
    	};
	var RangeVal = document.getElementById("DualMotorAngleId").value;
    	xhrSetAngle.open("GET", "generic.php?module=dualmotors&service=turn&param1=" + RangeVal, true);
    	xhrSetAngle.send(null);
}

function DualMotorsUpdateAngleValue() {
	document.getElementById("DualMotorAngleValueId").innerHTML=document.getElementById("DualMotorAngleId").value;
}

function CB_DualMotorSetAngle(oData) {	
	if(decodeXMLResult(oData,info)<0) return;
	document.getElementById("DualMotorAngleId").value=0;
	document.getElementById("DualMotorAngleValueId").innerHTML=0;
}

function DualMotorMoveFwd(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
    	xhr.open("GET", "generic.php?module=dualmotors&service=move_forward", true);
    	xhr.send(null);
}

function DualMotorMoveBwd(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
    	xhr.open("GET", "generic.php?module=dualmotors&service=move_backward", true);
    	xhr.send(null);
}


function DualMotorMoveLeft(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
    	xhr.open("GET", "generic.php?module=dualmotors&service=turn&param1=350", true);
    	xhr.send(null);
}

function DualMotorMoveRight(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
    	xhr.open("GET", "generic.php?module=dualmotors&service=turn&param1=10", true);
    	xhr.send(null);
}

function DualMotorMoveStop(callback) {
	xhr=getXMLHttpRequest();
	xhr.onreadystatechange = function() {
        	if (xhr.readyState == 4 && (xhr.status == 200 || xhr.status == 0)) {
            		callback(xhr.responseXML);
        	}
    	};
    	xhr.open("GET", "generic.php?module=dualmotors&service=stop", true);
    	xhr.send(null);
}

function CB_DualMotorMove(oData) {
	if(decodeXMLResult(oData,info)<0) return;
	//if(trackingDualMotorId==0) {
	//   dualMotor_get_state(dualMotor_get_State_CB);
	//}
}

//var trackingDualMotorId=0;
//function trackDualMotor(){
//	if(trackingDualMotorId==0) {
//		document.getElementById("trackingId").style.visibility = 'visible';
//		trackingDualMotorId=setInterval('dualMotor_get_state(dualMotor_get_State_CB);', 250); //call test every 1 seconds.
//		document.getElementById("EnableDualTracking").value="Stop tracking";
//		AnimOnGoing=1;	animate();
//	}
//	else {
//		clearInterval(trackingDualMotorId);
//		document.getElementById("EnableDualTracking").value="Start tracking";
//		trackingDualMotorId=0;
//		AnimOnGoing=0;
//		resetCanvas(myDebugcanvas,DebugContext);
//	}
//}


