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

#ifdef BUDDY_BOTS


// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

#include "BotPlayer.h"

idCVar	bot_debugBot( "bot_debugBot", "-1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "debug a specific bot -1 disable, -2 all bots, otherwise clientnum", -2, MAX_CLIENTS );

CLASS_DECLARATION( idPlayer, afiBotPlayer )
	END_CLASS

	void noOpDelete(afiBotPlayer*) { }
void noOpDeletePlayer(idPlayer*) {}

shared_ptr<afiBotPlayer> CreateBotPlayer() {

	return shared_ptr<afiBotPlayer>(new afiBotPlayer(),&noOpDelete);
}



	BOOST_PYTHON_MODULE(afiBotPlayer) {
		//import("idVec3");
		//import("idAngles");
		//import("idEntity");
		//import("afiBotBrain");
		//import("afiBotManager");
		import("idPlayer");

		class_<afiBotPlayer,bases<idPlayer>,shared_ptr<afiBotPlayer>>("afiBotPlayer",no_init)
			.def("__init__",make_constructor(&CreateBotPlayer))
			.def("FindNearestItem",&afiBotPlayer::FindNearestItem,return_value_policy<reference_existing_object>(),release_gil_policy())
			.def("MoveTo",&afiBotPlayer::MoveTo)
			.def("MoveToPosition",&afiBotPlayer::MoveToPosition)
			.def("MoveToEntity",&afiBotPlayer::MoveToEntity)
			.def("MoveToPlayer",&afiBotPlayer::MoveToPlayer)
			.def("Attack",&afiBotPlayer::Attack)
			.def("Jump",&afiBotPlayer::Jump)
			.def("LookInDirection",&afiBotPlayer::LookInDirection)
			.def("LookAtPosition",&afiBotPlayer::LookAtPosition)
			.def("MoveToNearest",&afiBotPlayer::MoveToNearest,return_value_policy<reference_existing_object>())
			.def("PathToGoal",&afiBotPlayer::PathToGoal)
			.def("ReachedPos",&afiBotPlayer::ReachedPos,release_gil_policy())
			.def("SwitchWeapon",&afiBotPlayer::SwitchWeapon)
			.def("HasAmmo",&afiBotPlayer::HasAmmo)
			.def("FindNearbyPlayers",&afiBotPlayer::FindNearbyPlayers)
			.def("GetPosition",&afiBotPlayer::GetPosition)
			.def_readonly("health",&afiBotPlayer::health)
			.def_readonly("team",&afiBotPlayer::team)
			.def_readonly("spectator",&afiBotPlayer::spectator)
			;
}



afiBotPlayer::afiBotPlayer() : idPlayer() {
	memset( &botcmd, 0, sizeof( botcmd ) );
	memset( &aiInput, 0, sizeof( aiInput ) );
	//Oh if I only had a brain
	brain = nullptr;
}

afiBotPlayer::~afiBotPlayer() {
	brain = nullptr;
	aas = NULL;
}

void afiBotPlayer::SetBrain(afiBotBrain* newBrain) {
	brain = newBrain;
}

afiBotBrain* afiBotPlayer::GetBrain() const {
	return brain;
}

void afiBotPlayer::SetAAS() {
	aas = gameLocal.GetAAS( "aas48" );
	if ( aas ) {
		const idAASSettings *settings = aas->GetSettings();
		if ( settings ) {
			float height = settings->maxStepHeight;
			physicsObj.SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}

	gameLocal.Error( "Bot cannot find AAS file for map\n" ); // TinMan: No aas, no play.
}


boost::python::list afiBotPlayer::FindNearbyPlayers() {

	boost::python::list nearbyPlayers = boost::python::list();
	unsigned int numClients = gameLocal.numClients;
	for (unsigned int iClient = 0; iClient < numClients; ++iClient) {

		idPlayer* player = gameLocal.GetClientByNum(iClient);
		if (player != nullptr) {
			if (CanSee(player, true)) {
				nearbyPlayers.append(shared_ptr<idPlayer>(player,&noOpDeletePlayer));
			}
		}
	}

	return nearbyPlayers;
}
bool afiBotPlayer::SwitchWeapon(const char* weaponName) {

	int weaponSlot = SlotForWeapon(weaponName);

	if (weaponSlot == -1) {
		return false;
	}

	SelectWeapon(weaponSlot, false);

	if (idealWeapon != weaponSlot)
		return false;

		
	return true;


}

int afiBotPlayer::HasAmmo(const char* weaponName) {

	int weaponSlot = SlotForWeapon(weaponName);

	if (weaponSlot == -1) {
		return 0;
	}

	return inventory.HasAmmo(weaponName, true, this);

}

void afiBotPlayer::SpawnFromSpawnSpot() {
	idVec3		spawn_origin;
	idAngles	spawn_angles;

	SelectInitialSpawnPoint(spawn_origin, spawn_angles);
	SpawnToPoint(spawn_origin, spawn_angles);

}

void afiBotPlayer::SpawnToPoint(const idVec3	&spawn_origin, const idAngles &spawn_angles) {
	idVec3 spec_origin;

	assert(!gameLocal.isClient);

	respawning = true;

	Init();

	fl.noknockback = false;

	// stop any ragdolls being used
	StopRagdoll();

	// set back the player physics
	SetPhysics(&physicsObj);

	physicsObj.SetClipModelAxis();
	physicsObj.EnableClip();

	if (!spectating) {
		SetCombatContents(true);
	}

	physicsObj.SetLinearVelocity(vec3_origin);

	// setup our initial view
	if (!spectating) {
		SetOrigin(spawn_origin);
	}
	else {
		spec_origin = spawn_origin;
		spec_origin[2] += pm_normalheight.GetFloat();
		spec_origin[2] += SPECTATE_RAISE;
		SetOrigin(spec_origin);
	}

	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	SetDeltaViewAngles(ang_zero);
	SetViewAngles(spawn_angles);
	spawnAngles = spawn_angles;
	spawnAnglesSet = false;

	legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if (spectating) {
		Hide();
	}
	else {
		Show();
	}

	if (gameLocal.isMultiplayer) {
		if (!spectating) {
			// we may be called twice in a row in some situations. avoid a double fx and 'fly to the roof'
			if (lastTeleFX < gameLocal.time - 1000) {
				idEntityFx::StartFx(spawnArgs.GetString("fx_spawn"), &spawn_origin, NULL, this, true);
				lastTeleFX = gameLocal.time;
			}
		}
		AI_TELEPORT = true;
	}
	else {
		AI_TELEPORT = false;
	}

	// kill anything at the new position
	if (!spectating) {
		physicsObj.SetClipMask(MASK_PLAYERSOLID); // the clip mask is usually maintained in Move(), but KillBox requires it
		gameLocal.KillBox(this);
	}

	// don't allow full run speed for a bit
	physicsObj.SetKnockBack(100);

	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	minRespawnTime = gameLocal.time;
	maxRespawnTime = gameLocal.time;

	if (forceRespawn && !spectating) {
		brain->OnRespawn();
	}

	if (!spectating) {
		forceRespawn = false;
	}

	privateCameraView = NULL;

	BecomeActive(TH_THINK);

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	idPlayer::Think();

	respawning = false;
	lastManOver = false;
	lastManPlayAgain = false;
	isTelefragged = false;


}

void afiBotPlayer::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location) {

	if (gameLocal.isClient) {
		return;
	}
	int			damage;
	int			armorSave;
	idPlayer::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(damageDefName, false);
	if (!damageDef) {
		gameLocal.Warning("Unknown damageDef '%s'", damageDefName);
		return;
	}

	if (damageDef->dict.GetBool("ignore_player")) {
		return;
	}

	if (inflictor == nullptr) {
		inflictor = gameLocal.world;
	}

	if (attacker == nullptr) {
		attacker = gameLocal.world;
	}

	CalcDamagePoints(inflictor, attacker, &damageDef->dict, damageScale, location, &damage, &armorSave);
	//POSSIBLE_CHANGE: If we want to send out events even if no damage is dealt in the future this
	//is where it would be done.
	if (damage == 0) {
		return;
	}
	brain->OnPain(inflictor, attacker, dir, damage);

	if (attacker->IsType(afiBotPlayer::Type)) {
		afiBotPlayer* attackingBot = (afiBotPlayer*)attacker;
		attackingBot->GetBrain()->OnHit(attackingBot, dir, damage);
	}

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

	if (thinkFlags & TH_THINK) {
		Move();

		aiInput.moveDirection = move.moveDir;
		aiInput.moveSpeed = move.speed;
		ProcessInput();
	}
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
	idAngles 	idealViewAngles;
	float		arcOffset = 20.0f;
	float		arcHalf = 0.0f;
	float		arcDistance;
	idVec3		lastLookAtPosition;
	idVec3		viewPos; //temphackan
	idAngles 	newViewAngles;
	//aiInput.viewDirection.Normalize();
	idVec3 eyePosition = GetEyePosition();
	assert( aiInput.viewType >= VIEW_DIR && aiInput.viewType <= VIEW_POS );
	if ( aiInput.viewType == VIEW_DIR ) {
		idealViewAngles = aiInput.viewDirection.ToAngles();
		viewPos = aiInput.viewDirection + eyePosition;
	} else if ( aiInput.viewType == VIEW_POS ) {
		idealViewAngles = (aiInput.viewDirection - eyePosition).ToAngles();
		viewPos = aiInput.viewDirection;
	} else {
		gameLocal.Warning( "BotPlayer::UpdateViewAngles - Unknown viewType" );
		idealViewAngles.Zero();
	}

	idealViewAngles = idVec3(viewPos - eyePosition).ToAngles();

	idAngles delta = ( idealViewAngles - viewAngles ).Normalize180();
	arcDistance = idMath::Sqrt( Square( delta.yaw ) + Square( delta.pitch ) );

	idAngles deltaAngles = ( idealViewAngles - lastViewAngles ).Normalize180();
	float deltaArcDistance = idMath::Sqrt( Square( deltaAngles.yaw ) + Square( deltaAngles.pitch ) );
	if ( deltaArcDistance > 20.0f || arcDistance > arcHalf * 2 ) {
		arcHalf = arcDistance / 2;
	}

	lastViewAngles = idealViewAngles;

	aimRate = idMath::ClampFloat( 0.1f, 1.0f, aimRate );

	// Calculate Acceleration or Deceleration
	if ( arcDistance <= arcHalf ) { // if arcDistance is less than half way point, decelerate to target normally
		newViewAngles = viewAngles + delta * aimRate;
	} else if ( arcDistance > 0.0f ) { // if arcDistance is greater than half way point, accelerate to target
		// Arc distances get reversed relative to initial arcDistance plus the offset,
		//     which gives initial movement some velocity.
		//     Then the crosshair accelerates to the arcHalf point.
		//     The change in angle is calculated from the scaled aimRate.
		// gd: old version delta = delta * ( aimRate * arcHalf / arcDistance * ( 200.0f - arcDistance ) / ( 200.0f - arcHalf ) );
		delta = delta * ( aimRate * arcHalf / arcDistance * ( arcHalf * 2 + arcOffset - arcDistance ) / ( arcHalf + arcOffset ) );
		delta.roll = delta.roll * aimRate; // needed if, after ragdoll, roll might be 45 degrees still
		newViewAngles = viewAngles + delta;
	}

	//newViewAngles to usrcmd
	const idAngles & deltaViewAngles = GetDeltaViewAngles();
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
	//float inspeed = aiInput.moveSpeed;
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

	if ( moveFlag == RUN ) {
		botcmd.buttons |= BUTTON_RUN;
	}

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

/*
===================
BotPlayer::Attack
===================
*/
void afiBotPlayer::Attack( void ) {
	aiInput.commands.attack = true;
}

/*
=====================
afiBotPlayer::Jump
=====================
*/
void afiBotPlayer::Jump( void ) {
	aiInput.moveFlag = JUMP;
}

/*
=====================
afiBotPlayer::LookInDirection
=====================
*/
void afiBotPlayer::LookInDirection( const idVec3 &dir ) {
	aiInput.viewDirection = dir;
	aiInput.viewType = VIEW_DIR;
}

/*
=====================
afiBotPlayer::LookAtPosition
=====================
*/
void afiBotPlayer::LookAtPosition( const idVec3 &pos ) {
	aiInput.viewDirection = pos;
	aiInput.viewType = VIEW_POS;
}

#endif