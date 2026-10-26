#include "FlightStickManager.h"
#include <SDL2/SDL.h>

FlightStickManager* FlightStickManager::mInstance = nullptr;

FlightStickManager* FlightStickManager::GetInstance() {
    if (!mInstance) mInstance = new FlightStickManager();
    return mInstance;
}

FlightStickManager::FlightStickManager() {}
FlightStickManager::~FlightStickManager() {}
void FlightStickManager::Init() {}
void FlightStickManager::Update() {}
void FlightStickManager::UpdateFFB() {}
void FlightStickManager::ProcessInput(OSContPad* pad) {}
bool FlightStickManager::IsEnabled() { return false; }
