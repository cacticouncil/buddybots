/*
===========================================================================
File: BotManager.h
Author: John Wileczek
Edited by: Esteban Isaiah Nazario
Description: Bot client management involving adding, removing, and event
dispatch to bots.
===========================================================================
*/
#ifndef BOTMANAGER_H_
#define BOTMANAGER_H_

#include "../Game_local.h"
#include "../MultiplayerGame.h"
#include <unordered_map>
#include <atomic>
#include <condition_variable>
#include "idlib/Timer.h"

using namespace std;

class afiBotBrain;
class idEntity;
class afiBotPlayer;
class botWorkerThread;
class afiBotManager;

enum	BotType { CODE, SCRIPT, DLL };

typedef afiBotBrain* (*CreateBotBrain_t)(botImport_t* dllSetup);
typedef std::unordered_map<PyThreadState*, botWorkerThread*> threadMap_t;


//This custom call policy allows me to mitigate some of the losses in performance due to
//the python GIL. When Some c++ functions are exectued from python scripts we can give up control
//of the GIL for the duration of the function.
namespace boost {
	namespace python {
		struct release_gil_policy {
			template<class ArgumentPackage>
			static bool precall(ArgumentPackage const&) {
				PyThreadState* saveState = PyEval_SaveThread();
				afiBotManager::UpdateThreadState(saveState);

				return true;
			}

			template<class ArgumentPackage>
			static PyObject* postcall(ArgumentPackage const&, PyObject* result) {
				PyThreadState* state = PyGILState_GetThisThreadState();
				PyEval_RestoreThread(state);
				return result;
			}

			typedef default_result_converter result_converter;
			typedef PyObject* argument_package;

			template<class Sig>
			struct extract_return_type : mpl::front<Sig>
			{
			};
		private:
		};
	}
}

//Bot Info is in effect a bot profile that get fills out upon game initialize
//for all valid bots located in the botPaks folder.Spawning multiple instances of
//the same bot is currently being handled, but event handling for multiple instances
//still needs to be added. (should just be creating a array of client nums for the instances)
typedef struct botInfo_s {
	idStr				pakName;
	idStr				botName;
	idStr				teamName;
	idStr				authorName;
	idStr				botSpawnClass;
	object				scriptInstances[MAX_CLIENTS];
	object				botClassInstance;

	int					clientNum[MAX_CLIENTS];
	int					entityNum[MAX_CLIENTS];
	int					botType;
	idCmdArgs			cmdArgs[MAX_CLIENTS];
	botInfo_s() {
		botName = "";
		teamName = "";
		authorName = "";
		for (unsigned int iClient = 0; iClient < MAX_CLIENTS; iClient++) {
			clientNum[iClient] = -1;
			cmdArgs[iClient].Clear();
			entityNum[iClient] = -1;
		}

		botType = CODE;
	}

	~botInfo_s() {
		botName = "";
		teamName = "";
		authorName = "";
		for (unsigned int iClient = 0; iClient < MAX_CLIENTS; iClient++) {
			clientNum[iClient] = 0;
			cmdArgs[iClient].Clear();
			entityNum[iClient] = -1;
		}
	}
} botInfo_t;

typedef struct teamInfo_s {
	idStr teamName;
	int size;
	idStrList bots;
	idList<bool> used;

	teamInfo_s() {
		teamName = "";
		size = 0;
	}

	~teamInfo_s() {
		teamName = "";
		size = 0;

		bots.Clear();
	}
} teamInfo_t;

typedef struct removeInfo_s {
	idStr botName;
	idStr teamName;
	//For Team Removal Toggling
	bool remove;

	removeInfo_s() {
		botName = "";
		teamName = "";
		remove = false;
	}

	~removeInfo_s() {
		botName = "";
		teamName = "";
		remove = false;
	}
} removeInfo_t;

//Worker thread class currently responsible for running the update tasks each frame for the bots
//Unfortunately due to the Python GIL the benefit of this class is somewhat reduced since python code
//will never be executing on more than one thread at a time.
class botWorkerThread {
	friend class afiBotManager;

public:
	botWorkerThread(condition_variable* conditional_variable, mutex* thread_mutex, PyInterpreterState* mainState, unsigned int* initializeCounter);
	~botWorkerThread();

	void						InitializeForFrame(unsigned int endUpdateIndex);
	void						RunWork();
	void						AddUpdateTask(afiBotBrain* newTask);
protected:

private:
	bool						CheckWorkTime();
	bool						LookForMoreWork();
	void						RemoveFailedBot(int removeIndex);

	afiBotBrain*				packedUpdateArray[MAX_CLIENTS];
	atomic<int>					currentUpdateTask;
	atomic<int>					endUpdateTask;
	int							currentFrameTime;
	int							deltaTime;
	thread						threadObj;
	idTimer						workTimer;
	//Thread control variables manipulated by thread or bot manager
	unsigned int*				threadInitializeCounter;
	condition_variable*			threadConditional;
	mutex*						threadMutex;
	PyThreadState*				threadState;
	PyInterpreterState*			interpState;
};

/*
===============================================================================

afiBotManager
Responsible for the loading,adding,removing, and event dispatch for bots.

===============================================================================
*/
class  afiBotManager {
public:
	static void					PrintInfo(void);
	static void					Initialize(void);
	static void					Shutdown(void);
	static void					UpdateUserInfo(void);
	static void					ConsolePrint(const char* string);

