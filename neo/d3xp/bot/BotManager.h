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

typedef afiBotBrain* (*CreateBotBrain_t)(botImport_t* dllSetup);

enum	BotType { CODE,SCRIPT,DLL };

typedef struct botInfo_s {
	idStr				botName;
	idStr				authorName;
	afiBotBrain*		brain;
	object				botClassInstance;
	object				scriptInstances[MAX_CLIENTS];
	
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

class botWorkerThread {
	friend class afiBotManager;

public:
								botWorkerThread(condition_variable* conditional_variable,mutex* thread_mutex );
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
	condition_variable*			threadConditional;
	mutex*						threadMutex;
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

protected:
	static usercmd_t			botCmds[MAX_CLIENTS];
private:
	static void					DistributeWorkToThreads( );
	
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
	static atomic<bool>			gameEnd;

	static unsigned int			threadUpdateCount;
	


	static idCmdArgs			cmdQue[MAX_CLIENTS];
	static idCmdArgs			persistArgs[MAX_CLIENTS];
	static bool					botSpawned[MAX_CLIENTS];
	static int					botEntityDefNumber[MAX_CLIENTS];
	static afiBotBrain*			brainFastList[MAX_CLIENTS];
};




#endif 
