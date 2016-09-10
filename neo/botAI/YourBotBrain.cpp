#include "precompiled.h"

#include "YourBotBrain.h"

YourBotBrain::YourBotBrain() : afiBotBrain() {

}

YourBotBrain::~YourBotBrain() {


}

void YourBotBrain::Spawn(idDict* userInfo) {

	spawnInfo = *userInfo;
}

void YourBotBrain::Restart() {


}

afiBotBrain* YourBotBrain::Clone() {
	return new YourBotBrain();
}

void YourBotBrain::Destroy() {

	delete this;
}

aiInput_t YourBotBrain::Think() {
	aiInput_t frameInput;

	memset( &frameInput, 0, sizeof( frameInput ) );

	return frameInput;
}