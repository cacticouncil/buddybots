<?php
$txt = "bots/userdata.def"; 
if (isset($_POST['username']) && isset($_POST['uiname'])) { // check if both fields are set
    $fh = fopen($txt, 'wa+'); 

	//making .def file
    $txt='entityDef '.$_POST['uiname'].'{
	"inherit"		"bot"
	
	"author"		"'.$_POST['username'].'"
	"bot_type"		"Script"
	"scriptclass"	"'.$_POST['uiname'].'"
	
	"ui_name"		"'.$_POST['uiname'].'"
	}'; 


    fwrite($fh,$txt); 
    fclose($fh); 
	header('Location: index.html');
}
?>