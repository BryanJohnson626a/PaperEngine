#include "Sprite.h"

namespace Engine
{
	std::vector<Sprite> Sprite::all_sprites;
	std::vector<int> Sprite::depth_order;

	int id_counter = 0;

	Sprite::Sprite() :
		id(id_counter++),
		texture(nullptr),
		position({ 0,0 }),
		rotation_radians(0),
		size({ 1,1 })
	{
	}

	Sprite * Sprite::AddSprite()
	{
		if (all_sprites.size() >= MAX_SPRITES)
			throw std::runtime_error("Max sprite limit reached.");

		all_sprites.emplace_back(Sprite());
		return &all_sprites.back();
	}

	void Sprite::RemoveSprite(SpriteID /*id*/)
	{
		throw "Not Implemented.";
	}

	void Sprite::RemoveSprite(Sprite * sprite)
	{
		RemoveSprite(sprite->id);
	}

	Sprite * Sprite::GetSprite(SpriteID id)
	{
		for (auto & sprite : all_sprites)
			if (sprite.id == id)
				return &sprite;

		throw std::invalid_argument(std::format("No sprite with ID {}.", id));
	}

	std::vector<Sprite> & Sprite::GetSprites()
	{
		return all_sprites;
	}

	std::vector<int> & Sprite::GetSpriteDepthOrder()
	{
		return depth_order;
	}

	void Sprite::SetPosition(glm::vec2 new_position)
	{
		position = new_position;
	}

	const glm::vec2 Sprite::GetPosition() const
	{
		return position;
	}

	void Sprite::SetSize(glm::vec2 new_size)
	{
		size = new_size;
	}

	const glm::vec2 Engine::Sprite::GetSize() const
	{
		return size;
	}

	void Engine::Sprite::SetRotation(float new_rotation)
	{
		rotation_radians = new_rotation;
	}

	const float Sprite::GetRotation() const
	{
		return rotation_radians;
	}
}
