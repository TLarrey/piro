var SensorResult=0;
var SensorState=0;


function UltraSoundRefreshDisplay(enable){
	if(enable=="1"){
		document.getElementById("EnableUltraSoundId").value="Stop sensor";
		document.getElementById("UltraSoundPeriodId").disabled=true;
	}
	else {
		document.getElementById("UltraSoundPeriodId").disabled=false;
		document.getElementById("EnableUltraSoundId").value="Start sensor";
	}
}


function UltraSoundControl(){
	var state=0;
	if(SensorState=="0") {		
		UltraSound_UpdateSensor(CB_UltraSound_UpdateSensor,1,document.getElementById("UltraSoundPeriodId").value);	
	}
	else {				
		UltraSound_UpdateSensor(CB_UltraSound_UpdateSensor,0,document.getElementById("UltraSoundPeriodId").value);		
	}
}

function UltraSound_UpdateSensor(callback,state,period) {
	xhr_UpdateSensor=getXMLHttpRequest();
	xhr_UpdateSensor.onreadystatechange = function() {
        	if (xhr_UpdateSensor.readyState == 4 && (xhr_UpdateSensor.status == 200 || xhr_UpdateSensor.status == 0)) {
            		callback(xhr_UpdateSensor.responseXML);
        	}
    	};
	if(state=="0") cmd="stop";
	else cmd="start";

    	xhr_UpdateSensor.open("GET", "generic.php?module=ultrasound&service="+cmd+"&param1="+period, true);
    	xhr_UpdateSensor.send(null);
}

function CB_UltraSound_UpdateSensor(oData) {
	if(decodeXMLResult(oData,info)<0) return;
	if(info.status==0){
		if(SensorState=="0") SensorState="1";
		else SensorState="0";
	}
	UltraSoundRefreshDisplay(SensorState);
}

function UltraSound_getResult(callback){

	xhr_getResult=getXMLHttpRequest();
	xhr_getResult.onreadystatechange = function() {
        	if (xhr_getResult.readyState == 4 && (xhr_getResult.status == 200 || xhr_getResult.status == 0)) {
            		callback(xhr_getResult.responseXML);
        	}
    	};
    	xhr_getResult.open("GET", "generic.php?module=ultrasound&service=get_results", true);
    	xhr_getResult.send(null);
}

function CB_UltraSound_getResult(oData) {
	if(decodeXMLSensorResult(oData,info)<0) return;
}
