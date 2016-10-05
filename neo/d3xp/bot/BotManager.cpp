/*
===========================================================================
File: BotManager.h
Author: John Wileczek
Description: Bot client management involving adding, removing, and event
dispatch to bots.
===========================================================================
*/

#include "BotManager.h"
#include "BotBrain.h"
#include "BotPlayer.h"
#include "framework/DeclEntityDef.h"
#include "framework/FileSystem.h"
#include "framework/Game.h"
#include "framework/async/NetworkSystem.h"

#define						THINK_SLICE 1.0

afiBotManager				afiBotManagerLocal;
afiBotManager *				BotManager = &afiBotManagerLocal;

unsigned int				afiBotManager::numBots = 0;
int							afiBotManager::numQueBots = 0;
int							afiBotManager::botEntityDefNumber[MAX_CLIENTS];
bool						afiBotManager::botSpawned[MAX_CLIENTS];
atomic<bool>				afiBotManager::gameEnd = false;
unsigned int				afiBotManager::threadUpdateCount = 0;
idCmdArgs					afiBotManager::cmdQue[MAX_CLIENTS];
idCmdArgs					afiBotManager::persistArgs[MAX_CLIENTS];
usercmd_t					afiBotManager::botCmds[MAX_CLIENTS];
idList<botInfo_t*>			afiBotManager::loadedBots;
afiBotBrain*				afiBotManager::brainFastList[MAX_CLIENTS];
botWorkerThread**			afiBotManager::workerThreadArray = nullptr;
unsigned int				afiBotManager::workerThreadCount = std::thread::hardware_concurrency();
condition_variable			afiBotManager::workerThreadDoneWorkConditional;
mutex						afiBotManager::workerThreadDoneWorkMutex;
mutex						afiBotManager::workerThreadMutex;
condition_variable			afiBotManager::workerThreadUpdateConditional;
PyInterpreterState*			afiBotManager::interpreterState = nullptr;
PyThreadState*				afiBotManager::mainThreadState = nullptr;
threadMap_t					afiBotManager::workerThreadMap;

//extern python module init functions
//BOOST_PYTHON_MODULE Creates these
//the names are different depending on python version
// Python 3 >= PyObject* PyInit_moduleName();
//Python 3 < void(*initfunc)(void) initModuleName();
extern "C" void initafiBotPlayer();
extern "C" void initafiBotBrain();
extern "C" void initidDict();
extern "C" void initidEntity();
extern "C" void initidVec2();
extern "C" void initidVec3();
extern "C" void initidAngles();
extern "C" void initidPlayer();
//TODO BUDDY_BOTS: Add new modules to system table upon startup
extern "C" void initidBounds();
extern "C" void initidRotation();
extern "C" void initidAAS();
extern "C" void initidActor();

// Workaround for problem in VS14
namespace boost
{
	template <>
	afiBotManager const volatile * get_pointer<class afiBotManager const volatile >(
		class afiBotManager const volatile *wrapped)
	{
		return wrapped;
	}
}


BOOST_PYTHON_MODULE(afiBotManager) {
	import("idPlayer");

	enum_<flagStatus_t>("flagStatus_t")
		.value("FLAGSTATUS_INBASE",FLAGSTATUS_INBASE)
		.value("FLAGSTATUS_TAKEN",FLAGSTATUS_TAKEN)
		.value("FLAGSTATUS_STRAY",FLAGSTATUS_STRAY)
		;

	class_<afiBotManager>("afiBotManager")
		.def("GetFlag", &afiBotManager::GetFlag, return_internal_reference<>())
		.def("GetFlagStatus", &afiBotManager::GetFlagStatus)
		.def("GetFlagCarrier", &afiBotManager::GetFlagCarrier, return_internal_reference<>())
		.def("GetWinningTeam", &afiBotManager::GetWinningTeam)
		.def("ConsolePrint",&afiBotManager::ConsolePrint)
		.staticmethod("GetFlag")
		.staticmethod("GetFlagStatus")
		.staticmethod("ConsolePrint")
		;
}

void afiBotManager::PrintInfo( void ) {
	common->Printf("Buddy Bots Initialized\n");
}

void afiBotManager::ConsolePrint(const char* string) {
	gameLocal.Printf(string);
}

bool afiBotManager::isGameEnding( ) {
	return gameEnd;
}

void afiBotManager::Cmd_ReloadAllBots_f(const idCmdArgs & args) {

	idPlayer* player = nullptr;

	//RestoreMainThreadState();
	//In-game 
	if (gameLocal.isMultiplayer) {

		for (int iBot = 0; iBot < numBots; iBot++) {
			afiBotBrain* oldBrain = brainFastList[iBot];
			afiBotPlayer* botPlayer = oldBrain->GetBody();

			botPlayer->thinkFlags &= (~TH_THINK);

			dict oldDict = oldBrain->botDict;

			botInfo_t* botProfile = FindBotProfileByIndex(botPlayer->clientNum);
			botInfo_t* newBotProfile = nullptr;
			bool loadedBot = LoadBot(botProfile->pakName, newBotProfile);

			if (newBotProfile == nullptr || loadedBot == false) {
				gameLocal.Warning("Could not reload bot: %s\n", botProfile->pakName.c_str());
				return;
			}
			newBotProfile->clientNum[botPlayer->clientNum] = botPlayer->clientNum;
			afiBotBrain* newBrain = SpawnBrain(newBotProfile->botName, botPlayer->clientNum);

			newBrain->SetPhysics((idPhysics_Player*)botPlayer->GetPhysics());
			newBrain->SetAAS();
			newBrain->SetBody(botPlayer);

			newBrain->botDict = oldDict;

			newBrain->Spawn();

			botPlayer->SetBrain(newBrain);


			botPlayer->thinkFlags |= TH_THINK;

			loadedBots.Remove(botProfile);
			AddBotInfo(newBotProfile);
		}

	}
	else {

		LoadAllBots();
	}
}

void afiBotManager::IncreaseThreadUpdateCount( ) {
	std::unique_lock<mutex> threadLock(workerThreadDoneWorkMutex);
	threadUpdateCount++;
	threadLock.unlock();
}

void afiBotManager::DecreaseThreadUpdateCount( ) {
	std::unique_lock<mutex> threadLock(workerThreadDoneWorkMutex);
	threadUpdateCount--;
	workerThreadDoneWorkConditional.notify_one();
	threadLock.unlock();
}

void afiBotManager::LaunchThreadsForFrame( ) {
	if(gameLocal.GameState() != GAMESTATE_ACTIVE || numBots == 0)
		return;

	SaveMainThreadState();

	std::unique_lock<mutex> threadLock(workerThreadMutex);
	workerThreadUpdateConditional.notify_all();
	threadLock.unlock();
}

void afiBotManager::WaitForThreadsTimed(  ) {
	if(gameLocal.GameState() != GAMESTATE_ACTIVE || numBots == 0)
		return;
	double milliseconds = numBots*THINK_SLICE;
	std::unique_lock<mutex> threadLock(workerThreadDoneWorkMutex);
	auto now = std::chrono::system_clock::now();
	auto timeToWait = std::chrono::milliseconds((int)milliseconds*1000);
	workerThreadDoneWorkConditional.wait_until(threadLock, now + timeToWait, [&]() {if(threadUpdateCount <= 0) return true; return false;});
	threadLock.unlock();

	RestoreMainThreadState();
}

void afiBotManager::InitializeThreadsForFrame(int deltaTimeMS ) {
	if(gameLocal.GameState() != GAMESTATE_ACTIVE || numBots == 0)
		return;

	if( numBots == 1) {
		for(unsigned int iClient = 0; iClient < MAX_CLIENTS; ++iClient) {
			if(IsClientBot(iClient)) {
				workerThreadArray[0]->AddUpdateTask(brainFastList[iClient]);
				IncreaseThreadUpdateCount();
				return;
			}
		}
	}

	//Attempt to calculate the amount of updates per thread that should occur to
	int idealTasksPerThread = max(numBots / workerThreadCount,1);
	unsigned int threadToFill = 0;
	unsigned int iClient = 0;
	unsigned int botsAdded = 0;
	unsigned int idealReached = 0;
	int expected = -1;
	bool fillThread = false;
	//First pass loop to fill all threads to idealTaskCount
	while(botsAdded < numBots) {
		fillThread = false;
		expected = -1;
		do {
			if(botsAdded > numBots) {
				break;
			}

			//Determine if this thread is currently running or if we can distribute work to it.
			if(std::atomic_compare_exchange_strong(&workerThreadArray[threadToFill]->endUpdateTask,&expected,0)) {
				IncreaseThreadUpdateCount();
				workerThreadArray[threadToFill]->currentUpdateTask = 0;
				workerThreadArray[threadToFill]->deltaTime = deltaTimeMS;
				fillThread = true;
			}
			else if( false == fillThread )
			{
				//The thread is processing or an error occured skip additional work for this frame
				break;
			}

			//If the client we are currently scanning is a bot then add it to be handled by this thread
			if(IsClientBot(iClient)) {
				workerThreadArray[threadToFill]->AddUpdateTask(brainFastList[iClient]);
				botsAdded++;
			}

			if(workerThreadArray[threadToFill]->endUpdateTask >= idealTasksPerThread) {
				idealReached++;
			}

			iClient++;
		} while(workerThreadArray[threadToFill]->endUpdateTask < idealTasksPerThread);

		if(idealReached == workerThreadCount && botsAdded < numBots) {
			for(threadToFill = 0; botsAdded < numBots;iClient++ ) {
				if(IsClientBot(iClient)) {
					workerThreadArray[threadToFill]->AddUpdateTask(brainFastList[iClient]);
					threadToFill++;
					botsAdded++;
				}
			}
			break;
		}
		threadToFill++;
	}
}

void afiBotManager::SetThreadState(PyThreadState* state, botWorkerThread* saveThread) {
	workerThreadMap[state] = saveThread;
}

void afiBotManager::UpdateThreadState(PyThreadState* state) {
	botWorkerThread* threadToUpdate = workerThreadMap[state];

	if(!threadToUpdate) {
		gameLocal.Error("Something has gone Terribly wrong with the worker threads.");
	}

	threadToUpdate->threadState = state;
}

void afiBotManager::SaveMainThreadState( ) {
	mainThreadState = PyEval_SaveThread();
}

void afiBotManager::RestoreMainThreadState( ) {
	
		PyEval_RestoreThread(mainThreadState);
}

void afiBotManager::InitializePython( ) {
	int result = PyImport_AppendInittab("idAngles",initidAngles);
	if( result == -1) {
		gameLocal.Error("Failed to Init idAngles Module");
	}
	if(PyImport_AppendInittab("idVec2", initidVec2) == -1) {
		gameLocal.Error("Failed to Init idVec2 Module");
	}
	if(PyImport_AppendInittab("idVec3", initidVec3) == -1) {
		gameLocal.Error("Failed to Init idVec3 Module");
	}
	if(PyImport_AppendInittab("idDict", initidDict) == -1) {
		gameLocal.Error("Failed to Init idDict Module");
	}

	if(PyImport_AppendInittab("idEntity", initidEntity) == -1) {
		gameLocal.Error("Failed to Init idEntity Module");
	}

	if(PyImport_AppendInittab("afiBotManager",initafiBotManager) == -1) {
		gameLocal.Error("Failed to Init afiBotManager Module");
	}

	if(PyImport_AppendInittab("afiBotPlayer", initafiBotPlayer) == -1) {
		gameLocal.Error("Failed to Init afiBotPlayer Module");
	}

	if(PyImport_AppendInittab("idAAS", initidAAS) == -1) {
		gameLocal.Error("Failed to Init idAAS Module");
	}

	if (PyImport_AppendInittab("idBounds", initidBounds) == -1) {
		gameLocal.Error("Failed to Init idBounds Module");
	}

	if (PyImport_AppendInittab("idActor", initidActor) == -1) {
		gameLocal.Error("Failed to Init idActor Module");
	}

	if (PyImport_AppendInittab("idRotation", initidRotation) == -1) {
		gameLocal.Error("Failed to Init afiBotBrain Module");
	}

	if (PyImport_AppendInittab("afiBotBrain", initafiBotBrain) == -1) {
		gameLocal.Error("Failed to Init afiBotBrain Module");
	}

	if (PyImport_AppendInittab("idPlayer", initidPlayer) == -1) {
		gameLocal.Error("Failed to Init idPlayer Module");
	}

	//Initialize Python
	Py_Initialize();

	PyEval_InitThreads();

	mainThreadState = PyThreadState_Get();
	interpreterState = mainThreadState->interp;
	//Grab the main module and globalNamespace
	gameLocal.main = object(handle<>(borrowed(PyImport_AddModule("__main__"))));

	gameLocal.globalNamespace = gameLocal.main.attr("__dict__");

	gameLocal.globalNamespace["sys"] = import("sys");
}

