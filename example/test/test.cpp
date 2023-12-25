#include "VEbase.h"

class TestApplication : public VEbase {
public:
	TestApplication() : VEbase("Vulkan Application - Test") {

	}

	void run() {
		init();
		mainLoop();
		cleanUpBase();
	}
};

int main() {
	auto temp = new TestApplication();
	temp->run();

	return EXIT_SUCCESS;
}