#pragma once
#include "Core.h"

namespace Engine
{
	class Transform
	{
	public:
		static Transform * NewTransform();

		glm::vec2 GetPosition();
		void SetPosition(glm::vec2 new_position);
		void SetPosition(float x, float y);
		void Move(glm::vec2 distance);
		void Move(float x, float y);

		Radians GetRotation();
		void SetRotation(Radians new_rotation);

		glm::vec2 GetSize();
		void SetSize(glm::vec2 new_size);
		void SetSize(float new_size);
	private:
		glm::vec2 position{ 0,0 };
		glm::vec2 size{ 1,1 };
		Radians rotation{ 0 };
	};
}
