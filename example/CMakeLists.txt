# Build a example
function(buildExample EXAMPLE_NAME)
	SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
	message(STATUS "Generating project file for example in ${EXAMPLE_FOLDER}")
	
	# Main
	SET(MAIN_CPP ${EXAMPLE_FOLDER}/${EXAMPLE_NAME}.cpp)
	
	# build 시 포함되어야하는 base폴더 파일
	file(GLOB BASE_SRC "${BASE_DIR}/*.cpp" "${BASE_DIR}/*.h")

	add_executable(${EXAMPLE_NAME}
		${MAIN_CPP}
		${BASE_SRC}
	)

	target_link_libraries(${EXAMPLE_NAME} ${Vulkan_LIBRARY} ${glfw_LIBRARY})

endfunction(buildExample)

# Build all examples
function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
	endforeach(EXAMPLE)
endfunction(buildExamples)

set(EXAMPLES
	test	# library 잘 불러오는지 확인
	triangle
)

buildExamples()