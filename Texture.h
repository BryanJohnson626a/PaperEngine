#pragma once
#include "Core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Engine
{
	using TextureID = int;

	class Texture
	{
	public:
		static TextureID AddTexture(std::string filename);
		static Texture & GetTexture(TextureID id);
		static void RemoveTexture(TextureID id);
		static void UnloadTextures();

		glm::vec4 GetOffset(int sub_sprite_number) const;
		VkImageView GetImageView() const;
	private:
		Texture();

		void Load(std::string filename);
		void Unload();

		static std::vector<Texture> all_textures;

		TextureID id;

		int num_images_x{};
		int num_images_y{};

		VkImage image{};
		VkDeviceMemory image_memory{};
		VkImageView image_view{};
	};
}
