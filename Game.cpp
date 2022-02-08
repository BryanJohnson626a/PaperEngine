#include "Game.h"
#include "Object.h"
#include "Sprite.h"
#include "Transform.h"
#include "Input.h"
#include "Random.h"

Engine::Object * player;
std::vector<Engine::Object *> enemies;
Engine::RNG rng;

void GameInitialization()
{
	player = Engine::Object::NewObject();
	player->sprite->SetSubsprite(2);

	Engine::Sprite * enemy_sprite = Engine::Sprite::NewSprite();

	for (int i = 0; i < 100; ++i)
	{
		auto enemy = Engine::Object::NewObject(enemy_sprite);
		enemies.emplace_back(enemy);
		enemy->transform->SetPosition(glm::vec2(rng(-4.f, 4.f), rng(-4.f, 4.f)));
		enemy->transform->SetSize(0.75f);
		enemy->sprite->SetSubsprite(112);
	}
}

void GameUpdate()
{
	float move_speed = 2;
	glm::vec2 move_vector{};

	if (Engine::Input::IsDown(Engine::Input::Key::MoveUp))
		move_vector.y += 1;

	if (Engine::Input::IsDown(Engine::Input::Key::MoveDown))
		move_vector.y -= 1;

	if (Engine::Input::IsDown(Engine::Input::Key::MoveRight))
		move_vector.x += 1;

	if (Engine::Input::IsDown(Engine::Input::Key::MoveLeft))
		move_vector.x -= 1;

	if (move_vector.x != 0 || move_vector.y != 0)
		move_vector = glm::normalize(move_vector);

	player->transform->Move(move_vector * move_speed * Engine::GetDeltaTime());


}

void GameShutdown()
{
}
