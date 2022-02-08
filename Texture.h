#pragma once
#include "Core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Engine
{
	class Texture
	{
	public:
		static Texture * AddTexture(std::string filename, int images_x = 1, int images_y = 1);
		static void LoadTextures();
		static void UnloadTextures();
		static Texture * GetTexture(int index);

		glm::mat3 GetOffset(int sub_sprite_number) const;
		VkImageView GetImageView() const;
	private:
		void Load(std::string filename);
		void Unload();

		int texture_width;
		int texture_height;

		int num_images_x{};
		int num_images_y{};

		VkImage image{};
		VkDeviceMemory image_memory{};
		VkImageView image_view{};
	};
}
