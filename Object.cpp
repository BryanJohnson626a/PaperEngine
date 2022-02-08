#include "Object.h"
#include "Sprite.h"
#include "Transform.h"

namespace Engine
{
	std::array<Object, MAX_OBJECTS> all_objects;
	int num_objects = 0;

	Object * Object::NewObject(Sprite * sprite)
	{
		Object * object = &all_objects[num_objects++];

		object->transform = Transform::NewTransform();

		if (sprite == nullptr)
		{
			sprite = Sprite::NewSprite();
		}
		object->sprite = sprite;

		return object;
	}

	std::array<Object, MAX_OBJECTS> & Object::GetObjects()
	{
		return all_objects;
	}
	int Object::GetNumObjects()
	{
		return num_objects;
	}
}
