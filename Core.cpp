#include <iostream>
#include <chrono>

#include "Renderer.h"
#include "Input.h"


#ifdef _WIN32
#define PAUSE system("pause")
#else
#define PAUSE 
#endif


namespace Engine
{
	int errors = 0;

	float delta_time;

	void(*PreInitialization)();
	void(*PostInitialization)();
	void(*PreUpdate)();
	void(*PostUpdate)();
	void(*PreShutdown)();
	void(*PostShutdown)();

	void Initialize()
	{
		try
		{
			if (PreInitialization)
				PreInitialization();

			Graphics::Initialize();
			Input::Initialize();

			if (PostInitialization != nullptr)
				PostInitialization();
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
		static auto previous_time = std::chrono::high_resolution_clock::now();

		auto current_time = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - previous_time).count();
		previous_time = current_time;

		try
		{
			if (PreUpdate) PreUpdate();

			Input::Update();
			Graphics::Update();

			if (PostUpdate) PostUpdate();
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
			if (PreShutdown) PreShutdown();

			Input::Shutdown();
			Graphics::Shutdown();

			if (PostShutdown) PostShutdown();
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
	float GetDeltaTime()
	{
		return delta_time;
	}
}