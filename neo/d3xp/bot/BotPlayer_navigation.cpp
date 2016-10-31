#include "BotPlayer.h"
#include "Mover.h"
#include "../Entity.h"
#include "gamesys/SysCvar.h"

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;
const float PM_AIRFRICTION		= 0.0f;

const float OVERCLIP			= 1.001f;
const int MAX_FRAME_SLIDE		= 5;

typedef struct pathTrace_s {
	float					fraction;
	idVec3					endPos;
	idVec3					normal;
	const idEntity *		blockingEntity;
} pathTrace_t;

botMoveState_t::botMoveState_t() {
	//	moveType			= MOVETYPE_ANIM;
	moveCommand			= MOVE_NONE;
	moveStatus			= MOVE_STATUS_DONE;
	moveDest.Zero();
	moveDir.Set( 1.0f, 0.0f, 0.0f );
	goalEntity			= NULL;
	goalEntityOrigin.Zero();
	toAreaNum			= 0;
	startTime			= 0;
	duration			= 0;
	speed				= 0.0f;
	range				= 0.0f;
	wanderYaw			= 0;
	nextWanderTime		= 0;
	blockTime			= 0;
	obstacle			= NULL;
	lastMoveOrigin		= vec3_origin;
	lastMoveTime		= 0;
	anim				= 0;
	travelFlags			= TFL_WALK|TFL_AIR|TFL_WALKOFFLEDGE|TFL_BARRIERJUMP|TFL_ELEVATOR|TFL_TELEPORT|TFL_SPECIAL;
	kickForce			= 2048.0f;
	flags.ignoreObstacles	= false;

	blockedRadius		= 0.0f;
	blockedMoveTime		= 750;
	blockedAttackTime	= 750;

	current_yaw			= 0.0f;

	seekPos.Zero();
	addVelocity.Zero();
	path.moveAreaNum = 0;
	path.moveGoal = vec3_zero;
	path.reachability = NULL;
	path.secondaryGoal = vec3_zero;
	path.type = -1;
	lastPath.moveAreaNum = 0;
	lastPath.moveGoal = vec3_zero;
	lastPath.reachability = NULL;
	lastPath.secondaryGoal = vec3_zero;
	lastPath.type = -1;
}

void afiBotPlayer::MoveToAttackPosition(idEntity* entity) {

	//int				attack_anim;
	//int				areaNum;
	//aasObstacle_t	obstacle;
	//aasGoal_t		goal;
	//idBounds		bounds;
	//idVec3			pos;

	//if (!aas || !entity) {
	//	StopMove(MOVE_STATUS_DEST_UNREACHABLE);
	//	AI_DEST_UNREACHABLE = true;
	//	return false;
	//}

	//const idVec3 &org = physicsObj.GetOrigin();
	//areaNum = PointReachableAreaNum(org);

	//attack_anim = GetAnimChannel
	//// consider the entity the monster is getting close to as an obstacle
	//obstacle.absBounds = entity->GetPhysics()->GetAbsBounds();

	//
	//pos = entity->GetPhysics()->GetOrigin();
	//

	//idAASFindAttackPosition findGoal(this, physicsObj.GetGravityAxis(), entity, pos, missileLaunchOffset[attack_anim]);
	//if (!aas->FindNearestGoal(goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal)) {
	//	StopMove(MOVE_STATUS_DEST_UNREACHABLE);
	//	AI_DEST_UNREACHABLE = true;
	//	return false;
	//}

	//move.moveDest = goal.origin;
	//move.toAreaNum = goal.areaNum;
	//move.goalEntity = ent;
	//move.moveCommand = MOVE_TO_ATTACK_POSITION;
	//move.moveStatus = MOVE_STATUS_MOVING;
	//move.speed = fly_speed;
	//move.startTime = gameLocal.time;
	//move.anim = attack_anim;
	//AI_MOVE_DONE = false;
	//AI_DEST_UNREACHABLE = false;
	//AI_FORWARD = true;
}

