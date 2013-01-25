/*
===========================================================================
File: BotBrain.h
Author: John Wileczek
Description: Defines the basic interface that bot brains should follow.
===========================================================================
*/
#ifndef BOTBRAIN_H_
#define BOTBRAIN_H_

class afiBotPlayer;


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

struct aiCommands_t {
	bool attack;
	bool zoom;
};

struct aiInput_t {
	idVec3				viewDirection;
	aiViewType_t		viewType;
	idVec3				moveDirection;
	float				moveSpeed;
	aiMoveFlag_t		moveFlag;
	aiCommands_t		commands;
};

/*
===============================================================================

	afiBotBrain
	This will serve as the base class that all bot brains should be derived off of.
	This will define the basic interface that bots must adhere to, to maintain
	proper communication with the body of the bot.
	*NOTE* Still unsure if this class should be derived off of idEntity or keep
	it completely seperate from their hierarchy. This would have to be derived
	from idEntity if we want to make use of DoomScript and TypeInfo of the brain
===============================================================================
*/
class afiBotBrain
{
public:
								afiBotBrain();
	virtual						~afiBotBrain();

	//Pure Virtual Bot Interface
	virtual void				Destroy() = 0;
	virtual aiInput_t			Think() = 0;
	virtual void				Spawn(idDict* userInfo) = 0;
	virtual void				Restart() = 0;
	virtual afiBotBrain*		Clone() = 0;

	//TODO: Event Handling functions.
	//virtual void OnPain(idEntity* inflictor, idEntity* attacker, int damage);
	//virtual void OnDisconnect();
	//virtual void OnKill();
	//virtual void OnDeath();

	//Accessors and Mutators
	void						SetUserInfo(idDict* userInfo);
	void						SetBody(afiBotPlayer* newBody);
protected:
	//Pointer to the fake client body of the bot.
	afiBotPlayer*				body;

	//This will be the same spawn dict as the body
	//so the student can fill the one entityDef and
	//have access to those pairs in the brain.
	idDict						botInfo;

	aiInput_t					bodyInput;



private:

};

#endif