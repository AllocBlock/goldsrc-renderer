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
		),
		glm::vec3(1.0, 0.0, 0.0)
	);
	Visualizer.addLine(
		Visualize::Line{
			glm::vec3(0.0f, -5.0f, 0.0f),
			glm::vec3(0.0f, 5.0f, 0.0f)
		},
		glm::vec3(0.0, 1.0, 0.0)
	);
	for (int i = 0; i < 100; ++i)
	{
		glm::vec3 Blue = glm::vec3(0.0, 0.0, 1.0);
		for (int k = 0; k < 100; ++k)
		{
			glm::vec3 P = glm::vec3(i / 100.0, 4, k / 100.0);
			Visualizer.addPoint(P, Blue);
		}
	}

	Visualizer.addSphere(glm::vec3(2, 0, 0), glm::vec3(1, 1, 1), glm::vec3(1, 1, 0));
	Visualizer.addSphere(glm::vec3(3.5, 0, 0), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0, 1, 1));
	Visualizer.addSphere(glm::vec3(2, 0, 2), glm::vec3(0.6, 0.6, 0.6), glm::vec3(1, 0, 1));
	
	Visualizer.addCube(glm::vec3(2, 0, 4), glm::vec3(1, 1, 1), glm::vec3(1, 1, 0));
	Visualizer.addCube(glm::vec3(3.5, 0, 4), glm::vec3(0.5, 0.5, 0.5), glm::vec3(0, 1, 1));
	Visualizer.addCube(glm::vec3(2, 0, 6), glm::vec3(0.6, 0.6, 0.6), glm::vec3(1, 0, 1));

	Visualizer.start();
	return 0;
}