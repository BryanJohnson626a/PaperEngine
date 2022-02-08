#include "Transform.h"

namespace Engine
{
	std::array<Transform, MAX_OBJECTS> all_transforms;
	int num_transforms = 0;

	Transform * Transform::NewTransform()
	{
		Transform * transform = &all_transforms[num_transforms++];
		return transform;
	}

	glm::vec2 Transform::GetPosition()
	{
		return position;
	}
	void Transform::SetPosition(glm::vec2 new_position)
	{
		position = new_position;
	}
	void Transform::SetPosition(float x, float y)
	{
		SetPosition(glm::vec2(x, y));
	}
	void Transform::Move(glm::vec2 distance)
	{
		position += distance;
	}
	void Transform::Move(float x, float y)
	{
		Move(glm::vec2(x, y));
	}
	Radians Transform::GetRotation()
	{
		return rotation;
	}
	void Transform::SetRotation(Radians new_rotation)
	{
		rotation = new_rotation;
	}
	glm::vec2 Transform::GetSize()
	{
		return size;
	}
	void Transform::SetSize(glm::vec2 new_size)
	{
		size = new_size;
	}
	void Transform::SetSize(float new_size)
	{
		SetSize(glm::vec2(new_size));
	}
}