void afiBotManager::Initialize( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		cmdQue[i].Clear();
		persistArgs[i].Clear();
		botSpawned[i] = false;
		botEntityDefNumber[i] = 0;
		brainFastList[i] = NULL;
	}
	loadedBots.Clear();
	memset( &botCmds, 0, sizeof( botCmds ) );

	InitializePython();

	if(workerThreadCount <= 1) {
		workerThreadCount = 2;
	}
	//Initialize Worker threads based on count

	SaveMainThreadState();
	unsigned int threadsInitialized = 0;
	workerThreadArray = new botWorkerThread*[workerThreadCount];
	for(unsigned int iThread = 0; iThread < workerThreadCount; ++iThread) {
		workerThreadArray[iThread] = new botWorkerThread(&workerThreadUpdateConditional,&workerThreadMutex,interpreterState,&threadsInitialized);
	}

	//Wait for threads to initialize before continuing
	std::unique_lock<mutex> initLock(workerThreadMutex);
	workerThreadUpdateConditional.wait(initLock,[&](){ if(threadsInitialized >= workerThreadCount) return true; else return false;});
	initLock.unlock();

	RestoreMainThreadState();
}

void afiBotManager::Shutdown( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		cmdQue[i].Clear();
		persistArgs[i].Clear();
		botSpawned[i] = false;
		botEntityDefNumber[i] = 0;
		brainFastList[i] = NULL;
	}
	numBots = 0;
	
	SaveMainThreadState();

	workerThreadMutex.lock();
	gameEnd = true;
	//Notify anyone who may be waiting for a update
	workerThreadUpdateConditional.notify_all();
	workerThreadMutex.unlock();

	//clean up the worker thread memory
	//Might need a function here to wait for safe thread shutdown.
	for(unsigned int iThread = 0; iThread < workerThreadCount; ++iThread) {
		delete workerThreadArray[iThread];
		workerThreadArray[iThread] = nullptr;
	}

	//delete the memory for both the list, python is handling the cleanup of
	// bot information.
	RestoreMainThreadState();

	loadedBots.DeleteContents(true);
	//loadedBots.Clear();
	memset( &botCmds, 0, sizeof( botCmds ) );

	
	delete[] workerThreadArray;
	workerThreadArray = nullptr;

	CleanUpPython();

}

int afiBotManager::GetWinningTeam() {

	return gameLocal.mpGame.WinningTeam();
}

 idPlayer*  afiBotManager::GetFlagCarrier(int team) {

	int flagEntity = gameLocal.mpGame.GetFlagCarrier(team);

	if (flagEntity == -1) {
		return nullptr;
	}

	 idPlayer* playerEntity = ( idPlayer*)gameLocal.entities[flagEntity];

	return playerEntity;
}


void afiBotManager::CleanUpPython() {
	if(Py_IsInitialized()) {
		//RestoreMainThreadState();

		//PyThreadState_Clear(mainThreadState);
		//PyInterpreterState_Clear(interpreterState);

		//mainThreadState = PyEval_SaveThread();
		//PyThreadState_Delete(mainThreadState);
		//PyInterpreterState_Delete(interpreterState);

		Py_Finalize();
	}
}

void afiBotManager::ParseForBotName(void* defBuffer, unsigned bufferLength, const char* name, idStr& botName, idStr& authorName, idStr& botType, idStr& botSpawnClass) {
	idDict		botProfile;
	idParser	parser;

	//First load the file into the parser from memory
	if (!parser.LoadMemory((const char *)defBuffer, bufferLength, name)) {
		//Error
		gameLocal.Warning("Failed to Load %s into memory\n", name);
		return;
	}

	parser.SetFlags(LEXFL_NOSTRINGCONCAT);
	gameLocal.Printf("Loaded %s into memory\n", name);

	idToken keyToken, valueToken;
	bool beginToken = false;

	while (parser.ReadToken(&keyToken)) {
		if (!beginToken) {
			if (keyToken.Cmp("{") == 0) {
				beginToken = true;
			}
			continue;
		}

		if (keyToken.Cmp("}") == 0) {
			break;
		}

		if (!parser.ReadToken(&valueToken) || valueToken.Cmp("}") == 0) {
			break;
		}

		botProfile.Set(keyToken.c_str(), valueToken.c_str());
	}

	//Bots must have the "name" key, and "author" key defined in the entityDef to
	//be considered a valid bot.
	bool hasName = botProfile.GetString("scriptclass", "", botName);
	bool hasAuthor = botProfile.GetString("author", "", authorName);
	bool hasType = botProfile.GetString("bot_type", "", botType);
	bool hasSpawnClass = botProfile.GetString("scriptclass", "", botSpawnClass);

	if (hasName && hasAuthor && hasType && hasSpawnClass) {
		gameLocal.Printf("Loading bot %s by %s.\n", botName.c_str(), authorName.c_str());
		parser.FreeSource(false);
		return;
	}

	//If we made it down here there was a problem parsing this def file
	if (!hasName) {
		gameLocal.Warning("Bot does not have a name. Please fill out \"name\" key/value pair in entityDef %s\n", name);
	}

	if (!hasAuthor) {
		gameLocal.Warning("Bot does not have a author. Please fill out \"author\" key/value pair in entityDef %s\n", name);
	}

	if (!hasType) {
		gameLocal.Warning("Bot does not have a type. Please fill out \"botType\" key/value pair in entityDef %s\n", name);
	}

	parser.FreeSource(false);
}

