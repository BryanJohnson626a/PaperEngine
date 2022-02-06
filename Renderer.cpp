#include "Renderer.h"
#include "Texture.h"

#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <format>
#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <set>

namespace Engine
{
	bool Graphics::framebuffer_resized = false;

	std::vector<Texture> textures;

	const std::vector<Graphics::Vertex> vertices
	{
		{ { -.5f, -.5f, 0 }, { 1 / 8.f, 0        } },
		{ {  .5f, -.5f, 0 }, { 0,       0        } },
		{ {  .5f,  .5f, 0 }, { 0,       1 / 16.f } },
		{ { -.5f,  .5f, 0 }, { 1 / 8.f, 1 / 16.f } }
	};

	const std::vector<uint32_t> indices
	{
		0, 1, 2,
		2, 3, 0
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	void Graphics::Initialize()
	{
		CreateWindow();
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateCommandPool();
		CreateDepthResources();
		CreateFramebuffers();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void Graphics::Update()
	{
		vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
			image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}
		else if (!(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR))
			throw std::runtime_error("Failed to acquire swap chain image.");

		if (images_in_flight[image_index] != VK_NULL_HANDLE)
			vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);

		images_in_flight[image_index] = in_flight_fences[current_frame];

		UpdateUniformBuffer(image_index);

		const std::array<VkSemaphore, 1> wait_semaphores{ image_available_semaphores[current_frame] };
		const std::array<VkPipelineStageFlags, 1> wait_stages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		const std::array<VkSemaphore, 1> signal_semaphores{ render_finished_semaphores[current_frame] };

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = uint32_t(wait_semaphores.size());
		submit_info.pWaitSemaphores = wait_semaphores.data();
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers[image_index];
		submit_info.signalSemaphoreCount = uint32_t(signal_semaphores.size());
		submit_info.pSignalSemaphores = signal_semaphores.data();

		vkResetFences(device, 1, &in_flight_fences[current_frame]);

		if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
			throw std::runtime_error("Failed to submit draw command buffer.");

		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = uint32_t(wait_semaphores.size());
		present_info.pWaitSemaphores = signal_semaphores.data();
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &swap_chain;
		present_info.pImageIndices = &image_index;

		result = vkQueuePresentKHR(present_queue, &present_info);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
		{
			framebuffer_resized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to present swap chain image.");

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

	}

	void Graphics::Shutdown()
	{
		vkDeviceWaitIdle(device);

		CleanupSwapChain();

		vkDestroySampler(device, texture_sampler, nullptr);
		vkDestroyImageView(device, texture_image_view, nullptr);

		vkDestroyImage(device, texture_image, nullptr);
		vkFreeMemory(device, texture_image_memory, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);

		vkDestroyBuffer(device, vertex_buffer, nullptr);
		vkFreeMemory(device, vertex_buffer_memory, nullptr);

		vkDestroyBuffer(device, index_buffer, nullptr);
		vkFreeMemory(device, index_buffer_memory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
			vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
			vkDestroyFence(device, in_flight_fences[i], nullptr);
		}

		vkDestroyCommandPool(device, command_pool, nullptr);

		vkDestroyDevice(device, nullptr);

		if (enable_validation_layers)
			DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	GLFWwindow * Graphics::GetWindow()
	{
		return window;
	}

	inline glm::vec2 Graphics::WindowSize() const
	{
		return glm::vec2(swap_chain_extent.width, swap_chain_extent.height);
	}

	inline uint32_t Graphics::SwapChainSize() const
	{
		return uint32_t(swap_chain_images.size());
	}

	void Graphics::FramebufferResizeCallback(GLFWwindow * /*window*/, int /*width*/, int /*height*/)
	{

		framebuffer_resized = true;
	}

	void Graphics::CleanupSwapChain()
	{
		vkDestroyImageView(device, depth_image_view, nullptr);
		vkDestroyImage(device, depth_image, nullptr);
		vkFreeMemory(device, depth_image_memory, nullptr);

		for (auto framebuffer : swap_chain_framebuffers)
			vkDestroyFramebuffer(device, framebuffer, nullptr);

		vkFreeCommandBuffers(device, command_pool, uint32_t(command_buffers.size()), command_buffers.data());

		vkDestroyPipeline(device, graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
		vkDestroyRenderPass(device, render_pass, nullptr);

		for (auto image_view : swap_chain_image_views)
			vkDestroyImageView(device, image_view, nullptr);

		vkDestroySwapchainKHR(device, swap_chain, nullptr);

		for (size_t i = 0; i < SwapChainSize(); i++)
		{
			vkDestroyBuffer(device, uniform_buffers[i], nullptr);
			vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	}

	void Graphics::CreateDepthResources()
	{
		VkFormat depth_format = FindDepthFormat();
		CreateImage(swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
		depth_image_view = CreateImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
		TransitionImageLayout(depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	}

	void Graphics::RecreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		CleanupSwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateDepthResources();
		CreateFramebuffers();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();

		images_in_flight.resize(SwapChainSize(), VK_NULL_HANDLE);
	}

	void Graphics::CreateWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	}

	void Graphics::CreateInstance()
	{
		if (enable_validation_layers && !CheckValidationLayerSupport())
			throw std::runtime_error("Validation layers requested, but not available.");

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Survivor";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.pEngineName = "Paper";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo creation_info{};
		creation_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		creation_info.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		creation_info.enabledExtensionCount = uint32_t(extensions.size());
		creation_info.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debug_creation_info{};
		if (enable_validation_layers)
		{
			creation_info.enabledLayerCount = uint32_t(validation_layers.size());
			creation_info.ppEnabledLayerNames = validation_layers.data();

			PopulateDebugMessengerCreateInfo(debug_creation_info);
			creation_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_creation_info;
		}
		else
		{
			creation_info.enabledLayerCount = 0;

			creation_info.pNext = nullptr;
		}

		if (vkCreateInstance(&creation_info, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("Failed to create instance.");
	}

	void Graphics::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	void Graphics::SetupDebugMessenger()
	{
		if (!enable_validation_layers) return;

		VkDebugUtilsMessengerCreateInfoEXT creation_info;
		PopulateDebugMessengerCreateInfo(creation_info);

		if (CreateDebugUtilsMessengerEXT(instance, &creation_info, nullptr, &debug_messenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger.");
	}

	void Graphics::CreateSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface.");
	}

	void Graphics::PickPhysicalDevice()
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count == 0)
			throw std::runtime_error("Failed to find GPU with Vulkan support.");

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

		physical_device = VK_NULL_HANDLE;
		for (const auto & device_candidate : devices)
			if (IsDeviceSuitable(device_candidate))
			{
				physical_device = device_candidate;
				break;
			}

		if (physical_device == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU.");

		vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
	}

	void Graphics::CreateLogicalDevice()
	{
		QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device);

		std::vector<VkDeviceQueueCreateInfo> queue_creation_infos;
		std::set<uint32_t> unique_queue_families = {
			queue_family_indices.graphics_family.value(),
			queue_family_indices.present_family.value()
		};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_creation_info{};
			queue_creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_creation_info.queueFamilyIndex = queueFamily;
			queue_creation_info.queueCount = 1;
			queue_creation_info.pQueuePriorities = &queuePriority;
			queue_creation_infos.push_back(queue_creation_info);
		}

		VkPhysicalDeviceFeatures device_features{};
		device_features.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo creation_info{};
		creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		creation_info.queueCreateInfoCount = uint32_t(queue_creation_infos.size());
		creation_info.pQueueCreateInfos = queue_creation_infos.data();

		creation_info.pEnabledFeatures = &device_features;

		creation_info.enabledExtensionCount = uint32_t(device_extensions.size());
		creation_info.ppEnabledExtensionNames = device_extensions.data();

		if (enable_validation_layers)
		{
			creation_info.enabledLayerCount = uint32_t(validation_layers.size());
			creation_info.ppEnabledLayerNames = validation_layers.data();
		}
		else
			creation_info.enabledLayerCount = 0;

		if (vkCreateDevice(physical_device, &creation_info, nullptr, &device) != VK_SUCCESS)
			throw std::runtime_error("Failed to create logical device.");

		vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
		vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
	}

	void Graphics::CreateSwapChain()
	{
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device);

		VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
		VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

		// We want at least min+1 images, but can't exceed the max image count if there is one.
		uint32_t min_image_count = swap_chain_support.capabilities.minImageCount + 1;
		if (swap_chain_support.capabilities.maxImageCount > 0 &&
			min_image_count > swap_chain_support.capabilities.maxImageCount)
		{
			min_image_count = swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swap_chain_info{};
		swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_chain_info.surface = surface;
		swap_chain_info.minImageCount = min_image_count;
		swap_chain_info.imageFormat = surface_format.format;
		swap_chain_info.imageColorSpace = surface_format.colorSpace;
		swap_chain_info.imageExtent = extent;
		swap_chain_info.imageArrayLayers = 1;
		swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device);
		uint32_t queueFamilyIndices[] = {
			queue_family_indices.graphics_family.value(),
			queue_family_indices.present_family.value()
		};

		if (queue_family_indices.graphics_family == queue_family_indices.present_family)
		{
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swap_chain_info.queueFamilyIndexCount = 0;
			swap_chain_info.pQueueFamilyIndices = nullptr;
		}
		else
		{
			swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swap_chain_info.queueFamilyIndexCount = 2;
			swap_chain_info.pQueueFamilyIndices = queueFamilyIndices;
		}

		swap_chain_info.preTransform = swap_chain_support.capabilities.currentTransform;
		swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_chain_info.presentMode = present_mode;
		swap_chain_info.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(device, &swap_chain_info, nullptr, &swap_chain) != VK_SUCCESS)
			throw std::runtime_error("Failed to create swap chain.");

		uint32_t image_count;
		vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
		swap_chain_images.resize(image_count);
		vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data());

		swap_chain_image_format = surface_format.format;
		swap_chain_extent = extent;
	}

	void Graphics::CreateImageViews()
	{
		swap_chain_image_views.resize(SwapChainSize());

		for (size_t i = 0; i < SwapChainSize(); i++)
			swap_chain_image_views[i] = CreateImageView(swap_chain_images[i], swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void Graphics::CreateRenderPass()
	{
		VkAttachmentDescription color_attach{};
		color_attach.format = swap_chain_image_format;
		color_attach.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attach_ref{};
		color_attach_ref.attachment = 0;
		color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attach{};
		depth_attach.format = FindDepthFormat();
		depth_attach.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attach_ref{};
		depth_attach_ref.attachment = 1;
		depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attach_ref;
		subpass.pDepthStencilAttachment = &depth_attach_ref;

		VkSubpassDependency subpass_dependency{};
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency.dstSubpass = 0;
		subpass_dependency.srcAccessMask = 0;
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments{ color_attach, depth_attach };

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = uint32_t(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &subpass_dependency;

		if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create render pass.");
	}

	void Graphics::CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding ubo_layout_binding{};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding sampler_layout_binding{};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };

		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = uint32_t(bindings.size());
		layout_info.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout.");
	}

	void Graphics::CreateGraphicsPipeline()
	{
		VkShaderModule vertex_shader_module = CreateShaderModule(ReadFile("shaders/vert.spv"));
		VkShaderModule fragment_shader_module = CreateShaderModule(ReadFile("shaders/frag.spv"));

		VkPipelineShaderStageCreateInfo vertex_shader_stage_info{};
		vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertex_shader_stage_info.module = vertex_shader_module;
		vertex_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo fragment_shader_stage_info{};
		fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragment_shader_stage_info.module = fragment_shader_module;
		fragment_shader_stage_info.pName = "main";

		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
			vertex_shader_stage_info,
			fragment_shader_stage_info
		};

		auto binding_description = Vertex::GetBindingDescription();
		auto attribute_descriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = uint32_t(attribute_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = WindowSize().x;
		viewport.height = WindowSize().y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent;

		VkPipelineViewportStateCreateInfo viewport_info{};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.pViewports = &viewport;
		viewport_info.scissorCount = 1;
		viewport_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer_info{};
		rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_info.depthClampEnable = VK_FALSE;
		rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
		rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer_info.lineWidth = 1.0f;
		rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer_info.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling_info{};
		multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_info.sampleShadingEnable = VK_FALSE;
		multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blend_info{};
		color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_info.logicOpEnable = VK_FALSE;
		color_blend_info.attachmentCount = 1;
		color_blend_info.pAttachments = &color_blend_attachment;

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
		pipeline_layout_info.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout.");

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state{};
		depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_state.depthTestEnable = VK_TRUE;
		depth_stencil_state.depthWriteEnable = VK_TRUE;
		depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_state.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = uint32_t(shader_stages.size());
		pipeline_info.pStages = shader_stages.data();
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly_info;
		pipeline_info.pViewportState = &viewport_info;
		pipeline_info.pRasterizationState = &rasterizer_info;
		pipeline_info.pMultisampleState = &multisampling_info;
		pipeline_info.pDepthStencilState = &depth_stencil_state;
		pipeline_info.pColorBlendState = &color_blend_info;
		pipeline_info.pDynamicState = nullptr;
		pipeline_info.layout = pipeline_layout;
		pipeline_info.renderPass = render_pass;
		pipeline_info.subpass = 0;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline.");

		vkDestroyShaderModule(device, fragment_shader_module, nullptr);
		vkDestroyShaderModule(device, vertex_shader_module, nullptr);
	}

	void Graphics::CreateFramebuffers()
	{
		swap_chain_framebuffers.resize(swap_chain_image_views.size());

		for (size_t i = 0; i < swap_chain_image_views.size(); i++)
		{
			std::array<VkImageView, 2> attachments{ swap_chain_image_views[i], depth_image_view };

			VkFramebufferCreateInfo framebuffer_info{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = render_pass;
			framebuffer_info.attachmentCount = uint32_t(attachments.size());
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = swap_chain_extent.width;
			framebuffer_info.height = swap_chain_extent.height;
			framebuffer_info.layers = 1;

			if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer.");
		}
	}

	void Graphics::CreateCommandPool()
	{
		QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device);

		VkCommandPoolCreateInfo command_pool_info{};
		command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

		if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool.");
	}

	void Graphics::CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize buffer_size)
	{
		VkCommandBuffer cb;
		BeginSingleTimeCommands(cb);

		VkBufferCopy copy_region{};
		copy_region.size = buffer_size;
		vkCmdCopyBuffer(cb, src_buffer, dst_buffer, 1, &copy_region);

		EndSingleTimeCommands(cb);
	}

	void Graphics::CreateVertexBuffer()
	{
		VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		void * data;
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, vertices.data(), size_t(buffer_size));
		vkUnmapMemory(device, staging_buffer_memory);

		CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);

