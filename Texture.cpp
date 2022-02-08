#include "Texture.h"
#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
	std::array<Texture, MAX_TEXTURES> all_textures;
	int num_textures = 0;

	Texture * Texture::AddTexture(std::string filename, int images_x, int images_y)
	{
		Texture * texture = &all_textures[num_textures++];

		texture->Load(filename);
		texture->num_images_x = images_x;
		texture->num_images_y = images_y;

		return texture;
	}

	void Texture::LoadTextures()
	{
		AddTexture("assets/DawnLike/Characters/Player0.png", 8, 15);
		AddTexture("assets/DawnLike/Characters/Undead0.png", 8, 10);
	}

	void Texture::UnloadTextures()
	{
		for (int i = 0; i < num_textures; ++i)
			all_textures[i].Unload();
	}

	Texture * Texture::GetTexture(int index)
	{
		return &all_textures[index];
	}

	void Texture::Load(std::string filename)
	{
		int texture_channels;
		stbi_uc * pixels = stbi_load(filename.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
		VkDeviceSize texture_size = texture_width * texture_height * texture_channels;

		if (!pixels)
			throw std::runtime_error(std::format("Failed to load {}.", filename));

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		Graphics::CreateBuffer(texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		void * data;
		vkMapMemory(Graphics::GetDevice(), staging_buffer_memory, 0, texture_size, 0, &data);
		memcpy(data, pixels, size_t(texture_size));
		vkUnmapMemory(Graphics::GetDevice(), staging_buffer_memory);

		stbi_image_free(pixels);

		Graphics::CreateImage(texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			image, image_memory);

		Graphics::TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		Graphics::CopyBufferToImage(staging_buffer, image, uint32_t(texture_width), uint32_t(texture_height));
		Graphics::TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(Graphics::GetDevice(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::GetDevice(), staging_buffer_memory, nullptr);

		image_view = Graphics::CreateImageView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	}

	void Texture::Unload()
	{
		vkDestroyImageView(Graphics::GetDevice(), image_view, nullptr);
		vkDestroyImage(Graphics::GetDevice(), image, nullptr);
		vkFreeMemory(Graphics::GetDevice(), image_memory, nullptr);
		image_view = nullptr;
		image = nullptr;
		image_memory = nullptr;
	}

	glm::mat3 Texture::GetOffset(int sub_sprite_number) const
	{
		glm::vec2 offset{ 1.f / num_images_x, 1.f / num_images_y };

		glm::vec2 topleft{ offset.x * (sub_sprite_number % num_images_x),
			offset.y * (sub_sprite_number / num_images_x)};

		glm::mat3 transform{ {offset.x, 0, 0},
							 {0, offset.y, 0},
							 {topleft.x, topleft.y, 1} };
		return transform;
	}
	VkImageView Texture::GetImageView() const
	{
		return image_view;
	}
}
