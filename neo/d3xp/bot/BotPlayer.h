#ifndef BOTPLAYER_H_
#define BOTPLAYER_H_

#include "../Game_local.h"

extern idCVar bot_debugBot;

/*
===============================================================================

	BotPlayer
	Basic fake client, and 'body' of the bot.
	This class will also contain basic navigation and translation of input to usercmd_t
===============================================================================
*/

enum aiViewType_t {
	VIEW_DIR,
	VIEW_POS
};

enum aiMoveFlag_t {
		NULLMOVE = 0,
		CROUCH,
		JUMP,
		WALK,
		RUN
	};
	// Tex: Actions/Commands
	struct aiCommands_t {
		bool attack;
		bool zoom;
	};
	// Tex: all input
	struct aiInput_t {
		idVec3				viewDirection;
		aiViewType_t		viewType;
		idVec3				moveDirection;
		float				moveSpeed;
		aiMoveFlag_t		moveFlag;
		aiCommands_t		commands;
	};

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
protected:
	void					UpdateViewAngles( void );
	void					ProcessMove( void );
	void					ProcessCommands( void );

protected:
	aiInput_t				aiInput;
	usercmd_t				botcmd;
};


ID_INLINE bool afiBotPlayer::DebugBot( void ) {
	if ( bot_debugBot.GetInteger() == -2 || bot_debugBot.GetInteger() == entityNumber ) {
		return true;
	}
	return false;
}

#endif  //BOTPLAYER_H_



