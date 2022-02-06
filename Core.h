#pragma once
#include <vector>
#include <string>

namespace Engine
{
	class Texture;

	void Initialize();
	bool Update();
	void Shutdown();

	float GetTimeElapsed();
	void WriteError(std::string message);
}