		CopyBuffer(staging_buffer, vertex_buffer, buffer_size);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
	}

	void Graphics::CreateTextureImage()
	{
		std::string filename = "assets/DawnLike/Characters/Player0.png";

		int texture_width, texture_height, texture_channels;
		stbi_uc * pixels = stbi_load(filename.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
		VkDeviceSize image_size = texture_width * texture_height * texture_channels;

		if (!pixels)
			throw std::runtime_error(std::format("Failed to load {}.", filename));

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		void * data;
		vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
		memcpy(data, pixels, size_t(image_size));
		vkUnmapMemory(device, staging_buffer_memory);

		stbi_image_free(pixels);

		CreateImage(texture_width, texture_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			texture_image, texture_image_memory);

		TransitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(staging_buffer, texture_image, uint32_t(texture_width), uint32_t(texture_height));
		TransitionImageLayout(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
	}

	void Graphics::CreateTextureImageView()
	{
		texture_image_view = CreateImageView(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void Graphics::CreateIndexBuffer()
	{
		VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;

		CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer, staging_buffer_memory);

		void * data;
		vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
		memcpy(data, indices.data(), size_t(buffer_size));
		vkUnmapMemory(device, staging_buffer_memory);

		CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);

		CopyBuffer(staging_buffer, index_buffer, buffer_size);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, staging_buffer_memory, nullptr);
	}

	void Graphics::CreateUniformBuffers()
	{
		VkDeviceSize buffer_size = sizeof(UniformBufferObject);

		uniform_buffers.resize(SwapChainSize());
		uniform_buffers_memory.resize(SwapChainSize());

		for (size_t i = 0; i < SwapChainSize(); ++i)
			CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uniform_buffers[i], uniform_buffers_memory[i]);
	}

	void Graphics::CreateTextureSampler()
	{
		VkSamplerCreateInfo sampler_info{};
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy;
		sampler_info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 0.0f;

		if (vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture sampler.");
	}

	void Graphics::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> pool_sizes{};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = uint32_t(SwapChainSize());
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = uint32_t(SwapChainSize());

		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = uint32_t(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = uint32_t(SwapChainSize());

		if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor pool.");
	}

	void Graphics::CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(SwapChainSize(), descriptor_set_layout);

		VkDescriptorSetAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorPool = descriptor_pool;
		allocate_info.descriptorSetCount = int32_t(SwapChainSize());
		allocate_info.pSetLayouts = layouts.data();

		descriptor_sets.resize(SwapChainSize());
		if (vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate descriptor sets.");

		for (size_t i = 0; i < SwapChainSize(); ++i)
		{
			VkDescriptorBufferInfo buffer_info{};
			buffer_info.buffer = uniform_buffers[i];
			buffer_info.offset = 0;
			buffer_info.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo image_info{};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = texture_image_view;
			image_info.sampler = texture_sampler;

			std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = descriptor_sets[i];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;

			descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[1].dstSet = descriptor_sets[i];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pImageInfo = &image_info;

			vkUpdateDescriptorSets(device, uint32_t(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
		}
	}

	void Graphics::CreateCommandBuffers()
	{
		std::array<VkClearValue, 2> clear_values;
		clear_values[0].color = clear_color;
		clear_values[1].depthStencil = { 1.f, 0 };

		command_buffers.resize(swap_chain_framebuffers.size());

		VkCommandBufferAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool = command_pool;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

		if (vkAllocateCommandBuffers(device, &allocate_info, command_buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers.");

		for (size_t i = 0; i < command_buffers.size(); ++i)
		{
			VkRenderPassBeginInfo render_pass_info{};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = render_pass;
			render_pass_info.framebuffer = swap_chain_framebuffers[i];
			render_pass_info.renderArea.offset = { 0, 0 };
			render_pass_info.renderArea.extent = swap_chain_extent;
			render_pass_info.clearValueCount = uint32_t(clear_values.size());
			render_pass_info.pClearValues = clear_values.data();

			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = 0;

			std::array<VkBuffer, 1> vertex_buffers{ vertex_buffer };
			std::array<VkDeviceSize, 1> offsets{ 0 };

			if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin recording command buffer.");

			vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
			vkCmdBindVertexBuffers(command_buffers[i], 0, uint32_t(vertex_buffers.size()), vertex_buffers.data(), offsets.data());
			vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i], 0, nullptr);
			vkCmdDrawIndexed(command_buffers[i], uint32_t(indices.size()), 1, 0, 0, 0);

			vkCmdEndRenderPass(command_buffers[i]);

			if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
				throw std::runtime_error("Failed to record command buffer.");
		}
	}

	void Graphics::CreateSyncObjects()
	{
		image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		images_in_flight.resize(SwapChainSize(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create synchronization objects for a frame.");
			}
		}
	}

	VkShaderModule Graphics::CreateShaderModule(const std::vector<char> & code)
	{
		VkShaderModuleCreateInfo shader_module_info{};
		shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_info.codeSize = code.size();
		shader_module_info.pCode = reinterpret_cast<const uint32_t *>(code.data());

		VkShaderModule shader_module;
		if (vkCreateShaderModule(device, &shader_module_info, nullptr, &shader_module) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module.");

		return shader_module;
	}

	void Graphics::UpdateUniformBuffer(uint32_t current_image)
	{
		float time = Engine::GetTimeElapsed();

		float aspect = float(WindowSize().x) / float(WindowSize().y);

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1), time * glm::radians(90.f), glm::vec3(0, 0, 1));
		ubo.view = glm::lookAt(glm::vec3(1, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
		ubo.projection = glm::perspective(glm::radians(45.f), aspect, 0.1f, 10.f);
		ubo.projection[1][1] *= -1; // Unflip Y for vulkan compatability.

		void * data;
		vkMapMemory(device, uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniform_buffers_memory[current_image]);
	}

	VkSurfaceFormatKHR Graphics::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & available_formats)
	{
		for (const auto & format : available_formats)
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;

		return available_formats[0];
	}

	VkPresentModeKHR Graphics::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes)
	{
		for (const auto & available_present_mode : availablePresentModes)
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return available_present_mode;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Graphics::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actual_extent = {
				uint32_t(width),
				uint32_t(height)
			};

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}


	Graphics::SwapChainSupportDetails Graphics::QuerySwapChainSupport(const VkPhysicalDevice & device_candidate)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_candidate, surface, &details.capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device_candidate, surface, &format_count, nullptr);

		if (format_count > 0)
		{
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device_candidate, surface, &format_count, details.formats.data());
		}

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device_candidate, surface, &present_mode_count, nullptr);

		if (present_mode_count > 0)
		{
			details.present_modes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device_candidate, surface, &present_mode_count, details.present_modes.data());
		}

		return details;
	}

	bool Graphics::IsDeviceSuitable(const VkPhysicalDevice & device_candidate)
	{
		if (!FindQueueFamilies(device_candidate).IsComplete())
			return false;

		if (!CheckDeviceExtensionSupport(device_candidate))
			return false;

		SwapChainSupportDetails details = QuerySwapChainSupport(device_candidate);
		if (details.formats.empty() || details.present_modes.empty())
			return false;

		VkPhysicalDeviceFeatures supported_features;
		vkGetPhysicalDeviceFeatures(device_candidate, &supported_features);

		if (!supported_features.samplerAnisotropy)
			return false;

		return true;
	}

	bool Graphics::CheckDeviceExtensionSupport(const VkPhysicalDevice & device_candidate)
	{
		// Get the available extensions.
		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(device_candidate, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device_candidate, nullptr, &extension_count, available_extensions.data());

		// Make sure all required extensions are available.
		std::set<std::string> missing_extensions(device_extensions.begin(), device_extensions.end());
		for (const auto & extension : available_extensions)
			missing_extensions.erase(extension.extensionName);

		return missing_extensions.empty();
	}

	Graphics::QueueFamilyIndices Graphics::FindQueueFamilies(const VkPhysicalDevice & device_candidate)
	{
		QueueFamilyIndices queue_family_indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device_candidate, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device_candidate, &queue_family_count, queue_families.data());

		for (int i = 0; i < queue_families.size(); ++i)
		{
			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				queue_family_indices.graphics_family = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device_candidate, i, surface, &presentSupport);

			if (presentSupport)
				queue_family_indices.present_family = i;

			if (queue_family_indices.IsComplete())
				break;
		}

		return queue_family_indices;
	}
	std::vector<const char *> Graphics::getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char ** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enable_validation_layers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	void Graphics::CreateBuffer(VkDeviceSize buffer_size, VkBufferUsageFlags buffer_usage,
		VkMemoryPropertyFlags memory_properties, VkBuffer & buffer, VkDeviceMemory & buffer_memory)
	{
		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = buffer_size;
		buffer_info.usage = buffer_usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
			throw std::runtime_error("Failed to create vertex buffer.");

		VkMemoryRequirements memory_requirements;
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

		VkMemoryAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = memory_requirements.size;
		allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, memory_properties);

		if (vkAllocateMemory(device, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate vertex buffer memory.");

		vkBindBufferMemory(device, buffer, buffer_memory, 0);
	}

	bool Graphics::CheckValidationLayerSupport()
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char * layerName : validation_layers)
		{
			bool layer_found = false;

			for (const auto & layer_properties : available_layers)
				if (strcmp(layerName, layer_properties.layerName) == 0)
				{
					layer_found = true;
					break;
				}

			if (!layer_found)
				return false;
		}

		return true;
	}

	void Graphics::BeginSingleTimeCommands(VkCommandBuffer & command_buffer)
	{
		VkCommandBufferAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandPool = command_pool;
		allocate_info.commandBufferCount = 1;

		vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &begin_info);
	}

	void Graphics::EndSingleTimeCommands(VkCommandBuffer command_buffer)
	{
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics_queue);

		vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
	}

	std::vector<char> Graphics::ReadFile(const std::string & filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error(std::format("Failed to open {}.", filename));

		size_t file_size = size_t(file.tellg());
		std::vector<char> buffer(file_size);

		file.seekg(0);
		file.read(buffer.data(), file_size);

		file.close();

		return buffer;
	}

	void Graphics::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		VkCommandBuffer cb;
		BeginSingleTimeCommands(cb);
		vkCmdCopyBufferToImage(cb, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		EndSingleTimeCommands(cb);
	}

	VkFormat Graphics::FindSupportedFormat(const std::vector<VkFormat> & candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				return format;
		}

		throw std::runtime_error("Failed to find supported format.");
	}

	VkFormat Graphics::FindDepthFormat()
	{
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool Graphics::HasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkImageView Graphics::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
	{
		VkImageViewCreateInfo image_view_info{};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = image;
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = format;
		image_view_info.subresourceRange.aspectMask = aspect_flags;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		VkImageView image_view;
		if (vkCreateImageView(device, &image_view_info, nullptr, &image_view) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture image view.");

		return image_view;
	}

	void Graphics::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
	{
		VkPipelineStageFlags src_stage;
		VkPipelineStageFlags dst_stage;
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HasStencilComponent(format))
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
			throw std::invalid_argument("Unsupported layout transition.");

		VkCommandBuffer cb;
		BeginSingleTimeCommands(cb);
		vkCmdPipelineBarrier(cb, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		EndSingleTimeCommands(cb);
	}

	void Graphics::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & image_memory)
	{
		VkImageCreateInfo image_info{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image.");

		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(device, image, &memory_requirements);

		VkMemoryAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = memory_requirements.size;
		allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate image memory.");

		vkBindImageMemory(device, image, image_memory, 0);
	}

	uint32_t Graphics::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
			if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
				return i;

		throw std::runtime_error("Failed to find suitable memory type.");
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT /*message_type*/, const VkDebugUtilsMessengerCallbackDataEXT * callback_data, void * /*user_data*/)
	{
		if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			WriteError(std::format("Vulkan Error: {}", callback_data->pMessage));
		else if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			WriteError(std::format("Vulkan Warning: {}", callback_data->pMessage));

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	inline bool Graphics::QueueFamilyIndices::IsComplete() const
	{
		return graphics_family.has_value() && present_family.has_value();
	}
}