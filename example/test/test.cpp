#include "VEbase.h"

class TestApplication : public VEbase {
public:
	TestApplication() : VEbase("Vulkan Application - Test") {

	}

	void run() {
		init();
		mainLoop();
		cleanUp();
	}
};

int main() {
	auto temp = new TestApplication();
	temp->run();

	return EXIT_SUCCESS;
}