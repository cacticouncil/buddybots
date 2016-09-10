#ifndef YOURBOTBRAIN_H_
#define YOURBOTBRAIN_H_


#include "../d3xp/bot/BotBrain.h"

class YourBotBrain : public afiBotBrain {
public:
								YourBotBrain();
								~YourBotBrain();
	 void						Destroy();
	 aiInput_t					Think();
	 void						Spawn(idDict* userInfo);
	 void						Restart();
	 afiBotBrain*				Clone();

private:

	idDict						spawnInfo;

};

#endif