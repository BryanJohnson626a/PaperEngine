#pragma once
#include "Core.h"

namespace Engine
{
	namespace Input
	{
		enum class Key
		{
			MoveUp,
			MoveDown,
			MoveLeft,
			MoveRight
		};

		void Initialize();
		void Update();
		void Shutdown();

		bool IsPressed(Key key);
		bool IsDown(Key key);
		bool IsReleased(Key key);
	}
}

