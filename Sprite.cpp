#include "Sprite.h"
#include "Texture.h"

namespace Engine
{
	std::array<Sprite, MAX_SPRITES> all_sprites;
	int num_sprites = 0;

	Sprite * Sprite::NewSprite()
	{
		Sprite * sprite = &all_sprites[num_sprites++];
		sprite->texture = Texture::GetTexture(0);
		sprite->SetSubsprite(0);
		return sprite;
	}

	std::array<Sprite, MAX_SPRITES> & Sprite::GetSprites()
	{
		return all_sprites;
	}

	int Sprite::GetNumSprites()
	{
		return num_sprites;
	}

	void Engine::Sprite::SetTexture(Texture * new_texture)
	{
		texture = new_texture;
	}

	Texture * Sprite::GetTexture()
	{
		return texture;
	}

	void Sprite::SetSubsprite(int new_subsprite)
	{
		subsprite = new_subsprite;
		tex_offset = texture->GetOffset(subsprite);
	}

	const int Sprite::GetSubsprite() const
	{
		return subsprite;
	}

	const glm::mat3 Sprite::GetTexOffset() const
	{
		return tex_offset;
	}
}
