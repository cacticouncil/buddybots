/*
===========================================================================
File: BotManager.h
Author: John Wileczek
Description: Bot client management involving adding, removing, and event
dispatch to bots.
===========================================================================
*/
#ifndef BOTMANAGER_H_
#define BOTMANAGER_H_

#include "../Game_local.h"
#include "../MultiplayerGame.h"

class afiBotBrain;
class idEntity;
class afiBotPlayer;
class botWorkerThread;

typedef std::unordered_map<PyThreadState*,botWorkerThread*> threadMap_t;
typedef afiBotBrain* (*CreateBotBrain_t)(botImport_t* dllSetup);

enum	BotType { CODE,SCRIPT,DLL };

//Bot Info is in effect a bot profile that get fills out upon game initialize
//for all valid bots located in the botPaks folder.Spawning multiple instances of
//the same bot is currently being handled, but event handling for multiple instances
//still needs to be added. (should just be creating a array of client nums for the instances)
typedef struct botInfo_s {
	idStr				botName;
	idStr				authorName;
	afiBotBrain*		brain;
	object				scriptInstances[MAX_CLIENTS];
	object				botClassInstance;

	int					clientNum;
	int					entityNum;
	int					botType;
	idCmdArgs			cmdArgs;
	botInfo_s() {
		brain = NULL;
		botName = "";
		authorName = "";
		clientNum = -1;
		entityNum = -1;
		cmdArgs.Clear();
		botType = CODE;
	}

	~botInfo_s() {
		brain = NULL;
		botName = "";
		authorName = "";
		clientNum = -1;
		entityNum = -1;
	}
} botInfo_t;

//Worker thread class currently responsible for running the update tasks each frame for the bots
//Unfortunately due to the Python GIL the benefit of this class is somewhat reduced since python code
//will never be executing on more than one thread at a time.
class botWorkerThread {
	friend class afiBotManager;

public:
	botWorkerThread(condition_variable* conditional_variable,mutex* thread_mutex,PyInterpreterState* mainState, unsigned int* initializeCounter );
	~botWorkerThread( );
	void						InitializeForFrame( unsigned int endUpdateIndex );
	void						RunWork( );
	void						AddUpdateTask( afiBotBrain* newTask );
protected:

private:
	bool						CheckWorkTime( );
	bool						LookForMoreWork( );
	void						RemoveFailedBot( int removeIndex );

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
	static void				PrintInfo( void );
	static void				Initialize( void );
	static void				Shutdown( void );
	static void				UpdateUserInfo( void );
	static void				ConsolePrint(const char* string);

	static void				Cmd_BotInfo_f( const idCmdArgs& args );
	static void				Cmd_AddBot_f( const idCmdArgs& args );
	static void				Cmd_RemoveBot_f( const idCmdArgs& args );
	static void				Cmd_RemoveAllBots_f( const idCmdArgs & args );
	static void				AddBot( const idCmdArgs& args );
	static void				DropABot( void );
	static void				RemoveBot( int clientNum );
	static int					IsClientBot( int clientNum );
	static void				SetBotDefNumber( int clientNum, int botDefNumber );
	static int					GetBotDefNumber( int clientNum );
	static idStr				GetBotClassname( int clientNum );
	static void				SpawnBot( int clientNum );
	static void				OnDisconnect( int clientNum );

	//Thread related functions
	static void				InitializeThreadsForFrame( );
	static void				LaunchThreadsForFrame( );
	static void				WaitForThreadsTimed(  );
	static bool				isGameEnding();
	static void				IncreaseThreadUpdateCount();
	static void				DecreaseThreadUpdateCount();
	static void				SetThreadState(PyThreadState* state,botWorkerThread* saveThread);
	static void				UpdateThreadState(PyThreadState* state);
	static void				SaveMainThreadState( );
	static void				RestoreMainThreadState( );

	static idEntity*			GetFlag(int team);
	static int					GetFlagStatus(int team);
	static void				ProcessChat(const char* text);
	static void				InitBotsFromMapRestart();
	static idCmdArgs *			GetPersistArgs( int clientNum );
	static usercmd_t *			GetUserCmd( int clientNum );
	static void				SetUserCmd( int clientNum, usercmd_t * usrCmd );
	static void				WriteUserCmdsToSnapshot(idBitMsg& msg);
	static void				ReadUserCmdsFromSnapshot(const idBitMsg& msg);
	static void				AddBotInfo(botInfo_t* newBotInfo);
	static afiBotBrain*		SpawnBrain(idStr botName, int clientNum);
	static botInfo_t*			FindBotProfile(idStr botName);
	static botInfo_t*			FindBotProfileByIndex(int clientNum);
	afiBotManager();
	~afiBotManager();

	static void					CleanUpPython();


protected:
	static usercmd_t			botCmds[MAX_CLIENTS];
private:
	static void					DistributeWorkToThreads( );
	static void					InitializePython( );

	static idList<botInfo_t*>	loadedBots;
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

//This custom call policy allows me to mitigate some of the losses in performance due to
//the python GIL. When Some c++ functions are exectued from python scripts we can give up control
//of the GIL for the duration of the function.
namespace boost { namespace python {
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

#endif
