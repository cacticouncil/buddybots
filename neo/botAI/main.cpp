#include "precompiled.h"

#include "main.h"
#include "Boitano.h"
#include "YourBotBrain.h"

idSys *						sys = NULL;
idCommon *					common = NULL;
idCmdSystem *				cmdSystem = NULL;
idCVarSystem *				cvarSystem = NULL;
idFileSystem *				fileSystem = NULL;
idNetworkSystem *			networkSystem = NULL;
idRenderSystem *			renderSystem = NULL;
idSoundSystem *				soundSystem = NULL;
idRenderModelManager *		renderModelManager = NULL;
idUserInterfaceManager *	uiManager = NULL;
idDeclManager *				declManager = NULL;
idAASFileManager *			AASFileManager = NULL;
idCollisionModelManager *	collisionModelManager = NULL;
idCVar *					idCVar::staticVars = NULL;

#if __MWERKS__
#pragma export on
#endif
#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" afiBotBrain* CreateBrain( void ) {
#if __MWERKS__
#pragma export off
#endif

	afiBotBrain* exportedBot = new BoitanoBot();




	return exportedBot;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif