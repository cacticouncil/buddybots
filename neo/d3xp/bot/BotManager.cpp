#include "precompiled.h"
#pragma hdrstop

#ifdef AFI_BOTS

#include "BotManager.h"

afiBotManager	afiBotManagerLocal;
afiBotManager *	BotManager = &afiBotManagerLocal;


int				afiBotManager::numQueBots = 0;
int				afiBotManager::botEntityDefNumber[MAX_CLIENTS];
bool			afiBotManager::botSpawned[MAX_CLIENTS];
idCmdArgs		afiBotManager::cmdQue[MAX_CLIENTS];
idCmdArgs		afiBotManager::persistArgs[MAX_CLIENTS];
usercmd_t		afiBotManager::botCmds[MAX_CLIENTS];



void afiBotManager::PrintInfo( void ) {
	common->Printf("AFI Bots Initialized\n");
}

void afiBotManager::Initialize( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		cmdQue[i].Clear();
		persistArgs[i].Clear();
		botSpawned[i] = false;
		botEntityDefNumber[i] = 0;
	}
	memset( &botCmds, 0, sizeof( botCmds ) );
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

	// TinMan: Start fake client connect
	int clientNum = networkSystem->ServerConnectBot();
	if ( clientNum == -1 ) {
		gameLocal.Printf( "No available slot for bot.\n" );
		return;
	}

	assert( clientNum >= 0 );

	persistArgs[clientNum] = args; // Add args to persist args, used for spawnbot/persistance over map change, which remind me I should check the weather tomorrow
	botSpawned[clientNum] = true;

	// Index num of the bots def is saved so it can be sent to clients in order to spawn the right bot class
	SetBotDefNumber( clientNum, botDef->Index() );

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

void afiBotManager::DropABot( void ) {
	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "DropABot can only be used in a multiplayer game\n" );
		return;
	}

	if ( gameLocal.isClient ) {
		gameLocal.Printf( "You have no such power. This is a server command\n" );
		return;
	}

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

	gameLocal.DPrintf( "SpawnBot: %i\n", clientNum );

	classname = GetBotClassname( clientNum );
	if ( !classname.Length() ) {
		gameLocal.Warning( "Unknown classname '%s', defaulting to 'bot'.", classname );
		classname = "bot";
	}

	spawnDict.SetInt( "spawn_entnum", clientNum );
	spawnDict.Set( "name", va( "bot%d", clientNum ) );
	spawnDict.Set( "classname", classname ); 
	spawnDict.SetBool( "hide", false );

	const idDict* botEntityDict = gameLocal.FindEntityDefDict( classname, false );
	if ( !botEntityDict ) {
		if ( classname ) {
			gameLocal.Error( "Unknown classname '%s'.", classname );
		}
		return;
	}

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

	idEntity *ent;
	if ( !gameLocal.SpawnEntityDef( spawnDict, &ent ) || !gameLocal.entities[ clientNum ] ) {
		gameLocal.Error( "Failed to spawn Bot as '%s'", spawnDict.GetString( "classname" ) );
	}
	
	if ( !ent->IsType( idPlayer::Type ) ){
		gameLocal.Error( "'%s' spawned the bot as a '%s'.  Bot spawnclass must be a subclass of idPlayer.", spawnDict.GetString( "classname" ), ent->GetClassname() );
	}

	if ( clientNum >= gameLocal.numClients ) {
		gameLocal.numClients = clientNum + 1; 
	}

	gameLocal.mpGame.SpawnPlayer( clientNum );
}

/*
===================
afiBotManager::OnDisconnect
===================
*/
void afiBotManager::OnDisconnect( int clientNum ) {
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );
	//botSpawned[ clientNum ] = false;
	memset( &botCmds[ clientNum ], 0, sizeof( usercmd_t ) );
	botEntityDefNumber[ clientNum ] = -1;

	//persistArgs[ clientNum ].Clear();
}


/*
===================
afiBotManager::InitBotsFromMapRestart
Connect all qued bots to the game or refresh current bots
===================
*/
void afiBotManager::InitBotsFromMapRestart( void ) {
	if ( !gameLocal.isServer ) return;

	// TinMan: 1.4.2 fakeclient gives us a bit of trouble, before we specificed which clientid we wanted, not botclientconnect gives us one, thus the seeming hack below
	// TinMan: grab current peristant args and rebuild bot que
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

#endif