/*
===================
BotPlayer::MoveToPlayer
uses player prediction right now to move more intelligently to the player
===================
*/
bool afiBotPlayer::MoveToPlayer( idPlayer *player )
{
	assert( player );
	predictedPath_t path;

	afiBotPlayer::PredictPath( player, aas, player->GetPhysics()->GetOrigin(), player->GetPhysics()->GetLinearVelocity(), 1000, 100, SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA, path );
	if ( !MoveToPosition( path.endPos, player->GetPhysics()->GetBounds().GetRadius() ) ) {
		gameLocal.Warning( "bot cannot MoveToPlayer at %s", path.endPos.ToString() );
		return false;
	}
	/*
	if ( CanSee( player, true ) ) {
	LookAt( player->GetEyePosition() ); // TODO: sb - check if CanSee and relook to move.goalPos if not
	} else {
	// goalPos is the path smoothing cutoff
	LookAt( move.goalPos );
	}
	*/
	return true;
}
/*
===================
BotPlayer::MoveToEntity
===================
*/
bool afiBotPlayer::MoveToEntity( idEntity* entity )
{
	return MoveToPosition( entity->GetPhysics()->GetOrigin(), entity->GetPhysics()->GetBounds().GetRadius()*0.25f );
}
/*
===================
BotPlayer::MoveToNearest
===================
*/
idEntity* afiBotPlayer::MoveToNearest( idStr item )
{
	idEntity* nearest = FindNearestItem( item );

	if ( nearest )
	{
		MoveToEntity( nearest );
	}
	return nearest;
}

/*
=====================
BotPlayer::StartMove
Initialize a new movement by setting up the movement structure
// TODO: this would be distributed in the FSM?
=====================
*/
bool afiBotPlayer::StartMove (const idVec3& goalOrigin, int goalArea, idEntity* goalEntity, float range ) {
	// If we are already there then we are done
	if ( ReachedPos( goalOrigin) ) {
		StopMove( MOVE_STATUS_DONE );
		return true;
	}

	move.lastMoveOrigin		= GetPhysics()->GetOrigin ( );

	move.seekPos = move.goalPos = move.moveDest	= goalOrigin;
	move.toAreaNum			= goalArea;
	move.goalEntity			= goalEntity;
	move.moveStatus			= MOVE_STATUS_MOVING;
	move.speed				= pm_walkspeed.GetFloat();	//400.0f;
	move.startTime			= gameLocal.time;
	move.range				= range;

	move.flags.done			= false;
	move.flags.goalUnreachable	= false;
	move.flags.moving			= true;

	//	TODO: cusTom3 - these are going to be fun
	// aasSensor->Reserve ( feature );

	//	TODO: OnStartMoving ( ); FSM ?

	return true;
}

/*
=====================
BotPlayer::StopMove
=====================
*/
void afiBotPlayer::StopMove( moveStatus_t status ) {
	//moveCommand_t oldCommand = move.moveCommand;

	move.flags.done			= true;
	move.flags.moving			= false;
	move.flags.goalUnreachable	= false;
	move.flags.obstacleInPath	= false;
	move.flags.blocked			= false;
	move.moveCommand		= MOVE_NONE;
	move.moveStatus			= status;
	move.toAreaNum			= 0;
	move.goalEntity			= NULL;
	move.moveDest			= GetPhysics()->GetOrigin();
	move.startTime			= gameLocal.time;
	move.duration			= 0;
	move.range				= 0.0f;
	move.speed				= 0.0f;
	move.anim				= 0;
	move.moveDir.Zero();
	move.lastMoveOrigin.Zero();
}