bool	afiBotManager::LoadBot(idStr brainPakName, botInfo_t*& outputBotProfile) {

	idStr botName;
	idStr authorName;
	idStr botType;
	idStr botSpawnClass;
	//Information we need to load and copy bots def file
	char* defBuffer = NULL;
	idStr defFileName;
	int   defFileSize = -1;
	char* scriptBuffer = NULL;
	idStr scriptFileName;
	int   scriptFileSize = -1;
	char* mainScriptBuffer = NULL;

	//Generate the OS path to the pakFile
	idStr fullBrainPath = fileSystem->RelativePathToOSPath(brainPakName, "fs_basepath");

	//Unzip that pakFile and return a list of all the Files that exist within that pakFile
	idList<idFile_InZip*>* filesInZip = fileSystem->GetFilesInZip(fullBrainPath.c_str());

	for (int iFile = 0; iFile < ((*filesInZip)).Num(); ++iFile) {
		//A student should submit a .def file, which contains the basic information
		//needed to spawn their bot, and a set of python scripts that contains the actual decision making code
		//for their bot.
		idStr	fileName;
		fileName = ((*filesInZip)[iFile])->GetName();

		//If we find the .def script file in this pakFile we need to actually read and load this
		//file.
		if (fileName.CheckExtension(".def")) {
			defFileName = fileName;
			defFileSize = ((*filesInZip)[iFile])->Length();
			defBuffer = new char[defFileSize];
			((*filesInZip)[iFile])->Read((void*)defBuffer, defFileSize);
		}
	}

	if (!defBuffer) {
		gameLocal.Warning(".def file not found in %s pak file\n", brainPakName.c_str());
		return false;
	}

	//Use the idParser to run through the entityDef and determine the bots name, and who created it.
	//This information is used later when we want to spawn a bot to link the entityDef we are spawning
	//to the botBrain class defined in the dll.
	//Since the entityDef spawns the afiBotPlayer class not the derived botBrain class.
	ParseForBotName((void*)defBuffer, defFileSize, defFileName, botName, authorName, botType, botSpawnClass);

	
	 outputBotProfile = new botInfo_t();

	 outputBotProfile->pakName = brainPakName;
	 outputBotProfile->botName = botName;
	 outputBotProfile->authorName = authorName;
	 outputBotProfile->botSpawnClass = botSpawnClass;
	if (botType.Icmp("Code") == 0) {
		outputBotProfile->botType = BotType::CODE;
	}
	else if (botType.Icmp("Script") == 0) {
		outputBotProfile->botType = BotType::SCRIPT;
	}
	else if (botType.Icmp("Dll") == 0) {
		outputBotProfile->botType = BotType::DLL;
	}
	//Write the .def file to a folder where all the loaded bot entityDefs will be, so they can be loaded into the game later.
	fileSystem->WriteFile(va("loadedBots/def/%s", defFileName.c_str()), defBuffer, defFileSize);
	//Write the script files into their own folder for the bot, so we don't overwrite previously loaded dlls.
	idStr sysPath = "";
	idStr fullPath = "";
	object botMainDef;
	if (outputBotProfile->botType == BotType::SCRIPT) {
		//load all the script files that might run this bot into the bot's folder
		for (int iFile = 0; iFile < ((*filesInZip)).Num(); ++iFile) {
			idStr	fileName;
			fileName = ((*filesInZip)[iFile])->GetName();

			//If we find the .def or .dll or script files in this pakFile we need actually read and load these
			//files.
			if (fileName.CheckExtension(".py")) {
				scriptFileName = fileName;
				scriptFileSize = ((*filesInZip)[iFile])->Length();

				scriptBuffer = new char[scriptFileSize + 1];
				memset(scriptBuffer, 0, scriptFileSize + 1);

				((*filesInZip)[iFile])->Read((void*)scriptBuffer, scriptFileSize);
				fileSystem->WriteFile(va("loadedBots/%s/%s", botName.c_str(), scriptFileName.c_str()), scriptBuffer, scriptFileSize);
				if (-1 != scriptFileName.Find("_main", false)) {
					//This is the main file where the bot class is defined make our wrapper object from this
					sysPath = fileSystem->RelativePathToOSPath("loadedBots/", "fs_basepath");
					sysPath += botName + "\\";
					fullPath = sysPath + scriptFileName;
					fullPath = fullPath.BackSlashesToSlashes();

					mainScriptBuffer = scriptBuffer;

				}
				else {

					delete[] scriptBuffer;
					scriptFileSize = -1;
				
				}
			}
		}
		if (fullPath != "") {
			try {
				//This code appends the loaded bot directory to they python system path
				//so we can separate work into other python script files.
				object sys = gameLocal.globalNamespace["sys"];
				sys.attr("path").attr("insert")(0, sysPath.c_str());
				gameLocal.globalNamespace["sys"] = sys;

				str  script(const_cast<const char*>(mainScriptBuffer));
				botMainDef = exec(script, gameLocal.globalNamespace, gameLocal.globalNamespace);


			}
			catch (...) {
				gameLocal.HandlePythonError();
				return false;
			}
			delete[] mainScriptBuffer;
			//Setup python object to spawn the bot class later on.
			outputBotProfile->botClassInstance = gameLocal.globalNamespace[botSpawnClass.c_str()];
		}
	}



	delete[] defBuffer;
	//Memory created on the engine heap must also be freed on the engine heap
	fileSystem->FreeFilesInList(filesInZip);
	
	return true;
}

