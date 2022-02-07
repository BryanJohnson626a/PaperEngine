#include "Core.h"
#include "Sprite.h"

int main()
{
	Engine::Initialize();

	auto p = Engine::Sprite::AddSprite();
	p->SetPosition(glm::vec2(0, 0));

	while (Engine::Update()) {}

	Engine::Shutdown();
}