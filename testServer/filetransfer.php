<?php

require_once "socket.php";

// Utilisation d'un header pour spécifier le type de contenu de la page. Ici, il s'agit juste de texte brut (text/plain). 
header("Content-Type: text/xml"); 
$filename='';
$filesize='';

$filename=(isset($_GET["filename"])) ? $_GET["filename"] : "";
$filesize = (isset($_GET["filesize"])) ? $_GET["filesize"] : "";
if($filename) {
	echo read_file("$filename");
}
?>
