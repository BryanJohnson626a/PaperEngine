#include "Input.h"
#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

namespace Engine
{
	namespace Input
	{
		std::map<Key, int> key_map
		{
			{Key::MoveUp, GLFW_KEY_W},
			{Key::MoveDown, GLFW_KEY_S},
			{Key::MoveLeft, GLFW_KEY_A},
			{Key::MoveRight, GLFW_KEY_D},
		};

		std::map<Key, bool> key_pressed;
		std::map<Key, bool> key_down;
		std::map<Key, bool> key_released;

		void Initialize()
		{
			for (auto k : key_map)
			{
				key_pressed[k.first] = false;
				key_down[k.first] = false;
				key_released[k.first] = false;
			}
		}

		void Update()
		{
			glfwPollEvents();

			for (auto k : key_map)
			{
				auto state = glfwGetKey(Graphics::GetWindow(), k.second);
				if (state == GLFW_PRESS)
				{
					// If key wasn't down last frame, it was just pressed.
					if (key_down[k.first] == false)
						key_pressed[k.first] = true;
					else
						key_pressed[k.first] = false;

					key_down[k.first] = true;
					key_released[k.first] = false;
				}
				else
				{
					// If key was down last frame, it was just released.
					if (key_down[k.first] == true)
						key_released[k.first] = true;
					else
						key_released[k.first] = false;

					key_down[k.first] = false;
					key_pressed[k.first] = false;
				}
			}
		}

		void Shutdown()
		{
		}

		bool IsPressed(Key key)
		{
			return key_pressed[key];
		}
		bool IsDown(Key key)
		{
			return key_down[key];
		}
		bool IsReleased(Key key)
		{
			return key_released[key];
		}
	}
}