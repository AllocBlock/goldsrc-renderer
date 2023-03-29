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
	Visualizer.addLine(
		Visualize::Line{
			glm::vec3(0.0f, -5.0f, 0.0f),
			glm::vec3(0.0f, 5.0f, 0.0f)
		}
	);
	for (int i = 0; i < 100; ++i)
	{
		for (int k = 0; k < 100; ++k)
		{
			glm::vec3 P = glm::vec3(i / 100.0, 4, k / 100.0);
			Visualizer.addPoint(P);
		}
	}
	Visualizer.start();
	return 0;
}