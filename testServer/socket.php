<?php

error_reporting(E_ALL);

function format_error($err_msg,$err) {
	$out='';
	$out="<result><status>".$err."</status><msg>".$err_msg."</msg><info1>0</info1><info2>0</info2><info3>0</info3><info4>0</info4><info5>0</info5></result>";
	return $out;
}


function send_command($param) {

	if($param=="") {
	    $err_msg="Please fill a new command";
	    return format_error($err_msg);	
	}

	$service_port = 2001;
	$address = gethostbyname('SartheForEver');
	print_r($address);
	$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
	if ($socket === false) {
	    $err_msg="socket_create() failed: \nReason: ". socket_strerror(socket_last_error());
	    return format_error($err_msg,-2);	
	}
	$result = socket_connect($socket, $address, $service_port);
	if ($result === false) {
	    $err_msg="socket_connect() failed.Reason: " . socket_strerror(socket_last_error($socket)) . "Is the Server started ?";
	    return format_error($err_msg,-2);	
	}

	$in = $param;
	socket_write($socket, $in, strlen($in));

	$out = '';
	$out = socket_read($socket, 2048);

	socket_close($socket);
	return $out;
}

function quit() {
	return send_command("quit");
} 

function generic($param) {
	return send_command($param);
} 


function read_file($param) {
	$service_port = 2001;
	$address = gethostbyname('xx.xx.xx.xx');

	$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
	if ($socket === false) {
	    $err_msg="socket_create() failed: \nReason: ". socket_strerror(socket_last_error());
	    return format_error($err_msg,-2);	
	}
	$result = socket_connect($socket, $address, $service_port);
	if ($result === false) {
	    $err_msg="socket_connect() failed.Reason: " . socket_strerror(socket_last_error($socket)) . "Is the Server started ?";
	    return format_error($err_msg,-2);	
	}
	$in = "file_transfer ".$param;

	socket_write($socket, $in, strlen($in));

	$file = ''; // <-- this is 2 apostrophies by the way
	$input = socket_read($client,2048);
	while(!feof($input)){
	  $file .= $input;
	  $input = socket_read($client,2048);
	}
	file_put_contents($param,$file);
	socket_close($socket);
}




?>
