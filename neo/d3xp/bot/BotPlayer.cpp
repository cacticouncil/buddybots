/*
===========================================================================
File: BotPlayer.cpp
Author: John Wileczek
Description: Fake client 'body' implementation of bot. Responsible for
translating input received from afiBotBrain to usercmd_t to be sent over
the network.
===========================================================================
*/
#include "precompiled.h"


#ifdef AFI_BOTS

#include "BotPlayer.h"


idCVar	bot_debugBot( "bot_debugBot", "-1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "debug a specific bot -1 disable, -2 all bots, otherwise clientnum", -2, MAX_CLIENTS );



CLASS_DECLARATION( idPlayer, afiBotPlayer )
END_CLASS


afiBotPlayer::afiBotPlayer() : idPlayer() {
	memset( &botcmd, 0, sizeof( botcmd ) );
	memset( &aiInput, 0, sizeof( aiInput ) );
	brain = NULL;
}

afiBotPlayer::~afiBotPlayer() {
}

void afiBotPlayer::SetAAS() {
	aas = gameLocal.GetAAS( "aas48" ); // TinMan: Hard coded the bots aas size
	if ( aas ) {
		const idAASSettings *settings = aas->GetSettings();
		if ( settings ) {
			// TinMan: *todo* why isn't physics bounds returning something nice?
			/*if ( !ValidForBounds( settings, physicsObject->GetBounds() ) ) {
				gameLocal.Error( "%s cannot use use_aas %s\n", name.c_str(), use_aas.c_str() );
			}*/
			float height = settings->maxStepHeight;
			physicsObj.SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}
	
	gameLocal.Error( "Bot cannot find AAS file for map\n" ); // TinMan: No aas, no play.
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

	userInfo->Set( "ui_team", spawnArgs.GetInt( "team" ) ? "Red" : "Blue" ); // ui_team is "Red"/"Blue" NOTE: may actually be "Blue"/"Red" need to check for that.

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", entityNumber ) ); // get engine to propagate userinfo changes

	afiBotManager::SetUserCmd( entityNumber, &botcmd );
}

void afiBotPlayer::PrepareForRestart( void ) {
	idPlayer::PrepareForRestart();

//	memset( &aiInput, 0, sizeof( aiInput ) );
	// TODO: botcmd?

}

void afiBotPlayer::Restart( void ) {
	idPlayer::Restart();

	BecomeActive( TH_THINK );
}

void afiBotPlayer::Think( void ) {
	idPlayer::Think();

	aiInput = brain->Think();

	ProcessInput();
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
	afiBotManager::SetUserCmd( entityNumber, &botcmd ); //Post usercmd so GetBotInput can do it's magic
}

void afiBotPlayer::UpdateViewAngles( void ) {

	//newViewAngles to usrcmd
	const idAngles & deltaViewAngles = GetDeltaViewAngles();
	idAngles newViewAngles;
	for ( int i = 0; i < 3; i++ ) {
		botcmd.angles[i] = ANGLE2SHORT( newViewAngles[i] - deltaViewAngles[i] );
	}
}

void afiBotPlayer::ProcessMove( void ) {
	//Grab vectors of viewangles for movement
	idVec3 forward, right;
	idAngles angles( 0, viewAngles.yaw , NULL );
	angles.ToVectors( &forward, &right, NULL );

	//aiInput.moveSpeed = pm_speed.GetFloat(); //the speed scaling below relies on speed being set each frame else it will whittle it down to 0, since the navigation state doesn't seem to be using fine grained speed control (human players don't exactly have it anyway) just using max.
	float inspeed = aiInput.moveSpeed;
	int maxSpeed = 160.0f;//pm_speed.GetFloat(); //this may not work for speed? TEST
	aiInput.moveSpeed = idMath::ClampFloat( -maxSpeed, maxSpeed, aiInput.moveSpeed );
	aiInput.moveSpeed = aiInput.moveSpeed * 127 / maxSpeed; //Scale from [0, 400] to [0, 127]


	aiInput.moveDirection.z = 0; //normalize can be smaller than wanted with a very large z value (like walk off ledge)
	aiInput.moveDirection.Normalize();

	botcmd.forwardmove = ( forward * aiInput.moveDirection ) * aiInput.moveSpeed;
	botcmd.rightmove	= ( right * aiInput.moveDirection ) * aiInput.moveSpeed;		
	botcmd.upmove		= abs( forward.z ) * aiInput.moveDirection.z * aiInput.moveSpeed;
	// MoveFlags
	aiMoveFlag_t moveFlag = aiInput.moveFlag;
	if ( moveFlag == JUMP ) {
		botcmd.upmove += 127.0f;
	} 
	if ( moveFlag == CROUCH ) {
		botcmd.upmove -= 127.0f;
	}

	//if ( moveFlag == RUN ) { // hard code for time being
		botcmd.buttons |= BUTTON_RUN;
	//}
}

void afiBotPlayer::ProcessCommands( void ) {	
	aiCommands_t * commands = &aiInput.commands;

	//Throw in buttons
	if ( commands->attack ) { 
		botcmd.buttons |= BUTTON_ATTACK;
	}
	
	if( commands->zoom ) {
		botcmd.buttons |= BUTTON_ZOOM;
	}
}

idEntity* afiBotPlayer::FindNearestItem( idStr item )
{
	idEntity* entity;
	for (int i = 0; i < MAX_GENTITIES; ++i)
	{
		entity = gameLocal.entities[i];

		if (entity)
		{
			
			if ((entity->IsType(idItem::Type)) || (entity->IsType(idItemPowerup::Type)))
			{
				if (idStr::FindText(entity->name,item.c_str(), false) == 0)
				{
					return entity;
				}
			}
		}
	}
	return 0;
}



#endif