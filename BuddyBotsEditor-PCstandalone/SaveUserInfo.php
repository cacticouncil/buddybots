<?php
//$filename = $_POST['uiname']; 
//create user def:
if (isset($_POST['username']) && isset($_POST['uiname'])) { // check if both fields are set
  $filename = $_POST['uiname'];
  $fh = fopen("$filename.def", 'wa+'); 

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
	//header('Location: index.html');
}

//create statemachine:
$finp = "statemachine.py";
$fi = fopen($finp, 'wa+'); 

$in = 'from afiBotBrain import *
from afiBotManager import *
from afiBotPlayer import *
from idPlayer import *
from idActor import *
from idEntity import *
from idVec3 import *

class IState:
	def __init__(self,owner):
		self.owner = owner
		return
	def Update(self):
		return
	def Init(self):
		return
	def Shutdown(self):
		return
	def DetermineNewState(self,currState):
		return
	def Transition(self,newState):
		return
		
		

class AssaultFlag(IState):
	def __init__(self,owner):
		self.owner = owner
		return
	def Update(self):
		return transition
	def Init(self):
		return
	def Shutdown(self):
		return
	def DetermineNewState(self,currState):
		return';

fwrite($fi,$in); 
fclose($fi);

//save bot code:
$bots = fopen("$filename.py", 'wa+');
$btxt = $_POST['myKey'];
fwrite($bots,$btxt); 
fclose($bots); 

//create bots:
//$bots = "botscode.py";
$files = array("$filename.def",$finp,$bots);

    # create new zip opbject
    $zip = new ZipArchive();

    # create a temp file & open it
    $tmp_file = tempnam('.','');
    $zip->open($tmp_file, ZipArchive::CREATE);

    # loop through each file
    foreach($files as $file){

        # download file
        $download_file = file_get_contents($file);

        #add it to the zip
        $zip->addFromString(basename($file),$download_file);

    }

    # close zip
    $zip->close();

    # send the file to the browser as a download
    header('Content-disposition: attachment; filename=YouBotName.pk4');
    header('Content-type: application/zip');
    readfile($tmp_file);

    unlink($tmp_file);
    unlink("$filename.def");
    unlink($finp);
?>