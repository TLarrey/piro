<!DOCTYPE html>
<html>
<?php
error_reporting(E_ALL);
function send_command($param) {
	$service_port = 2001;
	$address = gethostbyname('xx.xx.xx.xx');

	$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
	if ($socket === false) {
	    echo "socket_create() failed: \nReason: " . socket_strerror(socket_last_error()) . "";	
	}
	$result = socket_connect($socket, $address, $service_port);
	if ($result === false) {
	    echo "socket_connect() failed.Reason: " . socket_strerror(socket_last_error($socket)) . "Is the Server started ?";
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

?>

    <head>
        <meta charset="utf-8" />
        <title>Titre</title>
    </head>

    <body>
      <form method="post" action="?btnquit" name="quitForm"">
	<input type="submit" name="btnquit" value="Quit" />
      </form>
      <form method="post" action="?btnSendCmd" name="genericForm"">
     	<input type="text" name="command" placeholder="enter a command" id="CommandLine"/>	       
	<input type="submit" name="btnSendCmd" value="Send Command " />	
      </form>
      <h1>Motors control :</h1>
      <?php 
//		$myresults=generic("dualmotors get_state"); 
//		list($forwardState, $backwardState, $SturnState, $stopState, $speed) = explode(";", $myresults);
//		echo "Forward : " .$forwardState."<br>";
//		echo "Backward : " .$backwardState."<br>";
//		echo "Stop : " .$stopState."<br>";
      ?>
      <form method="post" action="?actionMotorForm" name="actionMotor"">
     	Speed : <input type="range" name="speed" min="0" max="100" step="1" value="<?php echo $speed ?>"/><br>
	<input type="radio" name="actionMotorForm" value="move_forward" <?php if($forwardState=="1") echo "checked=\"checked\""; ?>/>Forward<br>
	<input type="radio" name="actionMotorForm" value="move_backward"<?php if($backwardState=="1") echo "checked=\"checked\""; ?>/>Backward<br>
	<input type="radio" name="actionMotorForm" value="turn" <?php if($SturnState=="1") echo "checked=\"checked\""; ?>>Turn : <input type="range" name="angle" min="0" max="360" step="1" value="0"/><br>
	<input type="radio" name="actionMotorForm" value="turn" <?php if($stopState=="1") echo "checked=\"checked\""; ?>>Stop<br>
      </form>
      <h1>Results :</h1>
      <textarea cols="40" rows="5" name="myresults">
<?php
	if(isset($_POST['btnquit']))
	{
	 echo quit(); 
	}
	else if(isset($_POST['btnSendCmd']))
	{
   	   if(isset($_POST['command']) && $_POST['command']!="")
	      echo generic($_POST['command']);
	   else 
	     echo "Please fill a command";
	}
	else if(isset($_POST['actionMotorForm']))
	{
	    echo "ok";
	}
?>
</textarea>

</body>

</html>


