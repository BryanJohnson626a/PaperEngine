#include "Texture.h"
#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Engine
{
	std::vector<Texture> Texture::all_textures;

	TextureID Texture::AddTexture(std::string filename)
	{
		if (all_textures.size() >= MAX_TEXTURES)
			throw std::runtime_error("Max texture limit reached.");

		all_textures.emplace_back(Texture());

		Texture & new_texture = all_textures.back();

		new_texture.Load(filename);
		
		return new_texture.id;
	}

	Texture & Texture::GetTexture(TextureID id)
	{
		return all_textures[id];
	}

	void Texture::RemoveTexture(TextureID id)
	{
		throw std::runtime_error("TODO");
	}

	void Texture::UnloadTextures()
	{
		for (Texture & texture : all_textures)
			texture.Unload();
	}

	int texture_id_counter = 0;
	Texture::Texture() : id(texture_id_counter++)
	{}

	void Texture::Load(std::string filename)
	{
		int texture_width, texture_height, texture_channels;
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
	}

	glm::vec4 Texture::GetOffset(int sub_sprite_number) const
	{
		return glm::vec4((float)sub_sprite_number);
	}
	VkImageView Texture::GetImageView() const
	{
		return image_view;
	}
}