/*
=====================
BotPlayer::ReachedPos
=====================
*/
bool afiBotPlayer::ReachedPos( const idVec3 &pos, float range ) const {
	// When moving towards the enemy just see if our bounding box touches the desination

	idBounds bnds;
	bnds = idBounds ( idVec3(-range,-range,-16.0f), idVec3(range,range,64.0f) );
	bnds.TranslateSelf( GetPhysics()->GetOrigin() );
	return bnds.ContainsPoint( pos );
}

/*
=====================
BotPlayer::MoveToPosition
=====================
*/
bool afiBotPlayer::MoveToPosition ( const idVec3 &pos, float range ) {
	int areaNum = PointReachableAreaNum( GetPhysics()->GetOrigin() );
	if ( !areaNum ) {
		return false;
	}

	idVec3 goal = pos;
	int goalAreaNum = PointReachableAreaNum( goal );
	if ( !goalAreaNum ) {
		return false;
	}
	aas->PushPointIntoAreaNum( goalAreaNum, goal );

	if ( !PathToGoal( move.path, areaNum, GetPhysics()->GetOrigin(), goalAreaNum, goal ) ) {
		return false;
	}

	// Start moving
	return StartMove( goal, goalAreaNum, NULL, range );
}
/*
=====================
BotPlayer::MoveTo
=====================
*/
void afiBotPlayer::MoveTo ( const idVec3 &pos, float speed ) {
	move.moveDir = pos - GetPhysics()->GetOrigin() ;
	move.speed = speed;
}

/*
=====================
BotPlayer::Move
// TODO: this goes away when distributed to the goal system
=====================
*/
void afiBotPlayer::Move( void ) {
	idReachability*	goalReach;

	move.obstacle = NULL;
	if ( ReachedPos( move.moveDest, move.range ) ) {
		StopMove( MOVE_STATUS_DONE );
	} else {
		move.moveStatus = MOVE_STATUS_MOVING;

		// Otherwise, Update The Seek Pos
		if ( GetMovePos( move.goalPos, &goalReach ) ) {
			// TODO: look through obstacle avoidance and enable
			CheckObstacleAvoidance( move.goalPos, move.seekPos, goalReach );
			//move.seekPos = move.goalPos;
			move.moveDir = move.seekPos - GetPhysics()->GetOrigin();
		} else if ( move.moveStatus != MOVE_STATUS_DONE ) {
			gameLocal.Warning( "bot destination unreachable in Move" );
			if ( ai_debugMove.GetBool() ) {
				gameRenderWorld->DebugSphere( colorPink, idSphere( move.moveDest, 48 ) );
			}
			//if ( move.goalPos.x > GetPosition().x ) {
			//	MoveToPosition( GetPosition() + idVec3( -50, 0, 0 ), 8 );
			//}
			//else if (move.goalPos.x < GetPosition().x) {
			//	MoveToPosition(GetPosition() + idVec3(50, 0, 0), 8);
			//}
			StopMove( MOVE_STATUS_DEST_UNREACHABLE );
		}
	}

	BlockedFailSafe();

	// TODO: cusTom3 - look at new DebugFilter stuff
	if ( ai_debugMove.GetBool() ) { // AnimMove : GREEN / RED Bounds & Move Dest
		//	gameRenderWorld->DebugLine(		colorCyan, oldorigin, org, 5000 );
		gameRenderWorld->DebugBounds( colorRed, GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), gameLocal.msec );
		if (!ReachedPos( move.moveDest, move.range )) {
			gameRenderWorld->DebugBounds( colorRed, GetPhysics()->GetBounds(), move.moveDest, gameLocal.msec );
			// gameRenderWorld->DebugArrow( colorRed, org, move.moveDest, 4, gameLocal.msec );
			// TODO: team stuff - gameRenderWorld->DebugArrow(	(team==0)?(colorGreen):(colorRed), org, move.moveDest, 4, gameLocal.msec );
		}
		DrawRoute();
	}
}

