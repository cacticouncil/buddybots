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

namespace botBrain
{
	PYBIND11_PLUGIN(afiBotBrain) {
		py::module m("afiBotBrain", "description");
		//import("idDict");
		//import("idVec3");
		//import("afiBotPlayer");

		py::enum_<aiViewType_t>(m, "aiViewType_t")
			.value("VIEW_DIR", VIEW_DIR)
			.value("VIEW_POS", VIEW_POS)
			.export_values()
			;

		py::enum_<aiMoveFlag_t>(m, "aiMoveFlag_t")
			.value("NULLMOVE", NULLMOVE)
			.value("CROUCH", CROUCH)
			.value("JUMP", JUMP)
			.value("WALK", WALK)
			.value("RUN", RUN)
			.export_values()
			;

		py::class_<aiCommands_t>(m, "aiCommands_t")
			.def_readwrite("attack", &aiCommands_t::attack)
			.def_readwrite("zoom", &aiCommands_t::zoom)
			;
		py::class_<aiInput_t>(m, "aiInput_t")
			.def_readwrite("viewDirection", &aiInput_t::viewDirection)
			.def_readwrite("viewType", &aiInput_t::viewType)
			.def_readwrite("moveDirection", &aiInput_t::moveDirection)
			.def_readwrite("moveSpeed", &aiInput_t::moveSpeed)
			.def_readwrite("moveFlag", &aiInput_t::moveFlag)
			.def_readwrite("commands", &aiInput_t::commands)
			;

		py::class_<afiBotBrain, pyAfiBotBrain>(m, "afiBotBrain")
			.def("Think", &afiBotBrain::Think)
			.def("Spawn", &afiBotBrain::Spawn)
			.def("Restart", &afiBotBrain::Restart)
			.def_property("body", &afiBotBrain::GetBody, py::return_value_policy::reference_internal)
			.def_readonly("physicsObject", &afiBotBrain::physicsObject)
			;

		return m.ptr();
	}
}

/*aiInput_t afiBotBrainWrapper::Think(int deltaTimeMS)  {
	py::object scriptResult;
	aiInput_t scriptInput;
	try {
		scriptResult = this->get_override("Think")(deltaTimeMS);
	} catch(...) {
		gameLocal.HandlePythonError();
	}
	scriptInput = extract<aiInput_t>(scriptResult);
	return scriptInput;
}

void pyAfiBotBrain::Spawn() {
	try {
		this->get_override("Spawn")(botDict);
	}
	catch(...) {
		gameLocal.HandlePythonError();
	}
}

void pyAfiBotBrain::Restart() {
	try {
		this->get_override("Restart")();
	}
	catch(...) {
		gameLocal.HandlePythonError();
	}
}

void pyAfiBotBrain::OnRespawn() {
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

}*/

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