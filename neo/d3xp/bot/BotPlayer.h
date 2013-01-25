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

extern idCVar bot_debugBot;


#include "BotBrain.h"

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
protected:
	void					UpdateViewAngles( void );
	void					ProcessMove( void );
	void					ProcessCommands( void );

protected:
	aiInput_t				aiInput;
	usercmd_t				botcmd;

	afiBotBrain*			brain;
};


ID_INLINE bool afiBotPlayer::DebugBot( void ) {
	if ( bot_debugBot.GetInteger() == -2 || bot_debugBot.GetInteger() == entityNumber ) {
		return true;
	}
	return false;
}

#endif  //BOTPLAYER_H_