	static void					Cmd_BotInfo_f(const idCmdArgs& args);
	static void					Cmd_AddBot_f(const idCmdArgs& args);
	static void					Cmd_AddTeam_f(const idCmdArgs& args);
	static void					Cmd_RemoveBot_f(const idCmdArgs& args);
	static void					Cmd_RemoveAllBots_f(const idCmdArgs & args);
	static void					Cmd_RemoveTeam_f(const idCmdArgs& args);
	static void					Cmd_RemoveAllTeams_f(const idCmdArgs& args);
	static void					Cmd_ReloadBot_f(const idCmdArgs& args);
	static void					Cmd_ReloadAllBots_f(const idCmdArgs& args);
	static void					Cmd_PrintAllBots_f(const idCmdArgs& args);
	static void					Cmd_RemoveBotIndex_f(const idCmdArgs& args);
	static void					AddBot(const idCmdArgs& args);
	static void					DropABot(void);
	static void					RemoveBotMP(int clientNum);
	static void					RemoveBotPersistArgs(const int i);
	static void					RemoveBotCmdQue(const int i);
	static int					IsClientBot(int clientNum);
	static void					SetBotDefNumber(int clientNum, int botDefNumber);
	static int					GetBotDefNumber(int clientNum);
	static int					GetClientNumByPersistantArgs(const idCmdArgs& args);
	static idStr				GetBotClassname(int clientNum);
	static void					SpawnBot(int clientNum);
	static void					OnDisconnect(int clientNum);

	//Thread related functions
	static void					InitializeThreadsForFrame(int deltaTimeMS);
	static void					LaunchThreadsForFrame();
	static void					WaitForThreadsTimed();
	static bool					isGameEnding();
	static void					IncreaseThreadUpdateCount();
	static void					DecreaseThreadUpdateCount();
	static void					SetThreadState(PyThreadState* state, botWorkerThread* saveThread);
	static void					UpdateThreadState(PyThreadState* state);

	static void					SaveMainThreadState();
	static void					RestoreMainThreadState();

	static int					GetWinningTeam();
	static idPlayer*			GetFlagCarrier(int team);
	static idEntity*			GetFlag(int team);
	static int					GetFlagStatus(int team);
	static void					ProcessChat(const char* text);
	static void					InitBotsFromMapRestart();
	static idCmdArgs *			GetPersistArgs(int clientNum);
	static usercmd_t *			GetUserCmd(int clientNum);
	static void					SetUserCmd(int clientNum, usercmd_t * usrCmd);
	static void					WriteUserCmdsToSnapshot(idBitMsg& msg);
	static void					ReadUserCmdsFromSnapshot(const idBitMsg& msg);
	static void					AddBotInfo(botInfo_t* newBotInfo);
	static void					AddTeamInfo(teamInfo_t* newTeamInfo);
	static void					AddRemoveInfo(removeInfo_t* newRemoveInfo);
	static afiBotBrain*			SpawnBrain(idStr botName, int clientNum);
	static botInfo_t*			FindBotProfile(idStr botName);
	static teamInfo_t*			FindTeamProfile(idStr teamName);
	static botInfo_t*			FindBotProfileByIndex(int clientNum);
	static botInfo_t*			FindBotProfileByClassName(idStr botClassName);
	static botInfo_t*			ReloadPak(botInfo_t* botProfile, int clientNum);
	static bool					LoadBot(idStr brainPakName, botInfo_t*& outputBotProfile);
	static bool					LoadTeam(idStr teamPakName, teamInfo_t*& outputTeamProfile);
	static void					LoadAllBots();
	static void					ParseForBotName(void* defBuffer, unsigned bufferLength, const char* name, idStr& botName, idStr& authorName, idStr& botType, idStr& botSpawnClass);
	static void					ParseForTeamName(void* defBuffer, unsigned bufferLength, const char* name, idStr& teamName, int& teamSize, idStrList& bots, idList<bool>& used);

	afiBotManager();
	~afiBotManager();

	static void					CleanUpPython();


protected:
	static usercmd_t			botCmds[MAX_CLIENTS];
private:
	template<typename T>
	static void					SetDictionaryValue(T key, afiBotBrain* brain, idStr valStr);
	static void					DistributeWorkToThreads();
	static void					InitializePython();

	static idList<botInfo_t*>	loadedBots;
	static idList<removeInfo_t*>removeBots;
	static idList<teamInfo_t*>	loadedTeams;
	static idList<teamInfo_t*>	addedTeams;
	static unsigned int			numBots;
	static int					numQueBots;

	//Worker thread control variables
	static unsigned int			workerThreadCount;
	static condition_variable	workerThreadDoneWorkConditional;
	static mutex				workerThreadDoneWorkMutex;
	static condition_variable	workerThreadUpdateConditional;
	static mutex				workerThreadMutex;
	static botWorkerThread**	workerThreadArray;
	static threadMap_t			workerThreadMap;
	static atomic<bool>			gameEnd;
	static unsigned int			threadUpdateCount;
	static PyInterpreterState*	interpreterState;
	static PyThreadState*		mainThreadState;

	static idCmdArgs			cmdQue[MAX_CLIENTS];
	static idCmdArgs			persistArgs[MAX_CLIENTS];
	static bool					botSpawned[MAX_CLIENTS];
	static int					botEntityDefNumber[MAX_CLIENTS];
	static afiBotBrain*			brainFastList[MAX_CLIENTS];
};

#endif