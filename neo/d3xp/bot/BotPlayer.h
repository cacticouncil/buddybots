/*
===========================================================================
File: BotPlayer.h
Author: John Wileczek
Description: Fake client 'body' implementation of bot. Responsible for
translating input received from afiBotBrain to usercmd_t to be sent over
the network.
===========================================================================
*/
#ifndef BOTPLAYER_H_
#define BOTPLAYER_H_

#include "../Game_local.h"
#include "ai/AI.h"
#include "Player.h"

extern idCVar bot_debugBot;

#include "BotBrain.h"

struct botMoveState_t {
	botMoveState_t();

	struct movementFlags_s {
		bool		done				:1;
		bool		moving				:1;
		bool		crouching			:1;
		bool		running				:1;
		bool		blocked				:1;
		bool		obstacleInPath		:1;
		bool		goalUnreachable		:1;
		bool		onGround			:1;
		bool		flyTurning			:1;

		bool		idealRunning		:1;

		bool		disabled			:1;
		bool		ignoreObstacles		:1;
		bool		allowPushMovables	:1;

		bool		noRun				:1;
		bool		noWalk				:1;
		bool		noTurn				:1;
		bool		noGravity			:1;
		bool		noRangedInterrupt	:1;
	} flags;

	moveType_t				moveType;
	moveCommand_t			moveCommand;
	moveStatus_t			moveStatus;
	idVec3					moveDest;
	idVec3					moveDir;

	int						toAreaNum;
	int						startTime;
	int						duration;
	float					speed;
	float					range;
	float					wanderYaw;
	int						nextWanderTime;
	int						blockTime;
	idEntityPtr<idEntity>	obstacle;
	idVec3					lastMoveOrigin;
	int						lastMoveTime;
	int						anim;

	int						travelFlags;

	float					kickForce;
	float					blockedRadius;
	int						blockedMoveTime;
	int						blockedAttackTime;

	float					current_yaw;

	idVec3					goalPos;
	int						goalArea;
	idEntityPtr<idEntity>	goalEntity;
	idVec3					goalEntityOrigin;

	idVec3					seekPos;
	idVec3					addVelocity;

	aasPath_t				lastPath;
	aasPath_t				path;

	idVec3					lastValidPos;
};
/*
===============================================================================

afiBotPlayer
Basic fake client, and 'body' of the bot.
This class will also contain basic navigation and translation of input to usercmd_t
===============================================================================
*/
class afiBotPlayer : public idPlayer {
public:
	CLASS_PROTOTYPE( afiBotPlayer );

	static bool				PathTrace( idPlayer *player, const idAAS *aas, const idVec3 &start, const idVec3 &end, int stopEvent, struct pathTrace_s &trace, predictedPath_t &path, int drawDebug = 0 );
	static bool				PredictPath( idPlayer *player, const idAAS *aas, const idVec3 &start, const idVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path, int drawDebug = 0 );

	afiBotPlayer();
	virtual					~afiBotPlayer();

	void					Spawn( void );
	virtual void 			Think( void );

	virtual void			PrepareForRestart( void );
	virtual	void			Restart( void );

	bool					DebugBot( void );

public:

	
	void					ClearInput( void );
	void					ProcessInput( void );

	//virtual idEntity*		FindItem( const char* item );

	void					SetBrain(afiBotBrain* newBrain);
	afiBotBrain*			GetBrain(void) const;

	bool					SwitchWeapon(const char* weaponName);
	int						HasAmmo(const char* weaponName);
	void					AmmoInClip();


	void					SpawnFromSpawnSpot(void);
	void					SpawnToPoint(const idVec3	&spawn_origin, const idAngles &spawn_angles);
	//Movement
	void					SetAAS( void );

	virtual void			Move( void );

	virtual bool			GetMovePos( idVec3 &seekPos, idReachability** seekReach );
	void					CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos, idReachability* goalReach=0  );
	void					BlockedFailSafe( void );
	bool					InView( idEntity* entity );
	virtual void			MoveTo( const idVec3 &pos, float speed );
	virtual bool			MoveToPosition ( const idVec3 &pos, float range );
	virtual bool			MoveToEntity( idEntity* entity );
	virtual bool			MoveToPlayer( idPlayer *player );
	virtual idEntity*		MoveToNearest( const char* item );
	virtual idEntity*		FindItem(const char* item);
	virtual bool			PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const;
	virtual int				PointReachableAreaNum( const idVec3 &pos ) const;
	virtual void			MoveToAttackPosition(idEntity* entity);
	void					Attack(void);
	void					StopAttack(void);
	void					Jump(void);
	void					SaveLastTarget(idEntity* entity);
	idEntity*				GetLastTarget(void);
	void					LookInDirection(const idVec3& dir);
	void					LookAtPosition(const idVec3& pos);
	void					UpdateAIMoveFlag(aiMoveFlag_t flag);

	void					Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location);
	//void					Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

	virtual bool			ReachedPos( const idVec3 &pos,float range  = 0.0f ) const;

	virtual bool			StartMove ( const idVec3& goalOrigin, int goalArea, idEntity* goalEntity, float range );
	virtual void			StopMove( moveStatus_t status );

	py::list		FindNearbyPlayers();
	py::list		FindItemsInView();

	idStr					botName;
	idStr					teamName;
	int						clientNum;

protected:

	virtual void			DrawRoute( void ) const;

	void					UpdateViewAngles( void );
	void					ProcessMove( void );
	void					ProcessCommands( void );

protected:
	idAngles				lastViewAngles;
	idAngles				idealViewAngles;
	float					arcHalf;
	float					arcDistance;
	idVec3					lastLookAtPosition;
	float					aimRate;

	idEntity*				target;
	botMoveState_t			move;
	aiInput_t				aiInput;
	usercmd_t				botcmd;
	idAAS *					aas;
	afiBotBrain*			brain;
};

ID_INLINE bool afiBotPlayer::DebugBot( void ) {
	if ( bot_debugBot.GetInteger() == -2 || bot_debugBot.GetInteger() == entityNumber ) {
		return true;
	}
	return false;
}

#endif  //BOTPLAYER_H_
