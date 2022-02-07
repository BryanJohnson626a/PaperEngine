#pragma once
#include "Core.h"

namespace Engine
{
	using SpriteID = int;

	class Sprite
	{
	public:
		static Sprite * AddSprite();
		static void RemoveSprite(SpriteID id);
		static void RemoveSprite(Sprite * sprite);
		static Sprite * GetSprite(SpriteID id);
		static std::vector<Sprite> & GetSprites();
		static std::vector<int> & GetSpriteDepthOrder();

		void SetPosition(glm::vec2 new_position);
		const glm::vec2 GetPosition() const;

		void SetSize(glm::vec2 new_size);
		const glm::vec2 GetSize() const;

		void SetRotation(float new_rotation);
		const float GetRotation() const;
	private:
		Sprite();

		static std::vector<Sprite> all_sprites;
		static std::vector<int> depth_order;

		SpriteID id;
		Texture * texture;
		glm::vec2 position;
		float rotation_radians;
		glm::vec2 size;
	};
}
