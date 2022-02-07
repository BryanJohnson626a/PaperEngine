#include <iostream>
#include <chrono>

#include "Renderer.h"


#ifdef _WIN32
#define PAUSE system("pause")
#else
#define PAUSE 
#endif


namespace Engine
{
	int errors = 0;

	void Initialize()
	{
		try
		{
			Graphics::Initialize();
		}
		catch (const std::exception & e)
		{
			std::cerr << "FATAL ERROR: " << e.what() << std::endl;
			PAUSE;
			exit(EXIT_FAILURE);
		}
	}

	bool Update()
	{
		glfwPollEvents();

		try
		{
			Graphics::Update();
		}
		catch (const std::exception & e)
		{
			std::cerr << "FATAL ERROR: " << e.what() << std::endl;
			PAUSE;
			exit(EXIT_FAILURE);
		}

		return !glfwWindowShouldClose(Graphics::GetWindow());
	}

	void Shutdown()
	{
		try
		{
			Graphics::Shutdown();
		}
		catch (const std::exception & e)
		{
			std::cerr << "FATAL ERROR: " << e.what() << std::endl;
			PAUSE;
			exit(EXIT_FAILURE);
		}

		if (errors > 0)
			PAUSE;
	}

	float GetTimeElapsed()
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		return std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
	}

	void WriteError(std::string message)
	{
		++errors;
		std::cerr << message << "\n\n";
	}
}