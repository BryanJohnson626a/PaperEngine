#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <format>

namespace Engine
{
	const int MAX_SPRITES = 100;
	const int MAX_TEXTURES = 100;
	const int MAX_OBJECTS = 500;

	class Texture;
	class Sprite;
	class Transform;

	using Radians = float;

	extern void(*PreInitialization)();
	extern void(*PostInitialization)();
	extern void(*PreUpdate)();
	extern void(*PostUpdate)();
	extern void(*PreShutdown)();
	extern void(*PostShutdown)();

	void Initialize();
	bool Update();
	void Shutdown();

	float GetStartTime();
	float GetTimeElapsed();
	float GetDeltaTime();
	void WriteError(std::string message);
}
