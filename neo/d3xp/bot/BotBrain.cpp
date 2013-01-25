/*
===========================================================================
File: BotBrain.cpp
Author: John Wileczek
Description: Defines the basic interface that bot brains should follow.
===========================================================================
*/
#include "precompiled.h"

#include "BotBrain.h"
#include "BotPlayer.h"

afiBotBrain::afiBotBrain() {
	body = NULL;
	memset( &bodyInput, 0, sizeof( bodyInput ) );
}

afiBotBrain::~afiBotBrain() {

}

void afiBotBrain::SetBody(afiBotPlayer* newBody) {
	body = newBody;
}

void afiBotBrain::SetUserInfo(idDict* userInfo) {
	botInfo.TransferKeyValues(*userInfo);
}