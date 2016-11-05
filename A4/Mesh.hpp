#pragma once

#include <vector>
#include <iosfwd>
#include <string>

#include <glm/glm.hpp>

#include "Primitive.hpp"

struct Triangle
{
	size_t v1;
	size_t v2;
	size_t v3;

	Triangle( size_t pv1, size_t pv2, size_t pv3 )
		: v1( pv1 )
		, v2( pv2 )
		, v3( pv3 )
	{}

    double D;
    glm::vec3 plane_normal;
    glm::vec3 e0, e1, e2;

    void cache(std::vector<glm::vec3>& vertices);
};

// A polygonal mesh.
class Mesh : public Primitive {
public:
    Mesh( const std::string& fname );
    virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) override;
  
private:
	std::vector<glm::vec3> m_vertices;
	std::vector<Triangle> m_faces;
    NonhierBox* boundingBox;

    friend std::ostream& operator<<(std::ostream& out, const Mesh& mesh);
};
