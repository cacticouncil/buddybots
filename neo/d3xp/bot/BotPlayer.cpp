#include "precompiled.h"
#pragma hdrstop

#ifdef AFI_BOTS

#include "BotPlayer.h"

idCVar	bot_debugBot( "bot_debugBot", "-1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "debug a specific bot -1 disable, -2 all bots, otherwise clientnum", -2, MAX_CLIENTS );



CLASS_DECLARATION( idPlayer, afiBotPlayer )
END_CLASS

afiBotPlayer::afiBotPlayer() : idPlayer() {
	memset( &botcmd, 0, sizeof( botcmd ) );
	memset( &aiInput, 0, sizeof( aiInput ) );
}

afiBotPlayer::~afiBotPlayer() {
}


void afiBotPlayer::Spawn() {
	if ( gameLocal.isClient ) {
		return;
	}

	// Set user info
	idDict* userInfo = GetUserInfo();
	const idKeyValue* arg = NULL;
	arg = spawnArgs.MatchPrefix( "ui_", arg );
	while ( arg ) {
		userInfo->Set( arg->GetKey().c_str(), arg->GetValue().c_str() );
		arg = spawnArgs.MatchPrefix( "ui_", arg );
	}

	if ( idStr::Length( userInfo->GetString( "ui_name" ) ) < 1 ) {
		userInfo->Set( "ui_name", va( "bot%d", entityNumber ) );
	}

	userInfo->Set( "ui_team", spawnArgs.GetInt( "team" ) ? "Red" : "Blue" ); // TinMan: team is 0/1, ui_team is "Marine"/"Strogg"

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", entityNumber ) ); // get engine to propagate userinfo changes


	afiBotManager::SetUserCmd( entityNumber, &botcmd );
}

void afiBotPlayer::PrepareForRestart( void ) {
	idPlayer::PrepareForRestart();

	memset( &aiInput, 0, sizeof( aiInput ) );
	// TODO: botcmd?

}

void afiBotPlayer::Restart( void ) {
	idPlayer::Restart();

	BecomeActive( TH_THINK );
}

void afiBotPlayer::Think( void ) {
	idPlayer::Think();
}

void afiBotPlayer::ClearInput( void ) {
	//aiInput.moveDirection.Zero();
	//aiInput.moveSpeed = 0.f;
	//aiInput.viewDirection.Zero();
	memset( &aiInput.commands, 0, sizeof( aiCommands_t ) );
	memset( &aiInput.moveFlag, 0, sizeof( aiMoveFlag_t ) );
}

void afiBotPlayer::ProcessInput( void ) {
	// Clear Old Output
	memset( &botcmd, 0, sizeof( botcmd ) );

	UpdateViewAngles();
	ProcessMove();
	ProcessCommands();
	afiBotManager::SetUserCmd( entityNumber, &botcmd ); // TinMan: Post usercmd so GetBotInput can do it's magic
}

void afiBotPlayer::UpdateViewAngles( void ) {

	// TinMan: newViewAngles to usrcmd
	const idAngles & deltaViewAngles = GetDeltaViewAngles();
	idAngles newViewAngles;
	for ( int i = 0; i < 3; i++ ) {
		botcmd.angles[i] = ANGLE2SHORT( newViewAngles[i] - deltaViewAngles[i] );
	}
}

void afiBotPlayer::ProcessMove( void ) {
	// TinMan: Grab vectors of viewangles for movement
	idVec3 forward, right;
	idAngles angles( 0, viewAngles.yaw , NULL );
	angles.ToVectors( &forward, &right, NULL );

	//aiInput.moveSpeed = pm_speed.GetFloat(); // TinMan: *todo* the speed scaling below relies on speed being set each frame else it will whittle it down to 0, since the navigation state doesn't seem to be using fine grained speed control (human players don't exactly have it anyway) just using max.
	float inspeed = aiInput.moveSpeed;
	int maxSpeed = 0;//pm_speed.GetFloat(); // custom TODO: this may not work for speed? TEST
	aiInput.moveSpeed = idMath::ClampFloat( -maxSpeed, maxSpeed, aiInput.moveSpeed );
	aiInput.moveSpeed = aiInput.moveSpeed * 127 / maxSpeed; // TinMan: Scale from [0, 400] to [0, 127]


	aiInput.moveDirection.z = 0; // custom: normalize can be smaller than wanted with a very large z value (like walk off ledge)
	aiInput.moveDirection.Normalize();

	botcmd.forwardmove = ( forward * aiInput.moveDirection ) * aiInput.moveSpeed;
	botcmd.rightmove	= ( right * aiInput.moveDirection ) * aiInput.moveSpeed;		
	botcmd.upmove		= abs( forward.z ) * aiInput.moveDirection.z * aiInput.moveSpeed;
	aiInput.moveSpeed = inspeed;
	// MoveFlags
	aiMoveFlag_t moveFlag = aiInput.moveFlag;
	if ( moveFlag == JUMP ) {
		botcmd.upmove += 127.0f;
	} 
	if ( moveFlag == CROUCH ) {
		botcmd.upmove -= 127.0f;
	}

	//if ( moveFlag == RUN ) { // TinMan: *todo* hard code for time being
		botcmd.buttons |= BUTTON_RUN;
	//}
}

void afiBotPlayer::ProcessCommands( void ) {	
	aiCommands_t * commands = &aiInput.commands;

	// TinMan: Throw in buttons
	if ( commands->attack ) { // TinMan: Bang bang, you're dead. No! I shot you first!
		botcmd.buttons |= BUTTON_ATTACK;
	}
}

#endif