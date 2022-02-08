#include "Core.h"
#include "Game.h"

int main()
{
	Engine::PostInitialization = GameInitialization;
	Engine::PostUpdate = GameUpdate;
	Engine::PreShutdown = GameShutdown;

	Engine::Initialize();
	while (Engine::Update()) {}
	Engine::Shutdown();
}