/*
=====================
BotPlayer::PointReachableAreaNum
=====================
*/
int afiBotPlayer::PointReachableAreaNum( const idVec3 &pos ) const {
	return aas->PointReachableAreaNum( pos,  aas->GetSettings()->boundingBoxes[0], AREA_REACHABLE_WALK );
}

/*
=====================
BotPlayer::PathToGoal
=====================
*/
bool afiBotPlayer::PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const {
	idVec3 org;
	idVec3 goal;

	if ( !areaNum || !goalAreaNum) {
		return false;
	}

	org = origin;
	aas->PushPointIntoAreaNum( areaNum, org );

	goal = goalOrigin;

	if ( goal.z > GetEyePosition().z )  {
		idEntity *ent;
		for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next()) {
			// that is an elevator
			if (ent->IsType(idPlat::Type)) {
				idPlat *platform = static_cast<idPlat *>(ent);
				if (CanSee(platform, true)) {
					idClipModel *model = platform->GetPhysics()->GetClipModel();
					idBounds bounds = model->GetAbsBounds().Expand(0);
					idVec3 center;
					center = bounds.GetCenter();
					center.z = -127.75f;
					goal = center;
					break;
				}
			}
		}
	}

	aas->PushPointIntoAreaNum( goalAreaNum, goal );

	return aas->WalkPathToGoal( path, areaNum, org, goalAreaNum, goal, move.travelFlags );
}

/*
=====================
BotPlayer::GetMovePos
=====================
*/
bool afiBotPlayer::GetMovePos( idVec3 &seekPos, idReachability** seekReach ) {
	int			areaNum;
	aasPath_t	path;
	bool		result;
	idVec3		org;

	float delta = ( move.lastValidPos - GetPhysics()->GetOrigin() ).Length();
	if ( delta > 1 )
	{
		move.lastMoveTime = gameLocal.GetTime();
	}

	org = GetPhysics()->GetOrigin();
	seekPos = org;

	// RAVEN BEGIN
	// cdr: Alternate Routes Bug
	// TODO: cusTom3 - alternate routes
	if (seekReach) {
		(*seekReach) = 0;
	}
	// RAVEN END

	move.moveStatus = MOVE_STATUS_MOVING;
	result = false;

	if ( ReachedPos( move.moveDest, move.range ) ) {
		StopMove( MOVE_STATUS_DONE );
		seekPos	= org;
		return false;
	}

	if ( aas && move.toAreaNum ) {
		areaNum	= PointReachableAreaNum( org );
		if ( areaNum ) {
			move.lastValidPos = org;
			if ( PathToGoal( path, areaNum, org, move.toAreaNum, move.moveDest ) ) {
				move.lastPath = path;
				move.path = path;
				seekPos = path.moveGoal;

				// RAVEN BEGIN
				// cdr: Alternate Routes Bug
				if (seekReach) {
					(*seekReach) = (idReachability*)(path.reachability);
				}
				// RAVEN END

				result = true;
				//			move.nextWanderTime = 0;
			} else {
				move.flags.goalUnreachable = true;
			}
		} else if ( move.lastPath.moveAreaNum ) { // in an invalid area
			move.path = move.lastPath;
			seekPos = path.moveGoal;
			// RAVEN BEGIN
			// cdr: Alternate Routes Bug
			if (seekReach) {
				(*seekReach) = (idReachability*)(path.reachability);
			}
			// RAVEN END

			result = true;
		} else {
			move.flags.goalUnreachable = true;
		}
	}

	//if (  DebugFilter(ai_debugMove) ) { // YELLOW = seekPos
	//gameRenderWorld->DebugLine( colorYellow, GetPhysics()->GetOrigin(), seekPos );
	//}

	return result;
}

