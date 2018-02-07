var ServoStateId=0;
var ServoAngle=0;
var ServoState=0;

function ServoRefreshDisplay(enable) {
	if(enable=="1"){
		document.getElementById("ServoEnableId").value="Stop Infinite Move";
	}
	else {
		document.getElementById("ServoEnableId").value="Start Infinite Move";
	}
}

function ServoEnableDisable()
{
	if(ServoState=="0") {
		Servo_UpdateState(CB_Servo_UpdateState,1);	
	}
	else {				
		Servo_UpdateState(CB_Servo_UpdateState,0);
	}
}

function Servo_UpdateState(callback,state) {
	xhr_UpdateServo=getXMLHttpRequest();
	xhr_UpdateServo.onreadystatechange = function() {
        	if (xhr_UpdateServo.readyState == 4 && (xhr_UpdateServo.status == 200 || xhr_UpdateServo.status == 0)) {
            		callback(xhr_UpdateServo.responseXML);
        	}
    	};
	if(state=="0") cmd="stop";
	else cmd="start";

    	xhr_UpdateServo.open("GET", "generic.php?module=servo&service="+cmd, true);
    	xhr_UpdateServo.send(null);

}

function CB_Servo_UpdateState(oData) {
	if(decodeXMLResult(oData,info)<0) return;	
}

function Servo_getAngle(callback) {
	xhr_Servo_getAngle=getXMLHttpRequest();
	xhr_Servo_getAngle.onreadystatechange = function() {
        	if (xhr_Servo_getAngle.readyState == 4 && (xhr_Servo_getAngle.status == 200 || xhr_Servo_getAngle.status == 0)) {
            		callback(xhr_Servo_getAngle.responseXML);
        	}
    	};

    	xhr_Servo_getAngle.open("GET", "generic.php?module=servo&service=get_angle", true);
    	xhr_Servo_getAngle.send(null);
}

function CB_Servo_getAngle(oData) {
	if(decodeXMLResult(oData,info)<0) return;
	ServoAngle=info.info1;
}



