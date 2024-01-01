#include "VEbase.h"

class Triangle : public VEbase {
public:
	Triangle() : VEbase("Vulkan Application - Triangle") {

	}

	~Triangle() {
		for (auto i = 0;i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inflightFences[i]);
		}

		device.destroyCommandPool(commandPool);

		destroyFrameBuffers();

		device.destroyPipeline(graphicsPipeline);
		device.destroyRenderPass(renderPass);
		device.destroyPipelineLayout(pipelineLayout);

		cleanUpBase();
	}

	void run() {
		init();
		prepare();
		mainLoop();
	}

private:
	// Renderpass : 렌더링 구조를 명시한다고 보면 된다
	vk::RenderPass renderPass;

	// Pipeline이 Descriptor Sets에 접근하기 위해 필요하다
	vk::PipelineLayout pipelineLayout;

	// Pipeline (pipeline state object) - 파이프라인 단계마다 렌더링 동작을 명시한다
	vk::Pipeline graphicsPipeline;

	// Framebuffer는 Renderpass에서 다루는 attachment의 메모리 저장 정보를 들고있다
	std::vector<vk::Framebuffer> swapChainFrameBuffers;

	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	// CPU (user app.) 단에서의 synchronization에 사용한다
	std::vector<vk::Fence> inflightFences;

	// Queue 내의 synchronization에 사용한다
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;

	uint32_t currentFrame{ 0 };

	void prepare() {
		createRenderPass();
		createGraphicsPipeLine();
		createFrameBuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}

	// - Attachment
	//		렌더링 과정에서 사용하는 이미지
	// - Subpass
	//		1개 이상의 subpass를 가진다
	// - Subpass Dependency
	//		srcSubpass - 의존 관계에 있는 subpass / dstSubpass - 현재 subpass
	//		StageMask <- Pipeline Stage 명시
	//			src - ensures logically earlier stages to be executed
	//			dst - ensures logically later stages not to be executed
	//		AccessMask <- Memory access
	void createRenderPass() {
		vk::AttachmentDescription colorAttachment{
			.format = swapChainFormat,
			.samples = vk::SampleCountFlagBits::e1,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout = vk::ImageLayout::eUndefined,
			.finalLayout = vk::ImageLayout::ePresentSrcKHR,
		};

		vk::AttachmentReference colorAttachmentRef{
			.attachment = 0,
			.layout = vk::ImageLayout::eColorAttachmentOptimal,
		};

		vk::SubpassDescription subpass{
			.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentRef,
		};

		vk::SubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.srcAccessMask = vk::AccessFlagBits::eNone,
			.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		};

		vk::RenderPassCreateInfo renderPassInfo{
			.attachmentCount = 1,
			.pAttachments = &colorAttachment,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		renderPass = device.createRenderPass(renderPassInfo);
	}

	void createGraphicsPipeLine() {
		auto vert = readFileAsBinary(getShadersPath() + "triangle/triangle.vert.spv");
		auto frag = readFileAsBinary(getShadersPath() + "triangle/triangle.frag.spv");

		auto vertModule = createShaderModule(vert);
		auto fragModule = createShaderModule(frag);

		vk::PipelineShaderStageCreateInfo shaderStages[]{
			{
				.stage = vk::ShaderStageFlagBits::eVertex,
				.module = vertModule,
				.pName = "main",
			},
			{
				.stage = vk::ShaderStageFlagBits::eFragment,
				.module = fragModule,
				.pName = "main",
			}
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputState{
			.vertexBindingDescriptionCount = 0,
			.vertexAttributeDescriptionCount = 0,
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False,
		};

		vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1,
			.scissorCount = 1,
		};

		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eClockwise,
			.depthBiasEnable = vk::False,
			.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False,
		};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR |
							vk::ColorComponentFlagBits::eG |
							vk::ColorComponentFlagBits::eB |
							vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo colorBlendState{
			.logicOpEnable = vk::False,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
		};

		std::vector<vk::DynamicState> dynamicStates{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
		};
		vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data(),
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

		vk::GraphicsPipelineCreateInfo pipelineInfo{
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout,
			.renderPass = renderPass,
			.subpass = 0,	//index
		};

		graphicsPipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;

		device.destroyShaderModule(vertModule);
		device.destroyShaderModule(fragModule);
	}

	// Renderpass에서 명시한 attachment description과 구조가 일치하여야 함
	virtual void createFrameBuffers() {
		swapChainFrameBuffers.resize(swapChainImageViews.size());

		for (auto i = 0; i < swapChainFrameBuffers.size(); i++) {
			vk::ImageView attachments[] = {
				swapChainImageViews[i],
			};

			vk::FramebufferCreateInfo framebufferInfo{
				.renderPass = renderPass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = swapChainExtent.width,
				.height = swapChainExtent.height,
				.layers = 1,
			};

			swapChainFrameBuffers[i] = device.createFramebuffer(framebufferInfo);
		}
	}

	virtual void destroyFrameBuffers() {
		for (auto i = 0; i < swapChainFrameBuffers.size(); i++) {
			device.destroyFramebuffer(swapChainFrameBuffers[i]);
		}
	}

	void createCommandPool() {
		auto queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

		vk::CommandPoolCreateInfo poolInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		};

		commandPool = device.createCommandPool(poolInfo);
	}

	void createCommandBuffers() {
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::CommandBufferAllocateInfo allocInfo{
				.commandPool = commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = 1,
			};

			commandBuffers[i] = device.allocateCommandBuffers(allocInfo).front();
		}
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
		vk::CommandBufferBeginInfo beginInfo{};
		commandBuffer.begin(beginInfo);

		// attachment loadOp = Clear 시 값 지정
		vk::ClearValue clearValue;
		clearValue.color = { 0.0f, 0.0f, 0.1f, 1.0f };
		clearValue.depthStencil = {0.1f, 0};

		vk::RenderPassBeginInfo renderPassInfo{
			.renderPass = renderPass,
			.framebuffer = swapChainFrameBuffers[imageIndex],
			.renderArea = {
				.offset = {0, 0},
				.extent = swapChainExtent,
			},
			.clearValueCount = 1,
			.pClearValues = &clearValue,
		};

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// Update dynamic state
		commandBuffer.setViewport(0, vk::Viewport{
			.x = 0,
			.y = 0,
			.width = (float)swapChainExtent.width,
			.height = (float)swapChainExtent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
			});

		commandBuffer.setScissor(0, vk::Rect2D{
			.offset = {0, 0},
			.extent = swapChainExtent,
			});

		// rendering pipeline에 pipeline state object에 저장한 정보를 bind
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void createSyncObjects() {
		vk::SemaphoreCreateInfo semaforeInfo{};
		vk::FenceCreateInfo fenceInfo{
			.flags = vk::FenceCreateFlagBits::eSignaled,
		};

		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inflightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device.createSemaphore(semaforeInfo);
			renderFinishedSemaphores[i] = device.createSemaphore(semaforeInfo);
			inflightFences[i] = device.createFence(fenceInfo);
		}
	}

	virtual void drawFrame() {
		std::ignore = device.waitForFences({ inflightFences[currentFrame]}, vk::False, UINT64_MAX);

		if (framebufferResized) {
			recreateSwapChain();
			framebufferResized = false;
		}

		auto result{ device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr) };
		if (result.result == vk::Result::eSuboptimalKHR) {
			framebufferResized = true;
		}
		
		device.resetFences({ inflightFences[currentFrame]});

		uint32_t imageIndex{ result.value };

		commandBuffers[currentFrame].reset();
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
		
		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffers[currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		graphicsQueue.submit(submitInfo, inflightFences[currentFrame]);

		vk::SwapchainKHR swapChains[]{ swapChain };
		vk::PresentInfoKHR presentInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
		};

		std::ignore = presentQueue.presentKHR(presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
};

int main() {
	auto app = new Triangle();
	app->run();

	return EXIT_SUCCESS;
}