/*
=====================
BotPlayer::CheckObstacleAvoidance
=====================
*/
void afiBotPlayer::CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &seekPos, idReachability* goalReach ) {
	move.flags.blocked				= false;	// Makes Character Stop Moving
	move.flags.obstacleInPath		= false;	// Makes Character Walk
	move.obstacle				= NULL;
	seekPos						= goalPos;

	if (move.flags.ignoreObstacles) {
		return;
	}

	// Test For Path Around Obstacles
	//--------------------------------
	obstaclePath_t	path;
	move.flags.blocked				= !idAI::FindPathAroundObstacles( GetPhysics(), aas, /* move.moveCommand == MOVE_TO_ENEMY ? enemy.ent : */ NULL, physicsObj.GetOrigin(), goalPos, path ); // TinMan: changed from &physicsobj
	move.flags.obstacleInPath		= (path.firstObstacle || path.seekPosObstacle || path.startPosObstacle);
	move.obstacle				= (path.firstObstacle)?(path.firstObstacle):(path.seekPosObstacle);
	seekPos						= path.seekPos;

	// Don't Worry About Obstacles Out Of Walk Range
	//-----------------------------------------------
	idEntity* obs = move.obstacle.GetEntity();
	if (move.obstacle.GetEntity() && DistanceTo(obs)>155.0f) {
		move.flags.blocked			= false;
		move.flags.obstacleInPath	= false;
		move.obstacle			= 0;
		seekPos					= goalPos;
	}

	//if ( DebugFilter(ai_showObstacleAvoidance) ) {
	//gameRenderWorld->DebugLine( colorBlue, goalPos + idVec3( 1.0f, 1.0f, 0.0f ), goalPos + idVec3( 1.0f, 1.0f, 64.0f ), gameLocal.msec );
	//gameRenderWorld->DebugLine( !move.flags.blocked ? colorYellow : colorRed, path.seekPos, path.seekPos + idVec3( 0.0f, 0.0f, 64.0f ), gameLocal.msec );
	//}
}

/*
=====================
BotPlayer::BlockedFailSafe
TinMan: Check to see if you're just spinning your wheels
=====================
*/
void afiBotPlayer::BlockedFailSafe( void ) {
	if ( move.flags.blocked ) {
		return;
	}

	move.flags.blocked = false;

	if ( !ai_blockedFailSafe.GetBool() || move.blockedRadius < 0.0f ) {
		return;
	}
	if ( !AI_ONGROUND || ( physicsObj.GetOrigin() - move.lastMoveOrigin ).LengthSqr() > Square( move.blockedRadius ) ) {
		move.lastMoveOrigin = physicsObj.GetOrigin();
		move.lastMoveTime = gameLocal.time;
	}
	if ( move.lastMoveTime < gameLocal.time - move.blockedMoveTime ) {
		move.flags.blocked = true;
		move.lastMoveTime = gameLocal.time;
	}
}

/*
=====================
BotPlayer::DrawRoute
=====================
*/
void afiBotPlayer::DrawRoute( void ) const {
	idVec3 start = GetPhysics()->GetOrigin();
	if ( !( PointReachableAreaNum( start ) ) ) {
		start = move.lastValidPos;
	}
	aas->ShowWalkPath( start, move.toAreaNum, move.moveDest );
}

/*
==============
PlayerAccelerate

Handles user intended acceleration
==============
*/
void PlayerAccelerate( idVec3 &curVelocity, float frametime, const idVec3 &wishdir, const float wishspeed, const float accel ) {
#if 1
	// q2 style
	float addspeed, accelspeed, currentspeed;

	currentspeed = curVelocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	// RAVEN BEGIN
	// nmckenzie: added ability to try alternate accelerations.

	accelspeed = accel * frametime * wishspeed;
	// RAVEN END
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}
	// gameLocal.Printf( "accelspeed in Accelerate: %f\n", accelspeed );
	curVelocity += accelspeed * wishdir;

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	idVec3		wishVelocity;
	idVec3		pushDir;
	float		pushLen;
	float		canPush;

	wishVelocity = wishdir * wishspeed;
	pushDir = wishVelocity - current.velocity;
	pushLen = pushDir.Normalize();

	canPush = accel * frametime * wishspeed;
	if ( canPush > pushLen ) {
		canPush = pushLen;
	}

	curVelocity += canPush * pushDir;