void	afiBotManager::LoadAllBots() {
	idFileList* brainPaks;
	int			iBrainPak;
	int			numBrainPaks;
	int			botLoadCount = 0;
	//List all the pakFiles in the folder where the botPaks should be.
	brainPaks = fileSystem->ListFiles("botPaks", ".pk4", true, true);

	loadedBots.DeleteContents(true);

	numBrainPaks = brainPaks->GetNumFiles();

	botInfo_t** newBotProfiles = new botInfo_t*[numBrainPaks];
	for (iBrainPak = 0; iBrainPak < numBrainPaks; ++iBrainPak, ++botLoadCount) {
		
		idStr currentBrainPak = brainPaks->GetFile(iBrainPak);

		bool botLoaded = LoadBot(currentBrainPak, newBotProfiles[botLoadCount]);
		if (!botLoaded) {
			gameLocal.Warning("Failed to Load bot at %s\n", currentBrainPak.c_str());
			botLoadCount--;
			continue;
		}
		
		AddBotInfo(newBotProfiles[botLoadCount]);
	}

	gameLocal.Printf(" %i Number of Bots Successfully Loaded\n", botLoadCount);
	gameLocal.Printf(" Spawn bots via addBot <botClassName> during or before a multi-player match");
	gameLocal.Printf(" *****************************************\n");
	gameLocal.Printf(" Loaded Bot Names:\n");
	for (iBrainPak = 0; iBrainPak < botLoadCount; ++iBrainPak) {

		gameLocal.Printf("%s, \t SpawnName: %s \t Author: %s\n", newBotProfiles[iBrainPak]->botName.c_str(),newBotProfiles[iBrainPak]->botSpawnClass.c_str(), newBotProfiles[iBrainPak]->authorName.c_str());
	}

	delete[] newBotProfiles;
	fileSystem->FreeFileList(brainPaks);

}

botInfo_t* afiBotManager::FindBotProfileByIndex(int clientNum) {
	botInfo_t* returnProfile = nullptr;
	unsigned int numProfiles = 0;
	if(clientNum > MAX_CLIENTS) {
		return returnProfile;
	}

	numProfiles = loadedBots.Num();
	for(unsigned int iProfile = 0; iProfile < numProfiles; ++iProfile) {

		if(loadedBots[iProfile]->clientNum[clientNum] == clientNum) {
			returnProfile = loadedBots[iProfile];
			break;
		}
	}
	return returnProfile;
}

idEntity* afiBotManager::GetFlag(int team) {
#ifdef CTF
	idEntity* theFlag = gameLocal.mpGame.GetTeamFlag(team);
	return theFlag;
#endif
}

int afiBotManager::GetFlagStatus(int team) {
#ifdef CTF
	return gameLocal.mpGame.GetFlagStatus(team);
#endif
}

void afiBotManager::AddBotInfo(botInfo_t* newBotInfo) {
	loadedBots.Append(newBotInfo);
}

void afiBotManager::UpdateUserInfo( void ) {
	assert( !gameLocal.isClient );
	for ( int i = MAX_CLIENTS - 1; i >= 0; i-- ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( afiBotPlayer::Type ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "updateUI %d\n", i ) );
		}
	}
}

void afiBotManager::Cmd_AddBot_f( const idCmdArgs& args ) {
	if ( gameLocal.isClient ) { // !gameLocal.isServer isn't valid soon enough for some reason
		gameLocal.Printf( "Bots may only be added on server\n" );
		return;
	}

	// Detour addbot commands for next idGameLocal::InitFromMapRestart,
	//    the Clients will be ready at that point
	idStr classname = args.Argv(1);
	botInfo_t* botProfile = NULL;
	botProfile = FindBotProfile(classname);
	if(botProfile == NULL) {
		gameLocal.Warning("No Loaded Bot Profile Named: %s \n",classname.c_str());
		return;
	}

	if ( gameLocal.GameState() != GAMESTATE_ACTIVE ) {
		if(numQueBots < MAX_CLIENTS) {
			common->Printf( "QUEUE SUCCESS: Adding Bot to Que\n" );
			cmdQue[numQueBots] = args;
			numQueBots++;
		}
		else
			common->Printf( "QUEUE FAILED: Max Bots in Queue\n" );
		return;
	}

	int numClients = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ){
		if ( gameLocal.entities[i] ) {
			numClients++;
		}
	}

	if ( numClients >= gameLocal.serverInfo.GetInt( "si_maxPlayers" ) ) {
		gameLocal.Printf( "Server is full\n" );
		return;
	}

	//If we have gotten down here the server isn't full and the game is running so we
	//can add the bot.
	AddBot( args );
}

void afiBotManager::AddBot( const idCmdArgs& args ) {
	idStr classname = args.Argv( 1 );
	if ( !classname.Length() ) {
		gameLocal.Printf( "No bot def specified." );
		return;
	}

	const idDeclEntityDef *botDef = gameLocal.FindEntityDef( classname.c_str(), false );
	if ( !botDef ) {
		if ( classname.Length() ) {
			gameLocal.Printf( "Unknown bot def '%s'.", classname.c_str() );
		}
		return;
	}

	//RestoreMainThreadState();

	//Start fake client connect
	int clientNum = networkSystem->ServerConnectBot();
	if ( clientNum == -1 ) {
		gameLocal.Printf( "No available slot for bot.\n" );
		return;
	}

	persistArgs[clientNum] = args;
	botSpawned[clientNum] = true;
	numBots++;

	// Index num of the bots def is saved so it can be sent to clients in order to spawn the right bot class
	SetBotDefNumber( clientNum, botDef->Index() );

	//This function calls spawnBot instead of spawnPlayer, which actually creates the
	//afiBotPlayer and afiBotBrain, and generates the proper linkage.
	gameLocal.ServerClientBegin(clientNum);

	gameLocal.Printf( "Bot added.\n" );
}

void afiBotManager::Cmd_RemoveBot_f( const idCmdArgs& args ) {
	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "This isn't multiplayer, so there no bots to remove, so yeah, you're mental.\n" );
		return;
	}

	if ( !gameLocal.isServer ) {
		gameLocal.Printf( "Bots may only be removed on server, only it has the powah!\n" );
		return;
	}

	idPlayer* player = gameLocal.GetClientByCmdArgs( args );
	if ( !player ) {
		gameLocal.Printf( "usage: removeBot <client nickname> or removeBot <client index>\n" );
		return;
	}
	if ( player && player->IsType( afiBotPlayer::Type ) ) {
		RemoveBot( player->entityNumber );
	} else {
		gameLocal.Printf( "There is no spoon, I mean, bot..." );
		return;
	}
}

void afiBotManager::Cmd_RemoveAllBots_f( const idCmdArgs & args ) {
	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "RemoveAllBots can only be used in a multiplayer game\n" );
		return;
	}

	if ( gameLocal.isClient ) {
		gameLocal.Printf( "You have no such power. This is a server command\n" );
		return;
	}

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		RemoveBot( i );
	}
}

