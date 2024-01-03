#include <VEbase.h>

class Uniform : public VEbase {
public:
	Uniform() : VEbase("Vulkan Application - Uniform buffer") {

	}

	~Uniform() {
		// (TODO) vertices, indices, uniform datas
		device.destroyBuffer();
		device.freeMemory();

		device.destroyDescriptorSetLayout(descriptorSetLayout);

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(renderSemaphores[i]);
			device.destroySemaphore(presentReadySemaphores[i]);
		}
		destroyFences();

		device.destroyCommandPool(commandPool);
		destroyFrameBuffers();
		device.destroyPipeline(graphicsPipeline);
		device.destroyRenderPass(renderpass);
	}

	void run() {
		init();
		prepare();
		mainLoop();
	}
private:
	vk::RenderPass renderpass;
	vk::PipelineLayout pipelineLayout{};
	vk::Pipeline graphicsPipeline;

	vk::DescriptorSetLayout descriptorSetLayout{};
	std::vector<vk::DescriptorSet> descriptorSets{};
	vk::DescriptorPool descriptorPool{};

	std::vector<vk::Framebuffer> frameBuffers;

	int currentFrame{ 0 };

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
	};

	struct {
		vk::Buffer buffer;
		vk::DeviceMemory memory;

		const std::vector<Vertex> vertices = {
			{{-0.5, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}}
		};

		vk::VertexInputBindingDescription binding{
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = vk::VertexInputRate::eVertex,
		};

		std::array<vk::VertexInputAttributeDescription, 2> attributes{
			vk::VertexInputAttributeDescription {
				.location = 0,
				.binding = 0,
				.format = vk::Format::eR32G32Sfloat,
				.offset = 0,
			},
			vk::VertexInputAttributeDescription {
				.location = 1,
				.binding = 0,
				.format = vk::Format::eR32G32B32Sfloat,
				.offset = offsetof(Vertex, color),
			},
		};
	} Vertices;

	struct {
		vk::Buffer buffer;
		vk::DeviceMemory memory;

		const std::vector<uint16_t> indices = {
			0, 2, 1, 1, 2, 3
		};
	} Indices;

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct UniformData {
		vk::Buffer buffer;
		vk::DeviceMemory memory;
		UniformBufferObject data;
		void* map;
	};

	std::array<UniformData, 2> uniformData{};

	void createUniformBuffer() {
		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto size = sizeof(UniformBufferObject);
			createBuffer(size,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				uniformData[i].buffer, uniformData[i].memory);

			uniformData[i].map = device.mapMemory(uniformData[i].memory, 0, size);
		}
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{
			.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
			.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f),
		};

		// GLM's Y coord. of the clip coord. is inverted
		ubo.proj[1][1] *= -1;

		memcpy(uniformData[currentImage].map, &ubo, sizeof(ubo));
	}

	void setDescriptorSets() {
		vk::DescriptorSetLayoutBinding uboLayoutBinding{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eVertex,
		};

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
			.bindingCount = 1,
			.pBindings = &uboLayoutBinding,
		};

		descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCI);

		vk::DescriptorPoolSize poolSize{
			.type = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = MAX_FRAMES_IN_FLIGHT,	// descriptor 타입 별 개수 상한
		};

		vk::DescriptorPoolCreateInfo descriptorPoolCI{
			.maxSets = MAX_FRAMES_IN_FLIGHT,	// 풀에 존재할 수 있는 set의 개수 상한
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};

		descriptorPool = device.createDescriptorPool(descriptorPoolCI);

		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{
			.descriptorPool = descriptorPool,
			.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = layouts.data(),
		};

		descriptorSets = device.allocateDescriptorSets(allocInfo);

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DescriptorBufferInfo bufferInfo{
				.buffer = uniformData[i].buffer,
				.offset = 0,
				.range = sizeof(UniformBufferObject),
			};

			vk::WriteDescriptorSet descriptorWrite{
				.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &bufferInfo,
			};

			device.updateDescriptorSets(descriptorWrite, nullptr);
		}
	}

	void prepare() {
		// Uniform Buffer
		createUniformBuffer();
		setDescriptorSets();

		// renderpass
		vk::AttachmentDescription attachment{
			.format = swapChainFormat,
			.samples = vk::SampleCountFlagBits::e1,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eStore,
			.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
			.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
			.initialLayout = vk::ImageLayout::eUndefined,
			.finalLayout = vk::ImageLayout::ePresentSrcKHR,
		};

		vk::AttachmentReference attachmentRef{
			.attachment = 0,
			.layout = vk::ImageLayout::eColorAttachmentOptimal,
		};

		vk::SubpassDescription subpassInfo{
			.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentRef,
		};

		vk::SubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
			.srcAccessMask = vk::AccessFlagBits::eNone,
			.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
		};

		vk::RenderPassCreateInfo renderPassCI{
			.attachmentCount = 1,
			.pAttachments = &attachment,
			.subpassCount = 1,
			.pSubpasses = &subpassInfo,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		renderpass = device.createRenderPass(renderPassCI);

		// pipeline
		auto vert = readFileAsBinary(getShadersPath() + "uniform/uniform.vert.spv");
		auto frag = readFileAsBinary(getShadersPath() + "uniform/uniform.frag.spv");
		
		auto vertShaderModule = createShaderModule(vert);
		auto fragShaderModule = createShaderModule(frag);

		std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{
			{
				.stage = vk::ShaderStageFlagBits::eVertex,
				.module = vertShaderModule,
				.pName = "main",
			},
			{
				.stage = vk::ShaderStageFlagBits::eFragment,
				.module = fragShaderModule,
				.pName = "main",
			},
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputCI{
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &Vertices.binding,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = Vertices.attributes.data(),
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCI{
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False,
		};

		vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1,
			.scissorCount = 1,
		};

		vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicStateCI{
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates,
		};

		vk::PipelineRasterizationStateCreateInfo rasterizationCI{
			.depthClampEnable = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eBack,
			.frontFace = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable = vk::False,
			.lineWidth = 1.0f,
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
			.sampleShadingEnable = vk::False,
		};

		vk::PipelineColorBlendAttachmentState colorblendAttachmentState{
			.blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR |
								vk::ColorComponentFlagBits::eG |
								vk::ColorComponentFlagBits::eB |
								vk::ColorComponentFlagBits::eA,
		};

		vk::PipelineColorBlendStateCreateInfo colorBlending{
			.logicOpEnable = vk::False,
			.attachmentCount = 1,
			.pAttachments = &colorblendAttachmentState,
		};

		vk::PipelineLayoutCreateInfo layoutCI{
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
		};

		pipelineLayout = device.createPipelineLayout(layoutCI);

		vk::GraphicsPipelineCreateInfo pipelineCI{
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputCI,
			.pInputAssemblyState = &inputAssemblyCI,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationCI,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = nullptr,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicStateCI,
			.layout = pipelineLayout,
			.renderPass = renderpass,
			.subpass = 0,
		};

		graphicsPipeline = device.createGraphicsPipeline(nullptr, pipelineCI).value;

		device.destroyShaderModule(vertShaderModule);
		device.destroyShaderModule(fragShaderModule);

		// framebuffer
		createFrameBuffers();

		// commmandpool
		vk::CommandPoolCreateInfo commandPoolCI{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = findQueueFamilies(physicalDevice, surface).graphicsFamily.value(),
		};

		commandPool = device.createCommandPool(commandPoolCI);

		// commandbuffer
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		for (auto i = 0; i < commandBuffers.size(); i++) {
			vk::CommandBufferAllocateInfo allocInfo{
				.commandPool = commandPool,
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = 1,
			};

			commandBuffers[i] = device.allocateCommandBuffers(allocInfo).front();
		}

		// sync object
		vk::FenceCreateInfo fenceCI{
			.flags = vk::FenceCreateFlagBits::eSignaled,
		};

		inflightFences.resize(MAX_FRAMES_IN_FLIGHT);
		renderSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		presentReadySemaphores.resize(MAX_FRAMES_IN_FLIGHT);

		for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			inflightFences[i] = device.createFence(fenceCI);
			renderSemaphores[i] = device.createSemaphore({});
			presentReadySemaphores[i] = device.createSemaphore({});
		}

		vk::Buffer stagingBuffer;
		vk::DeviceMemory stagingBufferMemory;
		
		// vertexbuffer
		auto size = sizeof(Vertices.vertices[0]) * Vertices.vertices.size();
		createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

		auto data = device.mapMemory(stagingBufferMemory, 0, size);
		memcpy(data, Vertices.vertices.data(), size);
		device.unmapMemory(stagingBufferMemory);

		createBuffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, Vertices.buffer, Vertices.memory);

		copyBuffer(stagingBuffer, Vertices.buffer, size);

		device.destroyBuffer(stagingBuffer);
		device.freeMemory(stagingBufferMemory);

		// indexbuffer
		size = sizeof(Indices.indices[0]) * Indices.indices.size();
		createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

		data = device.mapMemory(stagingBufferMemory, 0, size);
		memcpy(data, Indices.indices.data(), size);
		device.unmapMemory(stagingBufferMemory);

		createBuffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, Indices.buffer, Indices.memory);

		copyBuffer(stagingBuffer, Indices.buffer, size);

		device.destroyBuffer(stagingBuffer);
		device.freeMemory(stagingBufferMemory);
	}

	void createFrameBuffers() {
		frameBuffers.resize(swapChainImageViews.size());

		for (auto i = 0; i < frameBuffers.size(); i++) {
			vk::ImageView attachments[]{
				swapChainImageViews[i],
			};
			
			vk::FramebufferCreateInfo framebufferCI{
				.renderPass = renderpass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = swapChainExtent.width,
				.height = swapChainExtent.height,
				.layers = 1,
			};

			frameBuffers[i] = device.createFramebuffer(framebufferCI);
		}
	}

	void destroyFrameBuffers() {
		for (auto i = 0; i < frameBuffers.size(); i++) {
			device.destroyFramebuffer(frameBuffers[i]);
		}
	}

	void recordCommand(vk::CommandBuffer commandbuffer, uint32_t imageIndex) {
		vk::CommandBufferBeginInfo beginInfo{};
		commandbuffer.begin(beginInfo);

		vk::ClearValue clearValue{ {0.0f, 0.0f, 0.0f, 1.0f} };
		vk::RenderPassBeginInfo renderPassInfo{
			.renderPass = renderpass,
			.framebuffer = frameBuffers[imageIndex],
			.renderArea = {
				.offset {0, 0},
				.extent{swapChainExtent}
			},
			.clearValueCount = 1,
			.pClearValues = &clearValue,
		};

		commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		commandbuffer.setViewport(0, vk::Viewport{
			.x = 0,
			.y = 0,
			.width = (float)swapChainExtent.width,
			.height = (float)swapChainExtent.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
			});

		commandbuffer.setScissor(0, vk::Rect2D{
			.offset {0, 0},
			.extent {swapChainExtent},
			});

		commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		// vertex, index binding
		vk::DeviceSize offsets[]{ 0 };
		commandbuffer.bindVertexBuffers(0, Vertices.buffer, offsets);
		commandbuffer.bindIndexBuffer(Indices.buffer, 0, vk::IndexType::eUint16);

		// bind descriptor sets
		commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		// draw();
		commandbuffer.drawIndexed(Indices.indices.size(), 1, 0, 0, 0);

		commandbuffer.endRenderPass();
		commandbuffer.end();
	}

	void drawFrame() {
		std::ignore = device.waitForFences(inflightFences[currentFrame], vk::False, UINT64_MAX);

		if (framebufferResized) {
			recreateSwapChain();
			framebufferResized = false;
		}

		auto result{ device.acquireNextImageKHR(swapChain, UINT64_MAX, renderSemaphores[currentFrame], nullptr) };
		if (result.result == vk::Result::eSuboptimalKHR) {
			framebufferResized = true;
		}

		device.resetFences(inflightFences[currentFrame]);

		commandBuffers[currentFrame].reset();
		
		uint32_t imageIndex{result.value};
		updateUniformBuffer(currentFrame);

		recordCommand(commandBuffers[currentFrame], imageIndex);

		vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
		vk::SubmitInfo submitInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderSemaphores[currentFrame],
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffers[currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &presentReadySemaphores[currentFrame],
		};

		graphicsQueue.submit(submitInfo, inflightFences[currentFrame]);

		vk::PresentInfoKHR presentInfo{
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &presentReadySemaphores[currentFrame],
			.swapchainCount = 1,
			.pSwapchains = &swapChain,
			.pImageIndices = &imageIndex,
		};

		auto presentResult = vkQueuePresentKHR((VkQueue&)presentQueue, &(VkPresentInfoKHR&)presentInfo);
		if (presentResult == VkResult::VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
};

int main() {
	auto app = new Uniform();
	app->run();

	return 0;
}