<?php

require_once "socket.php";

// Utilisation d'un header pour spécifier le type de contenu de la page. Ici, il s'agit juste de texte brut (text/plain). 
header("Content-Type: text/xml"); 
$module='';
$service='';
$param1='';
$param2='';

$module=(isset($_GET["module"])) ? $_GET["module"] : "";
$service = (isset($_GET["service"])) ? $_GET["service"] : "";
$param1 = (isset($_GET["param1"])) ? $_GET["param1"] : "";
$param2 = (isset($_GET["param2"])) ? $_GET["param2"] : "";
if($module) {
	echo send_command("$module $service $param1 $param2");
}
else {
	echo send_command("$service $param1 $param2");
	print_r($service);
}
?>
