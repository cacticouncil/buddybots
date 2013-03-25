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
#include "../Game_local.h"
#include "../physics/Physics_Player.h"

/*afiBotBrain::afiBotBrain() {
	body = NULL;
	memset( &bodyInput, 0, sizeof( bodyInput ) );
}

afiBotBrain::~afiBotBrain() {

}*/

void afiBotBrain::SetBody(afiBotPlayer* newBody) {
	body = newBody;
}

void afiBotBrain::SetUserInfo(idDict* userInfo) {
	botInfo.TransferKeyValues(*userInfo);
}

void afiBotBrain::SetAAS() {
	aas = gameLocal.GetAAS( "aas48" );
	if ( aas ) {
		const idAASSettings *settings = aas->GetSettings();
		if ( settings ) {
			float height = settings->maxStepHeight;
			physicsObject->SetMaxStepHeight( height );
			return;
		} else {
			aas = NULL;
		}
	}
	
	gameLocal.Error( "Bot cannot find AAS file for map\n" ); // TinMan: No aas, no play.
}

void afiBotBrain::SetPhysics(idPhysics_Player* _physicsObject) {
	physicsObject = _physicsObject;
}