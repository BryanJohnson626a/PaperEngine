#include "Core.h"

int main()
{
	Engine::Initialize();
	while (Engine::Update()) {}
	Engine::Shutdown();
}