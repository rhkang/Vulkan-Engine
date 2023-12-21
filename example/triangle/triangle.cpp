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
	}
private:
	void prepare() {

	}
};

int main() {
	auto app = new Triangle();
	app->run();

	return EXIT_SUCCESS;
}