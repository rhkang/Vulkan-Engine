#include <VEbase.h>

class Uniform : public VEbase {
public:
	Uniform() : VEbase("Vulkan Application - Uniform buffer") {

	}

	~Uniform() {

	}

	void run() {
		init();
		prepare();
		mainLoop();
	}
private:
	vk::RenderPass renderpass;

	void prepare() {
		// renderpass
		vk::AttachmentDescription attachment{
			.format = swapChainFormat,
			.samples = vk::SampleCountFlagBits::e1,
			.loadOp = vk::AttachmentLoadOp::eClear,
			.storeOp = vk::AttachmentStoreOp::eNone,
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

		vk::PipelineShaderStageCreateInfo shaderStages[]{
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
		};

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCI{
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = vk::False,
		};

		vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		vk::PipelineDynamicStateCreateInfo dynamicCI{
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates,
		};

		vk::PipelineRasterizationStateCreateInfo rasterizationCI{
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

		vk::GraphicsPipelineCreateInfo pipielineCI{
			.stageCount = 2,
			.pStages = shaderStages,

			/*stageCount_
			pStages_
			pVertexInputState_
			pInputAssemblyState_
			pTessellationState_
			pViewportState_
			pRasterizationState_
			pMultisampleState_
			pDepthStencilState_
			pColorBlendState_
			pDynamicState_
			layout_
			renderPass_
			subpass_
			basePipelineHandle_
			basePipelineIndex_*/
		};

		// framebuffer
		// commmandpool
		// commandbuffer
		// sync object
		// vertexbuffer
		// indexbuffer
		// uniformbuffer
	}

	void createFrameBuffers() {

	}

	void destroyFrameBuffers() {

	}

	void drawFrame() {

	}
};

int main() {
	auto app = new Uniform();
	app->run();

	return 0;
}