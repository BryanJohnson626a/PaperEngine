#pragma once
#include "Core.h"

namespace Engine
{
	class Sprite
	{
	public:
		static Sprite * NewSprite();
		static std::array<Sprite, MAX_SPRITES> & GetSprites();
		static int GetNumSprites();

		void SetTexture(Texture * new_texture);
		Texture * GetTexture();

		void SetSubsprite(int new_subsprite);
		const int GetSubsprite() const;
		const glm::mat3 GetTexOffset() const;
	private:
		Texture * texture{};
		int subsprite{};
		glm::mat3 tex_offset{ 1 };
	};
}
