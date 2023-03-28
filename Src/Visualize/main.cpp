#include "Visualizer.h"

int main()
{
	CVisualizer Visualizer;
	Visualizer.init();
	Visualizer.addTriangle(
		Visualize::Triangle(
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 1.0f)
		)
	);
	Visualizer.start();
	return 0;
}