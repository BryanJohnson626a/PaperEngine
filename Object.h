#pragma once
#include "Core.h"

namespace Engine
{
	class Object
	{
	public:
		static Object * NewObject(Sprite * sprite = nullptr);

		static std::array<Object, MAX_OBJECTS> & GetObjects();
		static int GetNumObjects();

		Transform * transform;
		Sprite * sprite;
	private:
	};
}