#endif
}
/*
============
PlayerCmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float PlayerCmdScale( const usercmd_t &cmd ) {
	int		max;
	float	total;
	float	scale;
	int		forwardmove;
	int		rightmove;
	int		upmove;

	forwardmove = cmd.forwardmove;
	rightmove = cmd.rightmove;

	//// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	//// otherwise crouch speed will be lower than specified
	//if ( walking ) {
	upmove = 0;
	//} else {
	//	upmove = cmd.upmove;
	//}

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) 160.0f * max / ( 127.0f * total );

	return scale;
}

bool afiBotPlayer::PathTrace( idPlayer *player, const idAAS *aas, const idVec3 &start, const idVec3 &end, int stopEvent, struct pathTrace_s &trace, predictedPath_t &path, int drawDebug ) {
	// RAVEN BEGIN
	trace_t clipTrace;
	aasTrace_t aasTrace;

	memset( &trace, 0, sizeof( trace ) );

	if ( !aas || !aas->GetSettings() ) {
		// RAVEN BEGIN
		// nmckenzie: Added ignore ent
		// ddynerman: multiple clip worlds
		gameLocal.clip.TranslationEntities(clipTrace,start,end,player->GetPhysics()->GetClipModel(),
			player->GetPhysics()->GetClipModel()->GetAxis(),MASK_MONSTERSOLID,player);

		//gameLocal.Translation( player, clipTrace, start, end, player->GetPhysics()->GetClipModel(),
		//						player->GetPhysics()->GetClipModel()->GetAxis(), MASK_MONSTERSOLID, player );
		// RAVEN END

		// NOTE: could do (expensive) ledge detection here for when there is no AAS file

		trace.fraction = clipTrace.fraction;
		trace.endPos = clipTrace.endpos;
		trace.normal = clipTrace.c.normal;
		trace.blockingEntity = gameLocal.entities[ clipTrace.c.entityNum ];
	} else {
		aasTrace.getOutOfSolid = true;
		if ( stopEvent & SE_ENTER_LEDGE_AREA ) {
			aasTrace.flags |= AREA_LEDGE;
		}
		if ( stopEvent & SE_ENTER_OBSTACLE ) {
			aasTrace.travelFlags |= TFL_INVALID;
		}

		aas->Trace( aasTrace, start, end );

		// RAVEN BEGIN
		// nmckenzie: Added ignore ent.
		// TODO: cusTom3 - using predict path will use this which doesn't use MASK_PLAYERSOLID, look at the effect of not unioning the two.
		gameLocal.clip.TranslationEntities( clipTrace, start, aasTrace.endpos, player->GetPhysics()->GetClipModel(),
			player->GetPhysics()->GetClipModel()->GetAxis(), MASK_MONSTERSOLID, player );
		// RAVEN END

		if ( clipTrace.fraction >= 1.0f ) {
			trace.fraction = aasTrace.fraction;
			trace.endPos = aasTrace.endpos;
			trace.normal = aas->GetPlane( aasTrace.planeNum ).Normal();
			trace.blockingEntity = (idEntity*)gameLocal.world;

			if ( aasTrace.fraction < 1.0f ) {
				if ( stopEvent & SE_ENTER_LEDGE_AREA ) {
					if ( aas->AreaFlags( aasTrace.blockingAreaNum ) & AREA_LEDGE ) {
						path.endPos = trace.endPos;
						path.endNormal = trace.normal;
						path.endEvent = SE_ENTER_LEDGE_AREA;
						path.blockingEntity = trace.blockingEntity;

						return true;
					}
				}
				if ( stopEvent & SE_ENTER_OBSTACLE ) {
					if ( aas->AreaTravelFlags( aasTrace.blockingAreaNum ) & TFL_INVALID ) {
						path.endPos = trace.endPos;
						path.endNormal = trace.normal;
						path.endEvent = SE_ENTER_OBSTACLE;
						path.blockingEntity = trace.blockingEntity;

						return true;
					}
				}
			}
		} else {
			trace.fraction = clipTrace.fraction;
			trace.endPos = clipTrace.endpos;
			trace.normal = clipTrace.c.normal;
			trace.blockingEntity = gameLocal.entities[ clipTrace.c.entityNum ];
		}
	}

	if ( trace.fraction >= 1.0f ) {
		trace.blockingEntity = NULL;
	}

	return false;
}
/*
============
BotPlayer::PredictPath

Can also be used when there is no AAS file available however ledges are not detected.
============
*/
// RAVEN BEGIN
// nmckenzie: Added ignore ent parm.
bool afiBotPlayer::PredictPath( idPlayer *player, const idAAS *aas, const idVec3 &start, const idVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path, int drawDebug ) {
	// RAVEN END
	int i, j, step, numFrames, curFrameTime;
	idVec3 delta, curStart, curEnd, curVelocity, lastEnd, stepUp, tmpStart;
	idVec3 gravity, gravityDir, invGravityDir;
	float maxStepHeight, minFloorCos;
	pathTrace_t trace;

	// physics setup
	idPhysics_Player *physics = static_cast<idPhysics_Player *>( player->GetPhysics() );
	gravity = physics->GetGravity();
	gravityDir = physics->GetGravityNormal();
	invGravityDir = gravityDir * -1;
	maxStepHeight = physics->GetMaxStepHeight();
	minFloorCos = 0.7f; // same as MIN_WALK_NORMAL from physics_player.cpp

	float scale = PlayerCmdScale( player->usercmd );
	idVec3 viewForward, viewRight;
	player->viewAngles.ToVectors( &viewForward, &viewRight );
	idVec3 wishvel = viewForward * player->usercmd.forwardmove + viewRight * player->usercmd.rightmove;
	idVec3 wishdir = wishvel;
	float wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// path initialization
	path.endPos = start;
	path.endVelocity = velocity;
	path.endNormal.Zero();
	path.endEvent = 0;
	path.endTime = 0;
	path.blockingEntity = NULL;

	curStart = start;
	curVelocity = velocity;

	numFrames = ( totalTime + frameTime - 1 ) / frameTime;
	curFrameTime = frameTime;
	for ( i = 0; i < numFrames; i++ ) {
		if ( i == numFrames-1 ) {
			curFrameTime = totalTime - i * curFrameTime;
		}

		// accelerate the current velocity if player is set to this is done in airmove - so happens up front (before averaged)
		PlayerAccelerate( curVelocity, frameTime * .001f, wishdir, wishspeed, PM_AIRACCELERATE );

		// add gravity
		path.endVelocity = curVelocity + gravity * frameTime * 0.001f;
		// use average velocity of frame for delta calc
		curVelocity = ( curVelocity + path.endVelocity ) * 0.5f;

		delta = curVelocity * curFrameTime * 0.001f;

		//path.endVelocity = curVelocity;
		path.endTime = i * frameTime;

		// allow sliding along a few surfaces per frame
		for ( j = 0; j < MAX_FRAME_SLIDE; j++ ) {
			idVec3 lineStart = curStart;

			// allow stepping up three times per frame
			for ( step = 0; step < 3; step++ ) {
				curEnd = curStart + delta;
				// RAVEN BEGIN
				// nmckenzie: Added ignore ent
				if ( PathTrace( player, aas, curStart, curEnd, stopEvent, trace, path, drawDebug ) ) {
					// RAVEN END
					return true;
				}

				if ( step ) {
					// step down at end point
					tmpStart = trace.endPos;
					curEnd = tmpStart - stepUp;
					// RAVEN BEGIN
					// nmckenzie: Added ignore ent
					if ( PathTrace( player, aas, tmpStart, curEnd, stopEvent, trace, path ) ) {
						return true;
					}
					// RAVEN END

					// if not moved any further than without stepping up, or if not on a floor surface
					if ( (lastEnd - start).LengthSqr() > (trace.endPos - start).LengthSqr() - 0.1f ||
						( trace.normal * invGravityDir ) < minFloorCos ) {
							if ( stopEvent & SE_BLOCKED ) {
								path.endPos = lastEnd;
								path.endEvent = SE_BLOCKED;

								if ( drawDebug ) {
									gameRenderWorld->DebugLine( colorWhite, lineStart, lastEnd, drawDebug );
								}

								return true;
							}

							curStart = lastEnd;
							break;
					}
				}

				path.endNormal = trace.normal;
				path.blockingEntity = trace.blockingEntity;

				// cusTom3 - added stop event SE_LAND - TODO: should i be checking trace.fraction < 1 first
				if ( ( stopEvent & SE_LAND ) && ( trace.normal * invGravityDir ) > minFloorCos ) {
					path.endEvent |= SE_LAND;
					path.endPos = trace.endPos;
					path.endNormal = trace.normal;
					path.blockingEntity = trace.blockingEntity;
					path.endVelocity.z = 0; // hit ground :)

					if ( drawDebug ) {
						gameRenderWorld->DebugLine( colorRed, curStart, trace.endPos, drawDebug );
					}

					return true;
				}

				// TODO: adjust for air physics on jump pads - test something like:
				if ( trace.fraction < 1.0f && trace.endPos.z > curStart.z ) {
					idVec3 ztest = curEnd;
					ztest.x = trace.endPos.x; // adjust x and y so z can go up more
					ztest.y = trace.endPos.y;
					if ( PathTrace( player, aas, curStart, ztest, stopEvent, trace, path, drawDebug ) ) {
						return true;
					}
					// ztest.Zero(); // just testing to see if i get here
				}

				if ( trace.fraction >= 1.0f || ( trace.normal * invGravityDir ) > minFloorCos ) {
					curStart = trace.endPos;
					break;
				}

				// save last result
				lastEnd = trace.endPos;

				// step up
				stepUp = invGravityDir * maxStepHeight;
				// RAVEN BEGIN
				// nmckenzie: Added ignore ent
				if ( PathTrace( player, aas, curStart, curStart + stepUp, stopEvent, trace, path, drawDebug ) ) {
					return true;
				}
				// RAVEN END
				stepUp *= trace.fraction;
				curStart = trace.endPos;
			}

			if ( drawDebug ) {//PredictPath
				gameRenderWorld->DebugLine( colorPurple, lineStart, curStart, drawDebug );
			}

			if ( trace.fraction >= 1.0f ) {
				break;
			}

			delta.ProjectOntoPlane( trace.normal, OVERCLIP );
			curVelocity.ProjectOntoPlane( trace.normal, OVERCLIP );

			if ( stopEvent & SE_BLOCKED ) {
				// if going backwards
				if ( (curVelocity - gravityDir * curVelocity * gravityDir ) *
					(velocity - gravityDir * velocity * gravityDir) < 0.0f ) {
						path.endPos = curStart;
						path.endEvent = SE_BLOCKED;

						return true;
				}
			}
		} // end allow sliding along a few surfaces per frame

		if ( j >= MAX_FRAME_SLIDE ) {
			if ( stopEvent & SE_BLOCKED ) {
				path.endPos = curStart;
				path.endEvent = SE_BLOCKED;
				return true;
			}
		}

		// since we are using curVelocity = average
		curVelocity = path.endVelocity;
	}

	path.endTime = totalTime;
	path.endVelocity = curVelocity;
	path.endPos = curStart;
	path.endEvent = 0;

	return false;
}
