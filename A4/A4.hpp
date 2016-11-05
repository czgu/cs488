#pragma once

#include <glm/glm.hpp>

#include "SceneNode.hpp"
#include "Light.hpp"
#include "Image.hpp"

void A4_Render(
		// What to render
		SceneNode * root,

		// Image to write to, set to a given width and height
		Image & image,

		// Viewing parameters
		const glm::vec3 & eye,
		const glm::vec3 & view,
		const glm::vec3 & up,
		double fovy,

		// Lighting parameters
		const glm::vec3 & ambient,
		const std::list<Light *> & lights
);

Ray* makePrimitiveRay(int x, int y, int w, int h, glm::vec3 a, glm::vec3 b, const glm::vec3 eye, const glm::vec3 view);
glm::vec3 illuminate(
    Intersection* intersection,
    const std::list<Light *>& lights,
    const glm::vec3& ambient,
    Ray* ray,
    SceneNode* root);
