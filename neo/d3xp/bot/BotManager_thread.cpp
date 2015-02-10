#include "precompiled.h"
#include "BotManager.h"
#include "BotBrain.h"
#include "BotPlayer.h"

botWorkerThread::botWorkerThread(condition_variable* conditional_variable, mutex* thread_mutex, PyInterpreterState* mainState, unsigned int* initializeCounter ) {
	for(unsigned int iClient = 0; iClient < MAX_CLIENTS; iClient++) {
		packedUpdateArray[iClient] = NULL;
	}
	threadState = NULL;
	interpState = mainState;
	threadMutex = thread_mutex;
	threadConditional = conditional_variable;
	endUpdateTask = -1;
	currentUpdateTask = -1;
	threadInitializeCounter = initializeCounter;
	threadObj = thread(&botWorkerThread::RunWork,this);
}

botWorkerThread::~botWorkerThread( ) {
	//Wait for the thread to end
	threadObj.join();

	for(unsigned int iClient = 0; iClient < MAX_CLIENTS; ++iClient) {
		packedUpdateArray[iClient] = NULL;
	}
	endUpdateTask = currentUpdateTask = -1;
	threadMutex = nullptr;
	threadConditional = nullptr;

}

void botWorkerThread::AddUpdateTask( afiBotBrain* newTask ) {
	if(-1 == endUpdateTask) {
		currentUpdateTask = 0;
		endUpdateTask = 0;
	} else if( MAX_CLIENTS < endUpdateTask ) {
		return;
	}

	packedUpdateArray[endUpdateTask++] = newTask;
}

void botWorkerThread::RunWork( ) {
	//Any One time thread init stuff should happen here
	std::unique_lock<mutex> threadLock(*threadMutex);
	threadState = PyThreadState_New(interpState);
	PyEval_RestoreThread(threadState);
	threadState = PyEval_SaveThread();
	afiBotManager::SetThreadState(threadState,this);
	(*threadInitializeCounter)++;
	threadConditional->notify_one();
	threadLock.unlock();
	while(true) {
		threadLock.lock();
		threadConditional->wait(threadLock,[&](){if(-1 == endUpdateTask && !afiBotManager::isGameEnding() ) return false;return true;});
		threadLock.unlock();

		if(afiBotManager::isGameEnding() ) {
			break;
		}

		do {
			workTimer.Clear();

			PyEval_RestoreThread(threadState);

			workTimer.Start();
			packedUpdateArray[currentUpdateTask]->Think(deltaTime);
			workTimer.Stop();

			threadState = PyEval_SaveThread();

			double workTime = workTimer.Milliseconds();
			if( workTime > 1.0f ) {
				//Work took to long, distribute work to other thread and stop for frame
				break;
			}
			packedUpdateArray[currentUpdateTask] = nullptr;

			currentUpdateTask++;
		} while( currentUpdateTask < endUpdateTask );

		//Look for more work to do this frame
		afiBotManager::DecreaseThreadUpdateCount();
		//TODO: Implement Work
		currentUpdateTask = -1;
		endUpdateTask = -1;

		if(afiBotManager::isGameEnding() ) {
			break;
		}
	}

	//Doing final shutdown of threadState
	PyEval_RestoreThread(threadState);
	PyThreadState_Clear(threadState);
	threadState = PyEval_SaveThread();
	PyThreadState_Delete(threadState);

}

void botWorkerThread::InitializeForFrame( unsigned int endUpdateIndex ) {
}

bool botWorkerThread::CheckWorkTime( ) {
	return false;
}

bool botWorkerThread::LookForMoreWork( ) {
	return false;
}

void botWorkerThread::RemoveFailedBot( int removeIndex ) {
}