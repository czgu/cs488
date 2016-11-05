#include <glm/ext.hpp>

#include "A4.hpp"
#include "Material.hpp"
#include "PhongMaterial.hpp"

using namespace glm;
using namespace std;

unsigned char* ReadBMP(const char* filename, int& width, int& height, int& row_padded);

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
) {

  // Fill in raytracing code here...
  int img_w, img_h, img_row_padded;
  unsigned char* background_img = ReadBMP("sunset.bmp", img_w, img_h, img_row_padded);

  std::cout << "Calling A4_Render(\n" <<
		  "\t" << *root <<
          "\t" << "Image(width:" << image.width() << ", height:" << image.height() << ")\n"
          "\t" << "eye:  " << glm::to_string(eye) << std::endl <<
		  "\t" << "view: " << glm::to_string(view) << std::endl <<
		  "\t" << "up:   " << glm::to_string(up) << std::endl <<
		  "\t" << "fovy: " << fovy << std::endl <<
          "\t" << "ambient: " << glm::to_string(ambient) << std::endl <<
		  "\t" << "lights{" << std::endl;

	for(const Light * light : lights) {
		std::cout << "\t\t" <<  *light << std::endl;
	}
	std::cout << "\t}" << std::endl;
	std:: cout <<")" << std::endl;

	size_t h = image.height();
	size_t w = image.width();

    vec3 _view = glm::normalize(view);
    vec3 _right = glm::normalize(cross(_view, up));
    vec3 _up = glm::normalize(cross(_view, _right));

    vec3 a, b;
    a = _right * glm::tan(glm::radians(fovy / 2)) * (w/h) / (w / 2);
    b = _up * glm::tan(glm::radians(fovy / 2)) / (h / 2);

	for (uint y = 0; y < h; ++y) {
		for (uint x = 0; x < w; ++x) {
            Ray* ray = makePrimitiveRay(x, y, w, h, a, b, eye, _view);
            Intersection* intersection = root->intersect(ray);
            vec3 color = illuminate(
                intersection, lights, ambient, ray, root);

            if (!intersection) {
                int img_x = (((float)x/w) * img_w);
                int img_y = ((1.0f - (float)y/h) * img_h);

                if (img_y == img_h) {
                    img_y -= 1;
                }

                int coord = (img_y) * img_row_padded + img_x * 3;
                color[2] = background_img[coord + 0] / 255.0;
                color[0] = background_img[coord + 1] / 255.0;
                color[1] = background_img[coord + 2] / 255.0;
            }

            double r = color[0], g = color[1], b = color[2];

			image(x, y, 0) = r;
			image(x, y, 1) = g;
			image(x, y, 2) = b;

            delete ray;
            if (intersection) {
                delete intersection;
            }
		}
	}
}

Ray* makePrimitiveRay(int x, int y, int w, int h, glm::vec3 a, glm::vec3 b, const glm::vec3 eye, const glm::vec3 view) {
    Ray* ray = new Ray();
    ray->eye = eye;
    ray->dir = glm::normalize(view + (x - (w/2)) * a + (y - (h/2)) * b);

    return ray;
}

glm::vec3 illuminate(
    Intersection* intersection,
    const std::list<Light* >& lights,
    const glm::vec3& ambient,
    Ray* ray,
    SceneNode* root
) {
    vec3 c(0, 0, 0);
    if (intersection) {
        PhongMaterial* m = dynamic_cast<PhongMaterial *>(intersection->material);
        if (m == NULL) {
            return c;
        }

        vec3 v = -ray->dir;
        vec3 n = glm::normalize(intersection->normal);

        for (Light* light : lights) {
            vec3 l = light->position - intersection->point;
            double d = glm::length(l);
            l = glm::normalize(l);

            // Cast Shadow Rays
            Ray light_ray;
            light_ray.eye = intersection->point + 0.01 * l;
            light_ray.dir = l;

            Intersection* lightIntersection = root->intersect(&light_ray);
            if (lightIntersection) {
                continue;
            }

            double nl = glm::dot(l, n);
            vec3 r = glm::normalize((float)(2 * nl) * n - l);
            double rv = glm::dot(r, v);

            if (rv < 0) rv = 0;
            if (nl < 0) nl = 0;

            double att = light->falloff[0] +
                light->falloff[1] * d +
                light->falloff[2] * d * d;

            nl = nl / att;
            rv = glm::pow(rv, m->m_shininess) / att;

            for (int i = 0; i < 3; i++) { // rgb
                c[i] += (m->m_kd[i] * light->colour[i] * nl) +
                        (m->m_ks[i] * light->colour[i] * rv);
            }
        }
        for (int i = 0; i < 3; i++) {
            c[i] += m->m_kd[i] * ambient[i];
        }
    }
    return c;
}


// FROM STACKOVERFLOW: http://stackoverflow.com/questions/9296059/read-pixel-value-in-bmp-file
unsigned char* ReadBMP(const char* filename, int& width, int& height, int& row_padded)
{
    int i;
    FILE* f = fopen(filename, "rb");

    if(f == NULL)
        throw "Argument Exception";

    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

    // extract image height and width from header
    width = *(int*)&info[18];
    height = *(int*)&info[22];

    row_padded = (width*3 + 3) & (~3);

    unsigned char* data = new unsigned char[row_padded * height];
    unsigned char tmp;

    for(int i = 0; i < height; i++)
    {
        fread(data + i * row_padded, sizeof(unsigned char), row_padded, f);
        for(int j = 0; j < width*3; j += 3)
        {
            // Convert (B, G, R) to (R, G, B)
            tmp = data[j + i * row_padded];
            data[j] = data[j+2 + i * row_padded];
            data[j+2 + i * row_padded] = tmp;
        }
    }

    fclose(f);
    return data;
}