void afiBotManager::Cmd_ReloadBot_f( const idCmdArgs& args ) {
	if( gameLocal.isClient ) {
		gameLocal.Printf( "The force is not strong with you. You are a mere mortal client.\n" );
		return;
	}

	if( args.Argc() < 1 ) {
		gameLocal.Warning("Incorrect command usage of reloadBot command.\n");
		gameLocal.Warning("Usage: reloadBot <botName,clientIndex> (in-game)\n");
		gameLocal.Warning("Usage: reloadBot <botClassName> \n");
		return;
	}

	idPlayer* player = nullptr;
	
	//RestoreMainThreadState();
	//In-game 
	if (gameLocal.isMultiplayer) {
		player = gameLocal.GetClientByCmdArgs(args);

		if (player == nullptr) {
			gameLocal.Warning("Cannot Find Bot please check command arguements again.\n");
			gameLocal.Warning("Usage: reloadBot <botName,clientIndex> (in-game)\n");
			gameLocal.Warning("Usage: reloadBot <botClassName> \n");
			return;
		}
		afiBotPlayer* botPlayer = reinterpret_cast<afiBotPlayer*>(player);

		botPlayer->thinkFlags &= (~TH_THINK);

		afiBotBrain* oldBrain = botPlayer->GetBrain();
		dict oldDict = oldBrain->botDict;

		botInfo_t* botProfile = FindBotProfileByIndex(botPlayer->clientNum);
		botInfo_t* newBotProfile = nullptr;
		bool loadedBot = LoadBot(botProfile->pakName, newBotProfile);

		if (newBotProfile == nullptr || loadedBot == false) {
			gameLocal.Warning("Could not reload bot: %s\n", botProfile->pakName.c_str());


			botPlayer->thinkFlags |= TH_THINK;

			return;
		}
		newBotProfile->clientNum[botPlayer->clientNum] = botPlayer->clientNum;
		afiBotBrain* newBrain = SpawnBrain(newBotProfile->botName, botPlayer->clientNum);

		newBrain->SetPhysics((idPhysics_Player*)botPlayer->GetPhysics());
		newBrain->SetAAS();
		newBrain->SetBody(botPlayer);

		newBrain->botDict = oldDict;

		newBrain->Spawn();

		botPlayer->SetBrain(newBrain);

		
		botPlayer->thinkFlags |= TH_THINK;

		loadedBots.Remove(botProfile);
		AddBotInfo(newBotProfile);

	}
	else {
		const char* className = nullptr;
		if (!player) {
			className = args.Argv(1);

			botInfo_t* pakFileToLoad = FindBotProfileByClassName(className);
			botInfo_t* newBotProfile = nullptr;
			if (false == LoadBot(pakFileToLoad->pakName, newBotProfile)) {
				gameLocal.Warning("Failed to Reload Bot %s", className);
			}
			loadedBots.Remove(pakFileToLoad);
			AddBotInfo(newBotProfile);

		}
	}

	//SaveMainThreadState();
	

	
	

}

botInfo_t* afiBotManager::ReloadPak(botInfo_t* botProfile, int clientNum) {
	idFileList*	brainPaks;
	unsigned int			iBrainPak;
	unsigned int			numBrainPaks;

	//List all the pakFiles in the folder where the botPaks should be.
	brainPaks = fileSystem->ListFiles("botPaks", ".pk4", true, true);

	numBrainPaks = brainPaks->GetNumFiles();

	for (iBrainPak = 0; iBrainPak < numBrainPaks; ++iBrainPak) {
		idStr botName;
		idStr authorName;
		idStr botType;
		idStr botSpawnClass;
		//Information we need to load and copy bots def file
		char* defBuffer = NULL;
		idStr defFileName;
		int   defFileSize = -1;
		char* scriptBuffer = NULL;
		idStr scriptFileName;
		int   scriptFileSize = -1;
		char* mainScriptBuffer = NULL;
		idStr currentBrainPak = brainPaks->GetFile(iBrainPak);

		if (currentBrainPak.Icmp(botProfile->pakName) != 0) {
			continue;
		}
		else if (iBrainPak == numBrainPaks - 1) {
			gameLocal.Warning("Could not find pak file named %s\n", botProfile->pakName.c_str());
			return nullptr;
		}

		//Generate the OS path to the pakFile
		idStr fullBrainPath = fileSystem->RelativePathToOSPath(currentBrainPak, "fs_basepath");

		//Unzip that pakFile and return a list of all the Files that exist within that pakFile
		idList<idFile_InZip*>* filesInZip = fileSystem->GetFilesInZip(fullBrainPath.c_str());

		unsigned int numFilesInZip = ((*filesInZip)).Num();
		for (unsigned int iFile = 0; iFile < numFilesInZip; ++iFile) {
			//A student should submit a .def file, which contains the basic information
			//needed to spawn their bot
			idStr	fileName;
			fileName = ((*filesInZip)[iFile])->GetName();

			//If we find the .def file we need to copy this file
			if (fileName.CheckExtension(".def")) {
				defFileName = fileName;
				defFileSize = ((*filesInZip)[iFile])->Length();
				defBuffer = new char[defFileSize];
				((*filesInZip)[iFile])->Read((void*)defBuffer, defFileSize);
			}
		}

		if (!defBuffer) {
			gameLocal.Warning(".def file not found in %s pak file\n", currentBrainPak.c_str());
		}

		//Use the idParser to run through the entityDef and determine the bots name, and who created it.
		//This information is used later when we want to spawn a bot to link the entityDef we are spawning
		//to the botBrain class defined in the dll.
		//Since the entityDef spawns the afiBotPlayer class not the derived botBrain class.
		ParseForBotName((void*)defBuffer, defFileSize, defFileName, botName, authorName, botType, botSpawnClass);

	}
}

void afiBotManager::DropABot( void ) {
	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "DropABot can only be used in a multiplayer game\n" );
		return;
	}

	if ( gameLocal.isClient ) {
		gameLocal.Printf( "You have no such power. This is a server command\n" );
		return;
	}

	//Remove the first bot we find.
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( afiBotPlayer::Type ) ) {
			RemoveBot( i );
			break;
		}
	}
}

