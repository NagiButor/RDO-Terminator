#define IMGUI_DEFINE_MATH_OPERATORS
#include "Renderer.hpp"

#include "core/memory/ModuleMgr.hpp"
#include "core/memory/PatternScanner.hpp"
#include "game/frontend/GUI.hpp"
#include "game/frontend/Menu.hpp"
#include "game/frontend/resources/resource.h"
#include "game/pointers/Pointers.hpp"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <wincodec.h>

#include <vector>


namespace YimMenu
{
	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::DestroyImpl()
	{
		ImGui_ImplWin32_Shutdown();

		GetInstance().DestroyLogoTexture();

		if (Pointers.IsVulkan)
		{
			VkCleanupRenderTarget();
			vkDestroyDescriptorPool(m_VkDevice, m_VkDescriptorPool, m_VkAllocator);
			vkDeviceWaitIdle(m_VkDevice);
			vkDestroyInstance(m_VkInstance, m_VkAllocator);
			ImGui_ImplVulkan_Shutdown();
		}
		else if (!Pointers.IsVulkan)
		{
			WaitForLastFrame();
			ImGui_ImplDX12_Shutdown();
		}

		ImGui::DestroyContext();
	}

	bool Renderer::InitDX12()
	{
		if (!Pointers.SwapChain)
		{
			LOG(WARNING) << "SwapChain pointer is invalid!";

			return false;
		}

		if (!Pointers.CommandQueue)
		{
			LOG(WARNING) << "CommandQueue pointer is invalid!";

			return false;
		}

		//This is required. In order to stop ComPtr from releasing the original pointer, we create a new ComPtr with the ptr as the intializer. (The '=' operator uses swap which releases the object passed into it)
		if (m_GameSwapChain = ComPtr<IDXGISwapChain1>(*Pointers.SwapChain); !m_GameSwapChain.Get())
		{
			LOG(WARNING) << "Dereferenced SwapChain pointer is invalid!";

			return false;
		}

		if (m_CommandQueue = ComPtr<ID3D12CommandQueue>(*Pointers.CommandQueue); !m_CommandQueue.Get())
		{
			LOG(WARNING) << "Dereferenced CommandQueue pointer is invalid!";

			return false;
		}

		m_GameSwapChain.As(&m_SwapChain); //We need GetCurrentBackBufferIndex from IDXGISwapChain3

		if (const auto result = m_SwapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(m_Device.GetAddressOf())); result < 0)
		{
			LOG(WARNING) << "Failed to get D3D Device with result: [" << result << "]";

			return false;
		}

		if (const auto result = m_SwapChain->GetDesc(&m_SwapChainDesc); result < 0)
		{
			LOG(WARNING) << "Failed to get SwapChain Description with result: [" << result << "]";

			return false;
		}

