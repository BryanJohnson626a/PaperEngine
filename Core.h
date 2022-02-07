#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <format>

namespace Engine
{
	class Texture;
	class Sprite;

	const int MAX_SPRITES = 100;
	const int MAX_TEXTURES = 100;


	void Initialize();
	bool Update();
	void Shutdown();

	float GetTimeElapsed();
	void WriteError(std::string message);
}
