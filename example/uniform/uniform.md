Object
```
struct UBO
- model, view, proj
```

vertex shader:

```
layout(binding = 0) uniform x {
  model
  view
  proj
} ubo <- alias name

gl_Position = ubo.model * ...;
```

### Descriptor Set Structures
set layout 구성, 이 정보를 토대로 pool내에 set을 형성한다.
```
descriptorSetLayoutBinding
- binding = 0
- type : (ex. Uniform Buffer, Storage Buffer, ...)
- count : # of values in the array
- stageFlags : shader stages (ex. vertex)
 
descriptorSetLayout
- bindingCnt
- pBindings
```

Pool 생성 정보는 Set 개수 상한, descriptor 타입 별 개수 상한을 제시한다. 이후 allocation 중 상한을 어기면 메모리 공간이 부족할 때 실패할 가능성이 있음
```
descriptorPool
- poolSize
	- type = uniform buffer
	- descriptorCount
- pPoolSizes
- maxSets

descriptorSet
- pool			: allocation target
- setCnt		: # of desc. sets to allocate
- setlayouts		: desc. set layout
```

GPU VRAM에 Allocation 후 실제 Buffer나 Image 등의 리소스를 가리키도록 update
```
vkUpdateDescriptorSets
  - writeDescriptorSet
	- dstSet : desc. set to update
	- dstBinding : descriptorSetLayoutBinding->binding
	- dstArrayElement : descriptorSetLayoutBinding->descriptorCount
	- desc. type
	- desc. cnt
	- BufferInfo (used to descriptors that refer to buffer)
		- buffer
		- offset
		- range : size of data or just vk_whole_size to overwrite whole anyway
	- imageInfo (... refer to image data)
	- texelBufferView (... refer to buffer views)
```

이렇게 설정한 Description Set을 pipelineLayout을 통해 pipeline에 binding 한다.
```
cmdBuffer.BindDescriptorSets
<- bind point, pipeline layout, first set idx, # of sets to bind, array of sets to bind, array of offsets
```
