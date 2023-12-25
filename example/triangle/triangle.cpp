#include "VEbase.h"

class Triangle : public VEbase {
public:
	Triangle() : VEbase("Vulkan Application - Triangle") {

	}

	void run() {
		init();
		prepare();
		mainLoop();
		cleanUp();
		cleanUpBase();
	}

private:
	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	std::vector<vk::Framebuffer> swapChainFrameBuffers;

	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;

	vk::Semaphore imageAvailableSemaphore;
	vk::Semaphore renderFinishedSemaphore;
	vk::Fence inflightFence;

	void prepare() {
		createRenderPass();
		createGraphicsPipeLine();
		createFrameBuffers();
		createCommandPool();
		createCommandBuffer();
		createSyncObjects();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			keyHandle();

			drawFrame();
		}

		device.waitIdle();
	}

	void cleanUp() {
		device.destroySemaphore(imageAvailableSemaphore);
		device.destroySemaphore(renderFinishedSemaphore);
		device.destroyFence(inflightFence);

		device.destroyCommandPool(commandPool);

		for (auto i = 0; i < swapChainFrameBuffers.size(); i++) {
			device.destroyFramebuffer(swapChainFrameBuffers[i]);
		}

		device.destroyPipeline(graphicsPipeline);
		device.destroyRenderPass(renderPass);
		device.destroyPipelineLayout(pipelineLayout);
	}

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
		auto vert = readFileAsBinary(getShadersPath() + "triangle/triangle_vert.spv");
		auto frag = readFileAsBinary(getShadersPath() + "triangle/triangle_frag.spv");

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

		std::vector<vk::DynamicState> dynamicStates{
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
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

	void createFrameBuffers() {
		swapChainFrameBuffers.resize(swapChainImageViews.size());

		for (auto i = 0; i < swapChainFrameBuffers.size(); i++) {
			vk::ImageView attachments[] = {
				swapChainImageViews[i]
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

	void createCommandPool() {
		auto queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

		vk::CommandPoolCreateInfo poolInfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		};

		commandPool = device.createCommandPool(poolInfo);
	}

	void createCommandBuffer() {
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1,
		};

		commandBuffer = device.allocateCommandBuffers(allocInfo).front();
	}

	void recordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex) {
		vk::CommandBufferBeginInfo beginInfo{};
		commandBuffer.begin(beginInfo);

		vk::RenderPassBeginInfo renderPassInfo{
			.renderPass = renderPass,
			.framebuffer = swapChainFrameBuffers[imageIndex],
			.renderArea = {
				.offset = {0, 0},
				.extent = swapChainExtent,
			},
			.clearValueCount = 1,
			.pClearValues = new vk::ClearValue {{0.0f, 0.0f, 0.0f, 1.0f}},
		};

		commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

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

		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();

		commandBuffer.end();
	}

	void createSyncObjects() {
		vk::SemaphoreCreateInfo semaforeInfo{};
		vk::FenceCreateInfo fenceInfo{
			.flags = vk::FenceCreateFlagBits::eSignaled,
		};

		imageAvailableSemaphore = device.createSemaphore(semaforeInfo);
		renderFinishedSemaphore = device.createSemaphore(semaforeInfo);
		inflightFence = device.createFence(fenceInfo);
	}

	void drawFrame() {
		std::ignore = device.waitForFences({ inflightFence }, vk::False, UINT64_MAX);
		device.resetFences({ inflightFence });

		uint32_t imageIndex = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, nullptr).value;

		commandBuffer.reset();
		recordCommandBuffer(commandBuffer, imageIndex);

		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphore };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphore };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

		vk::SubmitInfo submitInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		graphicsQueue.submit(submitInfo, inflightFence);

		vk::SwapchainKHR swapChains[]{ swapChain };
		vk::PresentInfoKHR presentInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = &imageIndex,
		};

		std::ignore = presentQueue.presentKHR(presentInfo);
	}
};

int main() {
	auto app = new Triangle();
	app->run();

	return EXIT_SUCCESS;
}