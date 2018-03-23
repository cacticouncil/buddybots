/*
===========================================================================
File: BotBrain.cpp
Author: John Wileczek
Description: Defines the basic interface that bot brains should follow.
===========================================================================
*/

#include "BotBrain.h"
#include "BotPlayer.h"
#include "../Game_local.h"
#include "../physics/Physics_Player.h"

#include <memory>
using namespace std;

// Workaround for problem in VS14
/*namespace boost
{
	template <>
	afiBotBrainWrapper const volatile * get_pointer<class afiBotBrainWrapper const volatile >(
		class afiBotBrainWrapper const volatile *wrapped)
	{
		return wrapped;
	}
}*/

BOOST_PYTHON_MODULE(afiBotBrain) {
	//import("idDict");
	//import("idVec3");
	//import("afiBotPlayer");

	enum_<aiViewType_t>("aiViewType_t")
		.value("VIEW_DIR",VIEW_DIR)
		.value("VIEW_POS",VIEW_POS)
		.export_values()
		;

	enum_<aiMoveFlag_t>("aiMoveFlag_t")
		.value("NULLMOVE",NULLMOVE)
		.value("CROUCH",CROUCH)
		.value("JUMP",JUMP)
		.value("WALK",WALK)
		.value("RUN",RUN)
		.export_values()
		;

	class_<aiCommands_t>("aiCommands_t")
		.def_readwrite("attack",&aiCommands_t::attack)
		.def_readwrite("zoom",&aiCommands_t::zoom)
		;
	class_<aiInput_t>("aiInput_t")
		.def_readwrite("viewDirection",&aiInput_t::viewDirection)
		.def_readwrite("viewType",&aiInput_t::viewType)
		.def_readwrite("moveDirection",&aiInput_t::moveDirection)
		.def_readwrite("moveSpeed",&aiInput_t::moveSpeed)
		.def_readwrite("moveFlag",&aiInput_t::moveFlag)
		.def_readwrite("commands",&aiInput_t::commands)
		;

	class_<afiBotBrainWrapper,shared_ptr<afiBotBrainWrapper>,boost::noncopyable>("afiBotBrain")
		.def("Think",pure_virtual(&afiBotBrain::Think))
		.def("Spawn",pure_virtual(&afiBotBrain::Spawn))
		.def("Restart",pure_virtual(&afiBotBrain::Restart))
		.add_property("body",
		make_function(&afiBotBrain::GetBody,return_internal_reference<>()))
		.def_readonly("physicsObject",&afiBotBrain::physicsObject)
		;
}

aiInput_t afiBotBrainWrapper::Think(int deltaTimeMS)  {
	object scriptResult;
	aiInput_t scriptInput;
	try {
		scriptResult = this->get_override("Think")(deltaTimeMS);
	} catch(...) {
		gameLocal.HandlePythonError();
	}
	scriptInput = extract<aiInput_t>(scriptResult);
	return scriptInput;
}

void afiBotBrainWrapper::Spawn() {
	try {
		this->get_override("Spawn")(botDict);
	}
	catch(...) {
		gameLocal.HandlePythonError();
	}
}

void afiBotBrainWrapper::Restart() {
	try {
		this->get_override("Restart")();
	}
	catch(...) {
		gameLocal.HandlePythonError();
	}
}

void afiBotBrainWrapper::OnRespawn() {
	override functionOverride = this->get_override("OnRespawn");
	if (functionOverride) {
		try {
			functionOverride();
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}
}

void afiBotBrainWrapper::OnKill(idPlayer* dead, idPlayer* killer, const idVec3& dir, int damage) {
	override functionOverride = this->get_override("OnKill");
	if (functionOverride) {
		try {
			functionOverride(dead,killer,dir,damage);
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}

}

void afiBotBrainWrapper::OnDeath(idPlayer* dead, idPlayer* killer, const idVec3& dir, int damage) {
	override functionOverride = this->get_override("OnDeath");
	if (functionOverride) {
		try {
			functionOverride(dead,killer,dir,damage);
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}
}

void afiBotBrainWrapper::OnDisconnect(int clientNum) {
	override functionOverride = this->get_override("OnDisconnect");
	if (functionOverride) {
		try {
			functionOverride(clientNum);
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}
}

void afiBotBrainWrapper::OnPain(idEntity* inflictor, idEntity* attacker,const idVec3& dir, int damage) {
	override functionOverride = this->get_override("OnPain");
	if (functionOverride) {
		try {
			functionOverride(inflictor,attacker,dir,damage);
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}
}

void afiBotBrainWrapper::OnHit(idPlayer* target, const idVec3& dir, int damage) {
	override functionOverride = this->get_override("OnHit");
	if (functionOverride) {
		try {
			functionOverride(target, dir, damage);
		}
		catch (...) {
			gameLocal.HandlePythonError();
		}
	}

}

void afiBotBrain::SetBody(afiBotPlayer* newBody) {
	body = newBody;
}

afiBotPlayer* afiBotBrain::GetBody() {
	return body;
}

idPhysics_Player* afiBotBrain::GetPhysics() {
	return physicsObject;
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