#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>
#include <array>

#include "Core.h"

namespace Engine
{
	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	namespace Graphics
	{
		const VkClearColorValue clear_color = { { .1f, .1f, .2f, 1.f } };

		const int MAX_FRAMES_IN_FLIGHT = 2;

		const std::vector<const char *> validation_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char *> device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

#ifdef NDEBUG
		const bool enable_validation_layers = false;
#else
		const bool enable_validation_layers = true;
#endif

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics_family;
			std::optional<uint32_t> present_family;

			bool IsComplete() const;
		};

		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities{};
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> present_modes;
		};

		struct Camera
		{
			float zoom{ 1 };
			glm::vec2 position{ 0,0 };
		};

		struct Vertex
		{
		public:
			glm::vec3 position;
			glm::vec2 tex_coord;

			static VkVertexInputBindingDescription GetBindingDescription()
			{
				VkVertexInputBindingDescription binding_description{};
				binding_description.binding = 0;
				binding_description.stride = sizeof(Vertex);
				binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				return binding_description;
			}

			static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
			{
				std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

				attribute_descriptions[0].binding = 0;
				attribute_descriptions[0].location = 0;
				attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
				attribute_descriptions[0].offset = offsetof(Vertex, position);

				attribute_descriptions[1].binding = 0;
				attribute_descriptions[1].location = 1;
				attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
				attribute_descriptions[1].offset = offsetof(Vertex, tex_coord);

				return attribute_descriptions;
			}
		};

		void Initialize();
		void Update();
		void Shutdown();

		VkDevice GetDevice();
		GLFWwindow * GetWindow();
		glm::vec2 WindowSize();

		void CreateWindow();
		void CreateInstance();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateRenderPass();
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline();
		void CreateCommandPool();
		void CreateDepthResources();
		void CreateFramebuffers();
		void LoadTextures();
		void CreateTextureSampler();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreateCommandBuffers();
		void CreateSyncObjects();

		void UpdateUniformBuffer(uint32_t current_image);

		void CleanupSwapChain();
		void RecreateSwapChain();

		void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize buffer_size);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties, VkBuffer & buffer,
			VkDeviceMemory & buffer_memory);

		void TransitionImageLayout(VkImage image, VkFormat format,
			VkImageLayout old_layout, VkImageLayout new_layout);

		void CreateImage(uint32_t width, uint32_t height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
			VkImage & image, VkDeviceMemory & image_memory);

		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo);
		void SetupDebugMessenger();

		uint32_t SwapChainSize();
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities);

		std::vector<const char *> getRequiredExtensions();
		bool CheckValidationLayerSupport();
		SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice & device_candidate);
		bool IsDeviceSuitable(const VkPhysicalDevice & device_candidate);
		bool CheckDeviceExtensionSupport(const VkPhysicalDevice & device_candidate);
		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice & device_candidate);

		VkShaderModule CreateShaderModule(const std::vector<char> & code);
		static void FramebufferResizeCallback(GLFWwindow * window, int width, int height);

		void BeginSingleTimeCommands(VkCommandBuffer & command_buffer);
		void EndSingleTimeCommands(VkCommandBuffer command_buffer);

		static std::vector<char> ReadFile(const std::string & filename);
		uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
		VkFormat FindSupportedFormat(const std::vector<VkFormat> & candidates,
			VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat FindDepthFormat();
		bool HasStencilComponent(VkFormat format);

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
			const VkAllocationCallbacks * pAllocator,
			VkDebugUtilsMessengerEXT * pDebugMessenger);

		void DestroyDebugUtilsMessengerEXT(VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks * pAllocator);

		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
			void * pUserData);
	};


}