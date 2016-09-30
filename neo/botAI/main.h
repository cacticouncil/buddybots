#ifndef MAIN_H_
#define MAIN_H_



#include "../d3xp/bot/BotBrain.h"
#include "../d3xp/Game.h"
//#include "../d3xp/bot/BotManager.h"

extern "C" afiBotBrain* (*CreateBrain_t)(botImport_t* dllSetup);


#endif