void afiBotManager::RemoveBot( int clientNum ) {
	//persistArgs[ clientNum ].Clear();
	//botSpawned[ clientNum ] = false;
	if ( gameLocal.entities[ clientNum ] && gameLocal.entities[ clientNum ]->IsType( afiBotPlayer::Type ) ) {
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say Removing Bot '%s^0'\n", gameLocal.userInfo[ clientNum ].GetString( "ui_name" ) ) );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %d\n", gameLocal.entities[ clientNum ]->entityNumber ) );
	}
}

int afiBotManager::IsClientBot( int clientNum ) {
	return botSpawned[clientNum];
}

void afiBotManager::SetBotDefNumber( int clientNum, int botDefNumber ) {
	botEntityDefNumber[clientNum] = botDefNumber;
}

/*
===================
afiBotManager::GetBotDefNumber
Gets the bot's entityDef number
===================
*/
int afiBotManager::GetBotDefNumber( int clientNum ) {
	return botEntityDefNumber[clientNum];
}

/*
===================
afiBotManager::GetBotClassname
Gets the bot's classname
===================
*/
idStr afiBotManager::GetBotClassname( int clientNum ) {
	return (idStr)declManager->DeclByIndex( DECL_ENTITYDEF, botEntityDefNumber[clientNum], false )->GetName();//botDeclName;
}

/*
===================
afiBotManager::SpawnBot
Equivalent of gamelocal spawnplayer except it loads from cmdargs
===================
*/
void afiBotManager::SpawnBot( int clientNum ) {
	idDict spawnDict;
	idStr classname;
	afiBotPlayer* playerBody;
	afiBotBrain* brain;

	gameLocal.DPrintf( "SpawnBot: %i\n", clientNum );

	classname = GetBotClassname( clientNum );
	if ( !classname.Length() ) {
		gameLocal.Warning( "Unknown classname '%s', defaulting to 'bot'.", classname );
		classname = "bot";
	}

	//Default setup of spawnDict if .def file is not filled out.
	spawnDict.SetInt( "spawn_entnum", clientNum );
	spawnDict.Set( "name", va( "bot%d", clientNum ) );
	spawnDict.Set( "classname", classname );
	spawnDict.SetBool( "hide", false );

	//Finding the loaded entityDef for the class.
	const idDict* botEntityDict = gameLocal.FindEntityDefDict( classname, false );
	if ( !botEntityDict ) {
		if ( classname ) {
			gameLocal.Error( "Unknown classname '%s'.", classname );
		}
		return;
	}

	//Copying those key/value pairs from the loaded entityDef over the default.
	spawnDict.Copy( *botEntityDict );

	// key/values passed from cmd args
	if ( gameLocal.isServer ) {
		const char *key, *value;
		const idCmdArgs * cmdArgs = GetPersistArgs( clientNum );
		if ( cmdArgs ) {
			for( int i = 2; i < cmdArgs->Argc() - 1; i += 2 ) {
				key = cmdArgs->Argv( i );
				value = cmdArgs->Argv( i + 1 );
				spawnDict.Set( key, value );
			}
		}
	}

	//Spawn the afiBotPlayer from the entityDef
	//Grab the name of the bot from the loaded entityDef
	idStr botName = spawnDict.GetString("scriptclass");
	botInfo_t* botProfile = FindBotProfile(botName);

	//Must be called the for the CallSpawn() function to operate properly.
	//Game spawns one entity at a time and uses single spawnArgs to spawn the entity.
	gameLocal.SetSpawnArgs(spawnDict);

	if( botProfile ) {
		botProfile->clientNum[clientNum] = clientNum;
		//If we are a script bot then the bot instances will be created from python.
		if(botProfile->botType == SCRIPT) {
			brain = SpawnBrain(botName,clientNum);

			object botPlayerClass = gameLocal.globalNamespace["afiBotPlayer"];
			brain->scriptBody = botPlayerClass();
			playerBody = extract<afiBotPlayer*>(ptr(brain->scriptBody));

			if ( clientNum >= gameLocal.numClients ) {
				gameLocal.numClients = clientNum + 1;
			}

			//Create a boost python dictionary
			brain->botDict = dict();
			
			//Copy from our dictonary to python dictonary
			//Since idDictonaries contain just string representations
			//we parse the strings and convert keys/values that are supposed
			//to be numbers to numbers
			int numPairs = spawnDict.GetNumKeyVals();
			for( int iPair = 0; iPair < numPairs; ++iPair ) {
				const idKeyValue* keyVal = spawnDict.GetKeyVal(iPair);
				idStr keyStr = keyVal->GetKey();
				idStr valStr = keyVal->GetValue();
				int keyInt = -INT_MAX;
				float keyFloat = std::numeric_limits<float>::quiet_NaN();
				if(keyStr.IsNumeric() ) {
					if(idStr::IsFloat(keyStr)) {
						keyFloat = atof(keyStr.c_str());
					}
					keyInt = atoi(keyStr.c_str());
				}
				
				if( keyInt != -INT_MAX ) {
					//Setting based on int key
					SetDictionaryValue(keyInt,brain,valStr);
				}
				else if( !_isnan(keyFloat) ) {
					//Setting based on float key
					SetDictionaryValue(keyFloat,brain,valStr);
				}
				else {
					//Setting based on string
					SetDictionaryValue(keyStr.c_str(),brain,valStr);
				}

			}

			//Necessary part of entity spawning process to initialize all the variables
			//from the hierarchy.
			playerBody->CallSpawn();

			playerBody->clientNum = clientNum;
			playerBody->botName = botProfile->botName;
			//Link all the necessary components between brain and body.
			brain->SetBody( playerBody );
			brain->SetPhysics( ( idPhysics_Player* )playerBody->GetPhysics() );
			playerBody->SetAAS();
			brain->SetAAS();
			playerBody->SetBrain(brain);
			//Call the script spawn function after entity has been created.
			brain->Spawn();
		}
	}
	//Let the networked game know that a bot spawned.
	gameLocal.mpGame.SpawnPlayer( clientNum );
}

