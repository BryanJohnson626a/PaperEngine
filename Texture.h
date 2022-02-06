#pragma once
#include "Core.h"

namespace Engine
{
	class Texture
	{
	public:
		Texture(std::string filename);

	private:
		int texture_ID;
		int num_images_x;
		int num_images_y;
	};
}