		if (const auto result = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)m_Fence.GetAddressOf()); result < 0)
		{
			LOG(WARNING) << "Failed to create Fence with result: [" << result << "]";

			return false;
		}

		if (const auto result = m_FenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr); !result)
		{
			LOG(WARNING) << "Failed to create Fence Event!";

			return false;
		}

		m_FrameContext.resize(m_SwapChainDesc.BufferCount);

		D3D12_DESCRIPTOR_HEAP_DESC DescriptorDesc{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_SwapChainDesc.BufferCount, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE};
		if (const auto result =
		        m_Device->CreateDescriptorHeap(&DescriptorDesc, __uuidof(ID3D12DescriptorHeap), (void**)m_DescriptorHeap.GetAddressOf());
		    result < 0)
		{
			LOG(WARNING) << "Failed to create Descriptor Heap with result: [" << result << "]";

			return false;
		}

		if (const auto result = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		        __uuidof(ID3D12CommandAllocator),
		        (void**)m_CommandAllocator.GetAddressOf());
		    result < 0)
		{
			LOG(WARNING) << "Failed to create Command Allocator with result: [" << result << "]";

			return false;
		}

		for (size_t i{}; i < m_SwapChainDesc.BufferCount; ++i)
		{
			m_FrameContext[i].CommandAllocator = m_CommandAllocator.Get();
		}

		if (const auto result = m_Device->CreateCommandList(0,
		        D3D12_COMMAND_LIST_TYPE_DIRECT,
		        m_CommandAllocator.Get(),
		        NULL,
		        __uuidof(ID3D12GraphicsCommandList),
		        (void**)m_CommandList.GetAddressOf());
		    result < 0)
		{
			LOG(WARNING) << "Failed to create Command List with result: [" << result << "]";

			return false;
		}

		if (const auto result = m_CommandList->Close(); result < 0)
		{
			LOG(WARNING) << "Failed to finalize the creation of Command List with result: [" << result << "]";

			return false;
		}

		D3D12_DESCRIPTOR_HEAP_DESC DescriptorBackbufferDesc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_SwapChainDesc.BufferCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1};
		if (const auto result = m_Device->CreateDescriptorHeap(&DescriptorBackbufferDesc,
		        __uuidof(ID3D12DescriptorHeap),
		        (void**)m_BackbufferDescriptorHeap.GetAddressOf());
		    result < 0)
		{
			LOG(WARNING) << "Failed to create Backbuffer Descriptor Heap with result: [" << result << "]";

			return false;
		}

		const auto RTVDescriptorSize{m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
		D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle{m_BackbufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};
		for (size_t i{}; i != m_SwapChainDesc.BufferCount; ++i)
		{
			ComPtr<ID3D12Resource> BackBuffer{};
			m_FrameContext[i].Descriptor = RTVHandle;
			m_SwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)BackBuffer.GetAddressOf());
			m_Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RTVHandle);
			m_FrameContext[i].Resource = BackBuffer.Get();
			RTVHandle.ptr += RTVDescriptorSize;
		}

		// never returns false, useless to check return
		ImGui::CreateContext(&GetInstance().m_FontAtlas);
		ImGui_ImplWin32_Init(*Pointers.Hwnd);
		ImGui_ImplDX12_Init(m_Device.Get(),
		    m_SwapChainDesc.BufferCount,
		    DXGI_FORMAT_R8G8B8A8_UNORM,
		    m_DescriptorHeap.Get(),
		    m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		    m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		ImGui::StyleColorsDark();

		LOG(INFO) << "DirectX 12 renderer has finished initializing.";
		return true;
	}

	bool Renderer::InitVulkan()
	{
		VkInstanceCreateInfo CreateInfo = {};

		const std::vector<const char*> InstanceExtensions = {"VK_KHR_surface"};

		const std::vector<const char*> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};

		CreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		CreateInfo.enabledExtensionCount   = (uint32_t)InstanceExtensions.size();
		CreateInfo.ppEnabledExtensionNames = InstanceExtensions.data();

		//	CreateInfo.enabledLayerCount       = (uint32_t)ValidationLayers.size();
		//	CreateInfo.ppEnabledLayerNames     = ValidationLayers.data();

		// Create Vulkan Instance without debug feature
		if (const VkResult result = vkCreateInstance(&CreateInfo, m_VkAllocator, &m_VkInstance); result != VK_SUCCESS)
		{
			LOG(WARNING) << "vkCreateInstance failed with result: [" << result << "]";
			return false;
		}

		;
		uint32_t GpuCount;
		if (const VkResult result = vkEnumeratePhysicalDevices(m_VkInstance, &GpuCount, NULL); result != VK_SUCCESS)
		{
			LOG(WARNING) << "vkEnumeratePhysicalDevices failed with result: [" << result << "]";
			return false;
		}

		IM_ASSERT(GpuCount > 0);

		ImVector<VkPhysicalDevice> GpuArr;
		GpuArr.resize(GpuCount);

		if (const VkResult result = vkEnumeratePhysicalDevices(m_VkInstance, &GpuCount, GpuArr.Data); result != VK_SUCCESS)
		{
			LOG(WARNING) << "vkEnumeratePhysicalDevices 2 failed with result: [" << result << "]";
			return false;
		}

		VkPhysicalDevice MainGPU = nullptr;
		for (const auto& Gpu : GpuArr)
		{
			VkPhysicalDeviceProperties Properties;
			vkGetPhysicalDeviceProperties(Gpu, &Properties);
			if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				LOG(INFO) << "Vulkan - Using GPU: " << Properties.deviceName;

				MainGPU = Gpu;
				break;
			}
		}

		if (!MainGPU)
		{
			LOG(INFO) << "Failed to get main GPU!";
			return false;
		}

		m_VkPhysicalDevice = MainGPU;

		uint32_t Count;
		vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &Count, NULL);
		m_VKQueueFamilies.resize(Count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_VkPhysicalDevice, &Count, m_VKQueueFamilies.data());
		for (uint32_t i = 0; i < Count; ++i)
		{
			if (m_VKQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_VkQueueFamily = i;
				break;
			}
		}

		IM_ASSERT(m_VkQueueFamily != (uint32_t)-1);

		constexpr const char* DeviceExtension = "VK_KHR_swapchain";
		constexpr const float QueuePriority   = 1.0f;

		VkDeviceQueueCreateInfo DeviceQueueInfo = {};
		DeviceQueueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		DeviceQueueInfo.queueFamilyIndex        = m_VkQueueFamily;
		DeviceQueueInfo.queueCount              = 1;
		DeviceQueueInfo.pQueuePriorities        = &QueuePriority;

		VkDeviceCreateInfo DeviceCreateInfo      = {};
		DeviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DeviceCreateInfo.queueCreateInfoCount    = 1;
		DeviceCreateInfo.pQueueCreateInfos       = &DeviceQueueInfo;
		DeviceCreateInfo.enabledExtensionCount   = 1;
		DeviceCreateInfo.ppEnabledExtensionNames = &DeviceExtension;

		if (const VkResult result = vkCreateDevice(m_VkPhysicalDevice, &DeviceCreateInfo, m_VkAllocator, &m_VkFakeDevice); result != VK_SUCCESS)
		{
			LOG(WARNING) << "Fake vkCreateDevice failed with result: [" << result << "]";
			return false;
		}

		Pointers.QueuePresentKHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(m_VkFakeDevice, "vkQueuePresentKHR"));
		Pointers.CreateSwapchainKHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(m_VkFakeDevice, "vkCreateSwapchainKHR"));
		Pointers.AcquireNextImageKHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(m_VkFakeDevice, "vkAcquireNextImageKHR"));
		Pointers.AcquireNextImage2KHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(m_VkFakeDevice, "vkAcquireNextImage2KHR"));

		vkDestroyDevice(m_VkFakeDevice, m_VkAllocator);
		m_VkFakeDevice = NULL;

		ImGui::CreateContext(&GetInstance().m_FontAtlas);
		ImGui_ImplWin32_Init(*Pointers.Hwnd);


		LOG(INFO) << "Vulkan renderer has finished initializing.";

		return true;
	}

	void Renderer::ResizeImpl(float scale)
	{
		DX12PreResize();

		if (scale != 1.0f)
			ImGui::GetStyle().ScaleAllSizes(scale);
		ImGui::GetStyle().MouseCursorScale = 1.0f;
		ImGui::GetIO().FontGlobalScale     = scale;
		DX12PostResize();
	}

	void Renderer::VkCreateRenderTarget(VkDevice device, VkSwapchainKHR swapchain)
	{
		uint32_t uImageCount;
		if (const VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &uImageCount, NULL))
		{
			LOG(WARNING) << "vkGetSwapchainImagesKHR failed with result: [" << result << "]";
			return;
		}

		VkImage BackBuffers[8] = {};
		if (const VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &uImageCount, BackBuffers))
		{
			LOG(WARNING) << "vkGetSwapchainImagesKHR 2 failed with result: [" << result << "]";
			return;
		}

		for (uint32_t i = 0; i < uImageCount; ++i)
		{
			m_VkFrames[i].Backbuffer = BackBuffers[i];

			ImGui_ImplVulkanH_Frame* fd            = &m_VkFrames[i];
			ImGui_ImplVulkanH_FrameSemaphores* fsd = &m_VkFrameSemaphores[i];
			{
				VkCommandPoolCreateInfo info = {};
				info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				info.queueFamilyIndex        = m_VkQueueFamily;

				if (const VkResult result = vkCreateCommandPool(device, &info, m_VkAllocator, &fd->CommandPool))
				{
					LOG(WARNING) << "vkCreateCommandPool failed with result: [" << result << "]";
					return;
				}
			}
			{
				VkCommandBufferAllocateInfo info = {};
				info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.commandPool                 = fd->CommandPool;
				info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				info.commandBufferCount          = 1;

				if (const VkResult result = vkAllocateCommandBuffers(device, &info, &fd->CommandBuffer))
				{
					LOG(WARNING) << "vkAllocateCommandBuffers failed with result: [" << result << "]";
					return;
				}
			}
			{
				VkFenceCreateInfo info = {};
				info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;
				if (const VkResult result = vkCreateFence(device, &info, m_VkAllocator, &fd->Fence))
				{
					LOG(WARNING) << "vkCreateFence failed with result: [" << result << "]";
					return;
				}
			}
			{
				VkSemaphoreCreateInfo info = {};
				info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				if (const VkResult result = vkCreateSemaphore(device, &info, m_VkAllocator, &fsd->ImageAcquiredSemaphore))
				{
					LOG(WARNING) << "vkCreateSemaphore failed with result: [" << result << "]";
					return;
				}

				if (const VkResult result = vkCreateSemaphore(device, &info, m_VkAllocator, &fsd->RenderCompleteSemaphore))
				{
					LOG(WARNING) << "vkCreateSemaphore 2 failed with result: [" << result << "]";
					return;
				}
			}
		}

		{
			VkAttachmentDescription attachment = {};
			attachment.format                  = VK_FORMAT_B8G8R8A8_UNORM;
			attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachment = {};
			color_attachment.attachment            = 0;
			color_attachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments    = &color_attachment;

			VkRenderPassCreateInfo info = {};
			info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount        = 1;
			info.pAttachments           = &attachment;
			info.subpassCount           = 1;
			info.pSubpasses             = &subpass;

			if (const VkResult result = vkCreateRenderPass(device, &info, m_VkAllocator, &m_VkRenderPass))
			{
				LOG(WARNING) << "vkCreateRenderPass failed with result: [" << result << "]";
				return;
			}
		}
		{
			VkImageViewCreateInfo info = {};
			info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
			info.format                = VK_FORMAT_B8G8R8A8_UNORM;

			info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.baseMipLevel   = 0;
			info.subresourceRange.levelCount     = 1;
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount     = 1;

			for (uint32_t i = 0; i < uImageCount; ++i)
			{
				ImGui_ImplVulkanH_Frame* fd = &m_VkFrames[i];
				info.image                  = fd->Backbuffer;

				if (const VkResult result = vkCreateImageView(device, &info, m_VkAllocator, &fd->BackbufferView))
				{
					LOG(WARNING) << "vkCreateImageView failed with result: [" << result << "]";
					return;
				}
			}
		}
		{
			VkImageView attachment[1];
			VkFramebufferCreateInfo info = {};
			info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass              = m_VkRenderPass;
			info.attachmentCount         = 1;
			info.pAttachments            = attachment;
			info.layers                  = 1;

			for (uint32_t i = 0; i < uImageCount; ++i)
			{
				ImGui_ImplVulkanH_Frame* fd = &m_VkFrames[i];
				attachment[0]               = fd->BackbufferView;

				if (const VkResult result = vkCreateFramebuffer(device, &info, m_VkAllocator, &fd->Framebuffer))
				{
					LOG(WARNING) << "vkCreateFramebuffer failed with result: [" << result << "]";
					return;
				}
			}
		}

		if (!m_VkDescriptorPool) // Create Descriptor Pool.
		{
			constexpr VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}, {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000}, {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000}, {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000}, {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000}, {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000}, {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets                    = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount              = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes                 = pool_sizes;

			if (const VkResult result = vkCreateDescriptorPool(device, &pool_info, m_VkAllocator, &m_VkDescriptorPool))
			{
				LOG(WARNING) << "vkCreateDescriptorPool failed with result: [" << result << "]";
				return;
			}
		}
	}

	void Renderer::VkCleanupRenderTarget()
	{
		for (uint32_t i = 0; i < RTL_NUMBER_OF(GetInstance().m_VkFrames); ++i)
		{
			if (GetInstance().m_VkFrames[i].Fence)
			{
				vkDestroyFence(GetInstance().m_VkDevice, GetInstance().m_VkFrames[i].Fence, GetInstance().m_VkAllocator);
				GetInstance().m_VkFrames[i].Fence = VK_NULL_HANDLE;
			}
			if (GetInstance().m_VkFrames[i].CommandBuffer)
			{
				vkFreeCommandBuffers(GetInstance().m_VkDevice,
				    GetInstance().m_VkFrames[i].CommandPool,
				    1,
				    &GetInstance().m_VkFrames[i].CommandBuffer);
				GetInstance().m_VkFrames[i].CommandBuffer = VK_NULL_HANDLE;
			}
			if (GetInstance().m_VkFrames[i].CommandPool)
			{
				vkDestroyCommandPool(GetInstance().m_VkDevice, GetInstance().m_VkFrames[i].CommandPool, GetInstance().m_VkAllocator);
				GetInstance().m_VkFrames[i].CommandPool = VK_NULL_HANDLE;
			}
			if (GetInstance().m_VkFrames[i].BackbufferView)
			{
				vkDestroyImageView(GetInstance().m_VkDevice, GetInstance().m_VkFrames[i].BackbufferView, GetInstance().m_VkAllocator);
				GetInstance().m_VkFrames[i].BackbufferView = VK_NULL_HANDLE;
			}
			if (GetInstance().m_VkFrames[i].Framebuffer)
			{
				vkDestroyFramebuffer(GetInstance().m_VkDevice, GetInstance().m_VkFrames[i].Framebuffer, GetInstance().m_VkAllocator);
				GetInstance().m_VkFrames[i].Framebuffer = VK_NULL_HANDLE;
			}
		}

		for (uint32_t i = 0; i < RTL_NUMBER_OF(GetInstance().m_VkFrameSemaphores); ++i)
		{
			if (GetInstance().m_VkFrameSemaphores[i].ImageAcquiredSemaphore)
			{
				vkDestroySemaphore(GetInstance().m_VkDevice,
				    GetInstance().m_VkFrameSemaphores[i].ImageAcquiredSemaphore,
				    GetInstance().m_VkAllocator);
				GetInstance().m_VkFrameSemaphores[i].ImageAcquiredSemaphore = VK_NULL_HANDLE;
			}
			if (GetInstance().m_VkFrameSemaphores[i].RenderCompleteSemaphore)
			{
				vkDestroySemaphore(GetInstance().m_VkDevice,
				    GetInstance().m_VkFrameSemaphores[i].RenderCompleteSemaphore,
				    GetInstance().m_VkAllocator);
				GetInstance().m_VkFrameSemaphores[i].RenderCompleteSemaphore = VK_NULL_HANDLE;
			}
		}

		if (IsResizing())
		{
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			ImGui::CreateContext(&GetInstance().m_FontAtlas);
			ImGui_ImplWin32_Init(*Pointers.Hwnd);
			Menu::SetupStyle();

			SetResizing(false);
		}
	}

	bool Renderer::DoesQueueSupportGraphic(VkQueue queue, VkQueue* pGraphicQueue)
	{
		for (uint32_t i = 0; i < m_VKQueueFamilies.size(); ++i)
		{
			const VkQueueFamilyProperties& family = m_VKQueueFamilies[i];
			for (uint32_t j = 0; j < family.queueCount; ++j)
			{
				VkQueue it = VK_NULL_HANDLE;
				vkGetDeviceQueue(m_VkDevice, i, j, &it);

				if (pGraphicQueue && family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					if (*pGraphicQueue == VK_NULL_HANDLE)
					{
						*pGraphicQueue = it;
					}
				}


				if (queue == it && family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					return true;
				}
			}
		}

		return false;
	}

	void Renderer::VkOnPresentImpl(VkQueue queue, const VkPresentInfoKHR* pPresentInfo)
	{
		if (!m_VkDevice || !g_Running || IsResizing())
		{
			return;
		}

		if (!ImGui::GetCurrentContext())
		{
			ImGui::CreateContext(&GetInstance().m_FontAtlas);
			ImGui_ImplWin32_Init(*Pointers.Hwnd);
		}

		VkQueue GraphicQueue            = VK_NULL_HANDLE;
		const bool QueueSupportsGraphic = DoesQueueSupportGraphic(queue, &GraphicQueue);
		if (GraphicQueue != VK_NULL_HANDLE)
			m_VkGraphicQueue = GraphicQueue;

		for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i)
		{
			VkSwapchainKHR swapchain = pPresentInfo->pSwapchains[i];
			if (m_VkFrames[0].Framebuffer == VK_NULL_HANDLE)
			{
				VkCreateRenderTarget(m_VkDevice, swapchain);
			}

			ImGui_ImplVulkanH_Frame* fd            = &m_VkFrames[pPresentInfo->pImageIndices[i]];
			ImGui_ImplVulkanH_FrameSemaphores* fsd = &m_VkFrameSemaphores[pPresentInfo->pImageIndices[i]];
			{
				if (const VkResult result = vkWaitForFences(m_VkDevice, 1, &fd->Fence, VK_TRUE, ~0ull); result != VK_SUCCESS)
				{
					LOG(WARNING) << "vkWaitForFences failed with result: [" << result << "]";
					return;
				}

				if (const VkResult result = vkResetFences(m_VkDevice, 1, &fd->Fence); result != VK_SUCCESS)
				{
					LOG(WARNING) << "vkResetFences failed with result: [" << result << "]";
					return;
				}
			}
			{
				if (const VkResult result = vkResetCommandBuffer(fd->CommandBuffer, 0); result != VK_SUCCESS)
				{
					LOG(WARNING) << "vkResetCommandBuffer failed with result: [" << result << "]";
					return;
				}

				VkCommandBufferBeginInfo info = {};
				info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				if (const VkResult result = vkBeginCommandBuffer(fd->CommandBuffer, &info); result != VK_SUCCESS)
				{
					LOG(WARNING) << "vkBeginCommandBuffer failed with result: [" << result << "]";
					return;
				}
			}
			{
				VkRenderPassBeginInfo info = {};
				info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				info.renderPass            = m_VkRenderPass;
				info.framebuffer           = fd->Framebuffer;
				if (m_VkImageExtent.width == 0 || m_VkImageExtent.height == 0)
				{
					info.renderArea.extent.width  = *Pointers.ScreenResX;
					info.renderArea.extent.height = *Pointers.ScreenResY;
				}
				else
				{
					info.renderArea.extent = m_VkImageExtent;
				}

				vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
			}

			if (!ImGui::GetIO().BackendRendererUserData)
			{
				ImGui_ImplVulkan_InitInfo init_info = {};
				init_info.Instance                  = m_VkInstance;
				init_info.PhysicalDevice            = m_VkPhysicalDevice;
				init_info.Device                    = m_VkDevice;
				init_info.QueueFamily               = m_VkQueueFamily;
				init_info.Queue                     = GraphicQueue;
				init_info.PipelineCache             = m_VkPipelineCache;
				init_info.DescriptorPool            = m_VkDescriptorPool;
				init_info.Subpass                   = 0;
				init_info.MinImageCount             = m_VkMinImageCount;
				init_info.ImageCount                = m_VkMinImageCount;
				init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
				init_info.Allocator                 = m_VkAllocator;

				ImGui_ImplVulkan_Init(&init_info);

				ImGui_ImplVulkan_CreateFontsTexture();
			}

			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			for (const auto& callback : m_RendererCallBacks | std::views::values)
				callback();

			ImGui::Render();

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fd->CommandBuffer);

			vkCmdEndRenderPass(fd->CommandBuffer);
			vkEndCommandBuffer(fd->CommandBuffer);

			uint32_t waitSemaphoresCount = i == 0 ? pPresentInfo->waitSemaphoreCount : 0;
			if (waitSemaphoresCount == 0 && !QueueSupportsGraphic)
			{
				constexpr VkPipelineStageFlags stages_wait = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				{
					VkSubmitInfo info = {};
					info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

					info.pWaitDstStageMask = &stages_wait;

					info.signalSemaphoreCount = 1;
					info.pSignalSemaphores    = &fsd->RenderCompleteSemaphore;

					if (const VkResult result = vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE); result != VK_SUCCESS)
					{
						LOG(WARNING) << "vkQueueSubmit failed with result: [" << result << "]";
						return;
					}
				}
				{
					VkSubmitInfo info       = {};
					info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					info.commandBufferCount = 1;
					info.pCommandBuffers    = &fd->CommandBuffer;

					info.pWaitDstStageMask  = &stages_wait;
					info.waitSemaphoreCount = 1;
					info.pWaitSemaphores    = &fsd->RenderCompleteSemaphore;

					info.signalSemaphoreCount = 1;
					info.pSignalSemaphores    = &fsd->ImageAcquiredSemaphore;

					if (const VkResult result = vkQueueSubmit(GraphicQueue, 1, &info, fd->Fence); result != VK_SUCCESS)
					{
						LOG(WARNING) << "vkQueueSubmit 2 failed with result: [" << result << "]";
						return;
					}
				}
			}
			else
			{
				std::vector<VkPipelineStageFlags> stages_wait(waitSemaphoresCount, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				VkSubmitInfo info       = {};
				info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				info.commandBufferCount = 1;
				info.pCommandBuffers    = &fd->CommandBuffer;

				info.pWaitDstStageMask  = stages_wait.data();
				info.waitSemaphoreCount = waitSemaphoresCount;
				info.pWaitSemaphores    = pPresentInfo->pWaitSemaphores;

				info.signalSemaphoreCount = 1;
				info.pSignalSemaphores    = &fsd->ImageAcquiredSemaphore;

				if (const VkResult result = vkQueueSubmit(GraphicQueue, 1, &info, fd->Fence); result != VK_SUCCESS)
				{
					LOG(WARNING) << "vkQueueSubmit 3 failed with result: [" << result << "]";
					return;
				}
			}
		}
	}


	bool Renderer::InitImpl()
	{
		while (!*Pointers.Hwnd || !*Pointers.ScreenResX)
		{
			std::this_thread::sleep_for(1s);
		}

		if (const auto& RendererInfo = Pointers.GetRendererInfo(); RendererInfo)
		{
			if (RendererInfo->is_rendering_type(eRenderingType::DX12))
			{
				Pointers.IsVulkan = false;
			}
			else if (RendererInfo->is_rendering_type(eRenderingType::Vulkan))
			{
				Pointers.IsVulkan = true;
			}
			else
			{
				LOG(INFO) << "Unknown renderer type!";
				return false;
			}
		}
		else
		{
			LOG(INFO) << "Invalid renderer info!";
			return false;
		}

		if (Pointers.IsVulkan)
		{
			LOG(INFO) << "Using Vulkan";
			return InitVulkan();
		}
		else if (!Pointers.IsVulkan)
		{
			LOG(INFO) << "Using DX12, clear shader cache if you're having issues.";
			LOG(INFO) << "Waiting...";
			std::this_thread::sleep_for(5s); //Early injection could result in errors.
			return InitDX12();
		}

		return false;
	}

	bool Renderer::AddRendererCallBackImpl(RendererCallBack&& callback, std::uint32_t priority)
	{
		return m_RendererCallBacks.insert({priority, callback}).second;
	}

	void Renderer::AddWindowProcedureCallbackImpl(WindowProcedureCallback&& callback)
	{
		return m_WindowProcedureCallbacks.push_back(callback);
	}

	void Renderer::DX12OnPresentImpl()
	{
		Renderer::DX12NewFrame();
		for (const auto& callback : m_RendererCallBacks | std::views::values)
			callback();
		Renderer::DX12EndFrame();
	}

	LRESULT Renderer::WndProcImpl(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		for (const auto& callback : m_WindowProcedureCallbacks)
			callback(hwnd, msg, wparam, lparam);

		return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
	}

	void Renderer::WaitForLastFrame()
	{
		FrameContext FrameCtx = GetInstance().m_FrameContext[GetInstance().m_FrameIndex % GetInstance().m_SwapChainDesc.BufferCount];

		UINT64 FenceValue = FrameCtx.FenceValue;

		if (FenceValue == 0)
		{
			return;
		}

		FrameCtx.FenceValue = 0;

		if (GetInstance().m_Fence->GetCompletedValue() >= FenceValue)
		{
			return;
		}

		GetInstance().m_Fence->SetEventOnCompletion(FenceValue, GetInstance().m_FenceEvent);

		WaitForSingleObject(GetInstance().m_FenceEvent, INFINITE);
	}

	void Renderer::WaitForNextFrame()
	{
		UINT NextFrameIndex        = GetInstance().m_FrameIndex + 1;
		GetInstance().m_FrameIndex = NextFrameIndex;

		HANDLE WaitableObjects[] = {GetInstance().m_SwapchainWaitableObject, nullptr};
		DWORD NumWaitableObjets  = 1;

		FrameContext FrameCtx = GetInstance().m_FrameContext[NextFrameIndex % GetInstance().m_SwapChainDesc.BufferCount];
		UINT64 FenceValue = FrameCtx.FenceValue;
		if (FenceValue != 0) // means no fence was signaled
		{
			FrameCtx.FenceValue = 0;
			GetInstance().m_Fence->SetEventOnCompletion(FenceValue, GetInstance().m_FenceEvent);
			WaitableObjects[1] = GetInstance().m_FenceEvent;
			NumWaitableObjets  = 2;
		}

		WaitForMultipleObjects(NumWaitableObjets, WaitableObjects, TRUE, INFINITE);
	}

	void Renderer::DX12PreResize()
	{
		SetResizing(true);

		WaitForLastFrame();

		ImGui_ImplDX12_InvalidateDeviceObjects();

		for (size_t i{}; i != GetInstance().m_SwapChainDesc.BufferCount; ++i)
		{
			REL(GetInstance().m_FrameContext[i].Resource);
		}
	}

	void Renderer::DX12PostResize()
	{
		//Recreate our pointers and ImGui's
		ImGui_ImplDX12_CreateDeviceObjects();
		const auto RTVDescriptorSize{GetInstance().m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
		D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle{GetInstance().m_BackbufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};
		for (size_t i{}; i != GetInstance().m_SwapChainDesc.BufferCount; ++i)
		{
			ComPtr<ID3D12Resource> BackBuffer{};
			GetInstance().m_FrameContext[i].Descriptor = RTVHandle;
			GetInstance().m_SwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)BackBuffer.GetAddressOf());
			GetInstance().m_Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, RTVHandle);
			GetInstance().m_FrameContext[i].Resource = BackBuffer.Get();
			RTVHandle.ptr += RTVDescriptorSize;
		}

		SetResizing(false);
	}

	void Renderer::DX12NewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void Renderer::DX12EndFrame()
	{
		WaitForNextFrame();

		FrameContext& CurrentFrameContext{GetInstance().m_FrameContext[GetInstance().m_SwapChain->GetCurrentBackBufferIndex()]};
		CurrentFrameContext.CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER Barrier{D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		    D3D12_RESOURCE_BARRIER_FLAG_NONE,
		    {{CurrentFrameContext.Resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET}}};
		GetInstance().m_CommandList->Reset(CurrentFrameContext.CommandAllocator, nullptr);
		GetInstance().m_CommandList->ResourceBarrier(1, &Barrier);
		GetInstance().m_CommandList->OMSetRenderTargets(1, &CurrentFrameContext.Descriptor, FALSE, nullptr);
		GetInstance().m_CommandList->SetDescriptorHeaps(1, GetInstance().m_DescriptorHeap.GetAddressOf());

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GetInstance().m_CommandList.Get());

		Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		Barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
		GetInstance().m_CommandList->ResourceBarrier(1, &Barrier);
		GetInstance().m_CommandList->Close();

		ID3D12CommandList* CommandLists[]{GetInstance().m_CommandList.Get()};
		GetInstance().m_CommandQueue->ExecuteCommandLists(_countof(CommandLists), CommandLists);

		UINT64 FenceValue = GetInstance().m_FenceLastSignaledValue + 1;
		GetInstance().m_CommandQueue->Signal(GetInstance().m_Fence.Get(), FenceValue);
		GetInstance().m_FenceLastSignaledValue = FenceValue;
		CurrentFrameContext.FenceValue         = FenceValue;
	}

	// ---- Terminator logo (menu header) ----

	static bool DecodeLogoPng(std::vector<std::uint8_t>& out_rgba, int& out_width, int& out_height)
	{
		HMODULE mod = nullptr;
		if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		        reinterpret_cast<LPCSTR>(&Renderer::GetLogoTexture),
		        &mod)
		    || !mod)
		{
			LOG(WARNING) << "Logo: failed to get module handle.";
			return false;
		}

		HRSRC res = FindResourceA(mod, MAKEINTRESOURCEA(IDR_TERMINATOR_PNG), RT_RCDATA);
		if (!res)
		{
			LOG(WARNING) << "Logo: resource not found.";
			return false;
		}

		const DWORD size = SizeofResource(mod, res);
		const void* data = LockResource(LoadResource(mod, res));
		if (!data || !size)
		{
			LOG(WARNING) << "Logo: resource is empty.";
			return false;
		}

		const HRESULT co_init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool need_uninit = SUCCEEDED(co_init);

		bool ok = false;
		ComPtr<IWICImagingFactory> factory;
		if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
		{
			ComPtr<IWICStream> stream;
			if (SUCCEEDED(factory->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromMemory((WICInProcPointer)data, size)))
			{
				ComPtr<IWICBitmapDecoder> decoder;
				if (SUCCEEDED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)))
				{
					ComPtr<IWICBitmapFrameDecode> frame;
					if (SUCCEEDED(decoder->GetFrame(0, &frame)))
					{
						ComPtr<IWICFormatConverter> converter;
						if (SUCCEEDED(factory->CreateFormatConverter(&converter))
						    && SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
						{
							UINT w = 0, h = 0;
							if (SUCCEEDED(converter->GetSize(&w, &h)) && w > 0 && h > 0 && w <= 1024 && h <= 1024)
							{
								out_rgba.resize((std::size_t)w * h * 4);
								if (SUCCEEDED(converter->CopyPixels(nullptr, w * 4, (UINT)out_rgba.size(), out_rgba.data())))
								{
									out_width  = (int)w;
									out_height = (int)h;
									ok         = true;
								}
							}
						}
					}
				}
			}
		}

		if (!ok)
			LOG(WARNING) << "Logo: WIC decode failed.";

		if (need_uninit)
			CoUninitialize();

		return ok;
	}

	bool Renderer::CreateLogoTextureDX12(const std::uint8_t* rgba, int width, int height)
	{
		if (!m_Device || !m_CommandQueue || !m_DescriptorHeap || m_SwapChainDesc.BufferCount < 2)
			return false;

		D3D12_RESOURCE_DESC tex_desc{};
		tex_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		tex_desc.Width              = (UINT64)width;
		tex_desc.Height             = (UINT)height;
		tex_desc.DepthOrArraySize   = 1;
		tex_desc.MipLevels          = 1;
		tex_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
		tex_desc.SampleDesc.Count   = 1;
		tex_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		tex_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES heap_props{};
		heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;
		if (FAILED(m_Device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &tex_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_LogoTextureResource))))
		{
			LOG(WARNING) << "Logo: failed to create DX12 texture.";
			return false;
		}

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT num_rows       = 0;
		UINT64 row_size     = 0;
		UINT64 upload_bytes = 0;
		m_Device->GetCopyableFootprints(&tex_desc, 0, 1, 0, &footprint, &num_rows, &row_size, &upload_bytes);

		D3D12_RESOURCE_DESC buf_desc{};
		buf_desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
		buf_desc.Width            = upload_bytes;
		buf_desc.Height           = 1;
		buf_desc.DepthOrArraySize = 1;
		buf_desc.MipLevels        = 1;
		buf_desc.Format           = DXGI_FORMAT_UNKNOWN;
		buf_desc.SampleDesc.Count = 1;
		buf_desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES upload_props{};
		upload_props.Type = D3D12_HEAP_TYPE_UPLOAD;
		if (FAILED(m_Device->CreateCommittedResource(&upload_props, D3D12_HEAP_FLAG_NONE, &buf_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_LogoUploadBuffer))))
		{
			LOG(WARNING) << "Logo: failed to create DX12 upload buffer.";
			m_LogoTextureResource.Reset();
			return false;
		}

		void* mapped = nullptr;
		D3D12_RANGE read_range{0, 0};
		if (FAILED(m_LogoUploadBuffer->Map(0, &read_range, &mapped)))
		{
			LOG(WARNING) << "Logo: failed to map DX12 upload buffer.";
			m_LogoTextureResource.Reset();
			m_LogoUploadBuffer.Reset();
			return false;
		}
		for (UINT y = 0; y < num_rows; ++y)
			memcpy((std::uint8_t*)mapped + footprint.Offset + (std::size_t)y * footprint.Footprint.RowPitch, rgba + (std::size_t)y * width * 4, (std::size_t)width * 4);
		m_LogoUploadBuffer->Unmap(0, nullptr);

		ComPtr<ID3D12CommandAllocator> alloc;
		ComPtr<ID3D12GraphicsCommandList> list;
		if (FAILED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc)))
		    || FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&list))))
		{
			LOG(WARNING) << "Logo: failed to create DX12 upload command list.";
			m_LogoTextureResource.Reset();
			m_LogoUploadBuffer.Reset();
			return false;
		}

		D3D12_TEXTURE_COPY_LOCATION dst{};
		dst.pResource        = m_LogoTextureResource.Get();
		dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = 0;
		D3D12_TEXTURE_COPY_LOCATION src{};
		src.pResource       = m_LogoUploadBuffer.Get();
		src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint = footprint;
		list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource   = m_LogoTextureResource.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		list->ResourceBarrier(1, &barrier);
		list->Close();

		ID3D12CommandList* lists[] = {list.Get()};
		m_CommandQueue->ExecuteCommandLists(1, lists);

		ComPtr<ID3D12Fence> fence;
		HANDLE event = nullptr;
		if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) || !(event = CreateEventA(nullptr, FALSE, FALSE, nullptr)))
		{
			LOG(WARNING) << "Logo: failed to create DX12 upload fence.";
			m_LogoTextureResource.Reset();
			m_LogoUploadBuffer.Reset();
			return false;
		}
		m_CommandQueue->Signal(fence.Get(), 1);
		fence->SetEventOnCompletion(1, event);
		WaitForSingleObject(event, 5000);
		CloseHandle(event);

		// SRV at descriptor index 1 of ImGui's heap (index 0 = fonts)
		const UINT increment = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
		srv.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Texture2D.MipLevels       = 1;
		D3D12_CPU_DESCRIPTOR_HANDLE cpu = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		cpu.ptr += increment;
		m_Device->CreateShaderResourceView(m_LogoTextureResource.Get(), &srv, cpu);

		D3D12_GPU_DESCRIPTOR_HANDLE gpu = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		gpu.ptr += increment;
		m_LogoTexture = (ImTextureID)gpu.ptr;

		LOG(INFO) << "Logo: DX12 texture ready.";
		return true;
	}

	static std::uint32_t FindVkMemoryType(VkPhysicalDevice physical, std::uint32_t type_bits, VkMemoryPropertyFlags props)
	{
		VkPhysicalDeviceMemoryProperties mem_props{};
		vkGetPhysicalDeviceMemoryProperties(physical, &mem_props);
		for (std::uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
		{
			if ((type_bits & (1u << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props)
				return i;
		}
		return (std::uint32_t)-1;
	}

	bool Renderer::CreateLogoTextureVulkan(const std::uint8_t* rgba, int width, int height)
	{
		if (!m_VkDevice || m_VkGraphicQueue == VK_NULL_HANDLE || m_VkQueueFamily == (std::uint32_t)-1)
			return false;

		const VkDeviceSize upload_bytes = (VkDeviceSize)width * height * 4;

		VkBuffer staging = VK_NULL_HANDLE;
		VkDeviceMemory staging_mem = VK_NULL_HANDLE;
		{
			VkBufferCreateInfo buf_info{};
			buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buf_info.size  = upload_bytes;
			buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			if (vkCreateBuffer(m_VkDevice, &buf_info, m_VkAllocator, &staging) != VK_SUCCESS)
				return false;

			VkMemoryRequirements req{};
			vkGetBufferMemoryRequirements(m_VkDevice, staging, &req);
			const std::uint32_t mem_type = FindVkMemoryType(m_VkPhysicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (mem_type == (std::uint32_t)-1)
			{
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}

			VkMemoryAllocateInfo alloc_info{};
			alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize  = req.size;
			alloc_info.memoryTypeIndex = mem_type;
			if (vkAllocateMemory(m_VkDevice, &alloc_info, m_VkAllocator, &staging_mem) != VK_SUCCESS)
			{
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}
			vkBindBufferMemory(m_VkDevice, staging, staging_mem, 0);

			void* mapped = nullptr;
			if (vkMapMemory(m_VkDevice, staging_mem, 0, req.size, 0, &mapped) != VK_SUCCESS)
			{
				vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}
			memcpy(mapped, rgba, (std::size_t)upload_bytes);
			vkUnmapMemory(m_VkDevice, staging_mem);
		}

		VkImageCreateInfo img_info{};
		img_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		img_info.imageType     = VK_IMAGE_TYPE_2D;
		img_info.format        = VK_FORMAT_R8G8B8A8_UNORM;
		img_info.extent        = {(std::uint32_t)width, (std::uint32_t)height, 1};
		img_info.mipLevels     = 1;
		img_info.arrayLayers   = 1;
		img_info.samples       = VK_SAMPLE_COUNT_1_BIT;
		img_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
		img_info.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (vkCreateImage(m_VkDevice, &img_info, m_VkAllocator, &m_LogoImage) != VK_SUCCESS)
		{
			vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
			vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
			return false;
		}

		{
			VkMemoryRequirements req{};
			vkGetImageMemoryRequirements(m_VkDevice, m_LogoImage, &req);
			const std::uint32_t mem_type = FindVkMemoryType(m_VkPhysicalDevice, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (mem_type == (std::uint32_t)-1)
			{
				vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
				m_LogoImage = VK_NULL_HANDLE;
				vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}

			VkMemoryAllocateInfo alloc_info{};
			alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			alloc_info.allocationSize  = req.size;
			alloc_info.memoryTypeIndex = mem_type;
			if (vkAllocateMemory(m_VkDevice, &alloc_info, m_VkAllocator, &m_LogoMemory) != VK_SUCCESS)
			{
				vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
				m_LogoImage = VK_NULL_HANDLE;
				vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}
			vkBindImageMemory(m_VkDevice, m_LogoImage, m_LogoMemory, 0);
		}

		VkCommandPool cmd_pool = VK_NULL_HANDLE;
		{
			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.queueFamilyIndex = m_VkQueueFamily;
			pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			if (vkCreateCommandPool(m_VkDevice, &pool_info, m_VkAllocator, &cmd_pool) != VK_SUCCESS)
			{
				vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
				vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
				m_LogoImage  = VK_NULL_HANDLE;
				m_LogoMemory = VK_NULL_HANDLE;
				vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
				vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);
				return false;
			}
		}

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VkFence fence       = VK_NULL_HANDLE;
		bool upload_ok      = false;
		{
			VkCommandBufferAllocateInfo cmd_info{};
			cmd_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmd_info.commandPool        = cmd_pool;
			cmd_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmd_info.commandBufferCount = 1;
			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

			if (vkAllocateCommandBuffers(m_VkDevice, &cmd_info, &cmd) == VK_SUCCESS && vkCreateFence(m_VkDevice, &fence_info, m_VkAllocator, &fence) == VK_SUCCESS)
			{
				VkCommandBufferBeginInfo begin_info{};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				if (vkBeginCommandBuffer(cmd, &begin_info) == VK_SUCCESS)
				{
					VkImageMemoryBarrier barrier{};
					barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.srcAccessMask       = 0;
					barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.image               = m_LogoImage;
					barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

					VkBufferImageCopy region{};
					region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
					region.imageExtent      = {(std::uint32_t)width, (std::uint32_t)height, 1};
					vkCmdCopyBufferToImage(cmd, staging, m_LogoImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

					if (vkEndCommandBuffer(cmd) == VK_SUCCESS)
					{
						VkSubmitInfo submit{};
						submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
						submit.commandBufferCount = 1;
						submit.pCommandBuffers    = &cmd;
						if (vkQueueSubmit(m_VkGraphicQueue, 1, &submit, fence) == VK_SUCCESS && vkWaitForFences(m_VkDevice, 1, &fence, VK_TRUE, 5000000000ull) == VK_SUCCESS)
							upload_ok = true;
					}
				}
			}
		}

		vkDestroyFence(m_VkDevice, fence, m_VkAllocator);
		vkDestroyCommandPool(m_VkDevice, cmd_pool, m_VkAllocator);
		vkFreeMemory(m_VkDevice, staging_mem, m_VkAllocator);
		vkDestroyBuffer(m_VkDevice, staging, m_VkAllocator);

		if (!upload_ok)
		{
			vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
			vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
			m_LogoImage  = VK_NULL_HANDLE;
			m_LogoMemory = VK_NULL_HANDLE;
			LOG(WARNING) << "Logo: Vulkan upload failed.";
			return false;
		}

		VkImageViewCreateInfo view_info{};
		view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image    = m_LogoImage;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format   = VK_FORMAT_R8G8B8A8_UNORM;
		view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		if (vkCreateImageView(m_VkDevice, &view_info, m_VkAllocator, &m_LogoView) != VK_SUCCESS)
		{
			vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
			vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
			m_LogoImage  = VK_NULL_HANDLE;
			m_LogoMemory = VK_NULL_HANDLE;
			return false;
		}

		VkSamplerCreateInfo sampler_info{};
		sampler_info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter    = VK_FILTER_LINEAR;
		sampler_info.minFilter    = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_info.maxAnisotropy = 1.0f;
		sampler_info.maxLod        = 0.0f;
		if (vkCreateSampler(m_VkDevice, &sampler_info, m_VkAllocator, &m_LogoSampler) != VK_SUCCESS)
		{
			vkDestroyImageView(m_VkDevice, m_LogoView, m_VkAllocator);
			m_LogoView = VK_NULL_HANDLE;
			vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
			vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
			m_LogoImage  = VK_NULL_HANDLE;
			m_LogoMemory = VK_NULL_HANDLE;
			return false;
		}

		m_LogoDescSet = ImGui_ImplVulkan_AddTexture(m_LogoSampler, m_LogoView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (m_LogoDescSet == VK_NULL_HANDLE)
		{
			vkDestroySampler(m_VkDevice, m_LogoSampler, m_VkAllocator);
			m_LogoSampler = VK_NULL_HANDLE;
			vkDestroyImageView(m_VkDevice, m_LogoView, m_VkAllocator);
			m_LogoView = VK_NULL_HANDLE;
			vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
			vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
			m_LogoImage  = VK_NULL_HANDLE;
			m_LogoMemory = VK_NULL_HANDLE;
			return false;
		}

		m_LogoTexture = (ImTextureID)m_LogoDescSet;
		LOG(INFO) << "Logo: Vulkan texture ready.";
		return true;
	}

	ImTextureID Renderer::GetLogoTextureImpl()
	{
		if (m_LogoTexture)
			return m_LogoTexture;

		// DX12 backend ready?
		if (m_Device && m_CommandQueue && m_DescriptorHeap)
		{
			static bool dx12_failed = false;
			if (dx12_failed)
				return (ImTextureID)0;

			static std::vector<std::uint8_t> png;
			static int png_w = 0, png_h = 0;
			static bool decoded = DecodeLogoPng(png, png_w, png_h);
			if (!decoded || !CreateLogoTextureDX12(png.data(), png_w, png_h))
				dx12_failed = true;

			return m_LogoTexture;
		}

		// Vulkan backend ready (needs the graphics queue, cached from present)?
		if (m_VkDevice && m_VkGraphicQueue != VK_NULL_HANDLE)
		{
			static bool vk_failed = false;
			if (vk_failed)
				return (ImTextureID)0;

			static std::vector<std::uint8_t> png;
			static int png_w = 0, png_h = 0;
			static bool decoded = DecodeLogoPng(png, png_w, png_h);
			if (!decoded || !CreateLogoTextureVulkan(png.data(), png_w, png_h))
				vk_failed = true;

			return m_LogoTexture;
		}

		return (ImTextureID)0;
	}

	void Renderer::DestroyLogoTexture()
	{
		m_LogoTexture = (ImTextureID)0;

		m_LogoUploadBuffer.Reset();
		m_LogoTextureResource.Reset();

		if (m_VkDevice)
		{
			if (m_LogoDescSet != VK_NULL_HANDLE && m_VkDescriptorPool != VK_NULL_HANDLE)
			{
				vkFreeDescriptorSets(m_VkDevice, m_VkDescriptorPool, 1, &m_LogoDescSet);
				m_LogoDescSet = VK_NULL_HANDLE;
			}
			if (m_LogoSampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(m_VkDevice, m_LogoSampler, m_VkAllocator);
				m_LogoSampler = VK_NULL_HANDLE;
			}
			if (m_LogoView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(m_VkDevice, m_LogoView, m_VkAllocator);
				m_LogoView = VK_NULL_HANDLE;
			}
			if (m_LogoImage != VK_NULL_HANDLE)
			{
				vkDestroyImage(m_VkDevice, m_LogoImage, m_VkAllocator);
				m_LogoImage = VK_NULL_HANDLE;
			}
			if (m_LogoMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(m_VkDevice, m_LogoMemory, m_VkAllocator);
				m_LogoMemory = VK_NULL_HANDLE;
			}
		}
	}
}