template<typename T>
void afiBotManager::SetDictionaryValue(T key,afiBotBrain* brain,idStr valStr) {
	int valInt = -INT_MAX;
	float valFloat = std::numeric_limits<float>::quiet_NaN();
	if(valStr.IsNumeric() ) {
		if(idStr::IsFloat(valStr)) {
			valFloat = atof(valStr.c_str());
		}
		valInt = atoi(valStr.c_str());
	}
	if( valInt != -INT_MAX ) {
		brain->botDict[key] = valInt;
		return;
	}
	else if( !_isnan(valFloat) ) {
		brain->botDict[key] = valFloat;
		return;
	}

	brain->botDict[key] = valStr.c_str();
	
}

afiBotBrain* afiBotManager::SpawnBrain(idStr botName,int clientNum) {
	afiBotBrain*	returnBrain = NULL;
	botInfo_t*		loadedBotProfile = NULL;

	loadedBotProfile = FindBotProfile(botName);

	if( loadedBotProfile != NULL ) {
		if( 0 == botName.Cmp(loadedBotProfile->botName.c_str()) ) {
			try {
				loadedBotProfile->scriptInstances[clientNum] = loadedBotProfile->botClassInstance();
				returnBrain = extract<afiBotBrain*>(loadedBotProfile->scriptInstances[clientNum].ptr());
			}
			catch(...) {
				gameLocal.HandlePythonError();
			}

			//Place a reference to the spawnedBrain in the fast list
			//so we can easily dispatch event functions later on
			brainFastList[clientNum] = returnBrain;
		}
	}

	return returnBrain;
}

botInfo_t*  afiBotManager::FindBotProfile(idStr botName) {
	botInfo_t* botProfile = NULL;
	int numLoadedBots;
	int iBotProfile = 0;

	numLoadedBots = loadedBots.Num();
	for(iBotProfile = 0; iBotProfile < numLoadedBots; ++iBotProfile) {
		idStr loadedName = loadedBots[iBotProfile]->botName;

		if(0 == botName.Icmp(loadedName.c_str()) ) {
			botProfile = loadedBots[iBotProfile];
			return botProfile;
		}
	}
	return botProfile;
}

botInfo_t* afiBotManager::FindBotProfileByClassName(idStr botClassName) {
	botInfo_t* botProfile = NULL;
	int iBotProfile = 0;

	int numLoadedBots = loadedBots.Num();
	for (iBotProfile = 0; iBotProfile < numLoadedBots; iBotProfile++) {
		if (0 == botClassName.Icmp(loadedBots[iBotProfile]->botSpawnClass)) {
			botProfile = loadedBots[iBotProfile];
			return botProfile;
		}
	}
	return botProfile;
}

void afiBotManager::ProcessChat(const char* text) {
}
/*
===================
afiBotManager::OnDisconnect
===================
*/
			
void afiBotManager::OnDisconnect( int clientNum ) {
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );
	botSpawned[ clientNum ] = false;
	memset( &botCmds[ clientNum ], 0, sizeof( usercmd_t ) );
	botEntityDefNumber[ clientNum ] = -1;

	//TODO: Let other bots know about the disconnect of a player.
	for (unsigned int iClient = 0; iClient < MAX_CLIENTS; ++iClient) {

		if (botSpawned[iClient])
			brainFastList[iClient]->OnDisconnect(clientNum);
	}
}

/*
===================
afiBotManager::InitBotsFromMapRestart
Connect all qued bots to the game or refresh current bots
===================
*/
void afiBotManager::InitBotsFromMapRestart( void ) {
	if ( !gameLocal.isServer ) return;

	//This function gets called on map load.
	//All qued up bots are added. The Queueing of bots allows
	//us to tackle two birds with one stone. This allows us to
	//setup a match before we actually enter the game, and handles
	//if we should switch maps on a running server and need to reload bots.
	gameLocal.Printf( "***Starting Bot Refreshes\n" );
	for ( int botID = 0; botID < MAX_CLIENTS; botID++ ) {
		if ( IsClientBot( botID ) ) {
			common->Printf( "***Adding existing Bot to Que\n" );
			cmdQue[numQueBots] = persistArgs[botID];
			persistArgs[botID].Clear();
			botSpawned[botID] = false;
			numQueBots++;
		}
	}
	numBots = 0;
	// Add bots from command line or config Que -
	if ( numQueBots ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			botSpawned[i] = false;
		}
		for ( int i = 0; i < numQueBots; i++ ) {
			common->Printf( "***Adding Bot %i from Que\n", i );
			Cmd_AddBot_f( cmdQue[i] );
			cmdQue[i].Clear();
		}
		numQueBots = 0;
		return;
	}
}

/*
=====================
afiBotManager::GetPersistArgs
=====================
*/
idCmdArgs *	afiBotManager::GetPersistArgs( int clientNum ) {
	idCmdArgs * args = &(persistArgs[ clientNum ]);
	if ( args->Argc() ) {
		return args;
	} else {
		return NULL;
	}
}

/*
===================
afiBotManager::GetUserCmd
===================
*/
usercmd_t * afiBotManager::GetUserCmd( int clientNum ) {
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );
	return &( botCmds[clientNum] );
}

/*
===================
afiBotManager::SetUserCmd
===================
*/
void afiBotManager::SetUserCmd( int clientNum, usercmd_t * usrCmd ) {
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );

	botCmds[clientNum].angles[0] = usrCmd->angles[0];
	botCmds[clientNum].angles[1] = usrCmd->angles[1];
	botCmds[clientNum].angles[2] = usrCmd->angles[2];
	botCmds[clientNum].forwardmove = usrCmd->forwardmove;
	botCmds[clientNum].rightmove = usrCmd->rightmove;
	botCmds[clientNum].upmove =	usrCmd->upmove;
	botCmds[clientNum].buttons = usrCmd->buttons;
	botCmds[clientNum].impulse = usrCmd->impulse;
}

/*
===================
afiBotManager::afiBotManager
===================
*/
afiBotManager::afiBotManager() {
}

/*
===================
afiBotManager::~afiBotManager
===================
*/
afiBotManager::~afiBotManager() {
	
}
