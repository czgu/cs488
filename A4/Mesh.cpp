#include <iostream>
#include <fstream>
#include <cfloat>

#include <glm/ext.hpp>

// #include "cs488-framework/ObjFileDecoder.hpp"
#include "Mesh.hpp"

Mesh::Mesh( const std::string& fname )
	: m_vertices()
	, m_faces()
{
	std::string code;
	double vx, vy, vz;
	size_t s1, s2, s3;

	std::ifstream ifs( fname.c_str() );

    if (ifs.fail()) {
        ifs.open(("Assets/" + fname).c_str());
    }

    double minx = DBL_MAX, miny = DBL_MAX, minz = DBL_MAX;
    double maxx = DBL_MIN, maxy = DBL_MIN, maxz = DBL_MIN;

	while( ifs >> code ) {
		if( code == "v" ) {
			ifs >> vx >> vy >> vz;
            minx = vx < minx ? vx : minx;
            maxx = vx > maxx ? vx : maxx;

            miny = vy < miny ? vy : miny;
            maxy = vy > maxy ? vy : maxy;

            minz = vz < minz ? vz : minz;
            maxz = vz > maxz ? vz : maxz;

			m_vertices.push_back( glm::vec3( vx, vy, vz ) );
		} else if( code == "f" ) {
			ifs >> s1 >> s2 >> s3;
			m_faces.push_back( Triangle( s1 - 1, s2 - 1, s3 - 1 ) );
		}
	}
    for (Triangle& tri : m_faces) {
        tri.cache(m_vertices);
    }

    double dx = maxx - minx;
    double dy = maxy - miny;
    double dz = maxz - minz;

    double maxd = dx > dy ? dx : dy;
    maxd = dz > maxd ? dz : maxd;

    boundingBox = NULL;
    if (!(dx < 0.005 || dy < 0.005 || dz < 0.005)) {
        boundingBox = new NonhierBox(glm::vec3(minx, miny, minz), maxd);
    }
}

std::ostream& operator<<(std::ostream& out, const Mesh& mesh)
{
  out << "mesh {";
  /*
  
  for( size_t idx = 0; idx < mesh.m_verts.size(); ++idx ) {
  	const MeshVertex& v = mesh.m_verts[idx];
  	out << glm::to_string( v.m_position );
	if( mesh.m_have_norm ) {
  	  out << " / " << glm::to_string( v.m_normal );
	}
	if( mesh.m_have_uv ) {
  	  out << " / " << glm::to_string( v.m_uv );
	}
  }

*/
  out << "}";
  return out;
}

bool Mesh::intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) {
    bool found = false;
    if (boundingBox && !boundingBox->intersect(orig, dir, p, n, t)) {
        return false;
    }

    t = DBL_MAX;

    for (Triangle& tri : m_faces) {
        glm::vec3 a = m_vertices[tri.v1];
        glm::vec3 b = m_vertices[tri.v2];
        glm::vec3 c = m_vertices[tri.v3];

        glm::vec3& plane_normal = tri.plane_normal;
        double& D = tri.D;

        double pd = glm::dot(plane_normal, dir);

        if (glm::abs((float)pd) < 0.005) {
            continue;
        }

        double tt = - (D + glm::dot(plane_normal, orig)) / pd;

        if (tt < 0) {
            continue;
        }

        glm::vec3 pp = orig + (float)tt * dir;

        glm::vec3& e0 = tri.e0;
        glm::vec3 v0 = pp - a;
        if (glm::dot(plane_normal, glm::cross(e0, v0)) < 0) {
            continue;
        }

        glm::vec3& e1 = tri.e1;
        glm::vec3 v1 = pp - b;
        if (glm::dot(plane_normal, glm::cross(e1, v1)) < 0) {
            continue;
        }

        glm::vec3& e2 = tri.e2;
        glm::vec3 v2 = pp - c;
        if (glm::dot(plane_normal, glm::cross(e2, v2)) < 0) {
            continue;
        }

        found = true;
        if (tt < t) {
            t = tt;
            p = pp;
            n = plane_normal;
        }
    }
    return found;
}


void Triangle::cache(std::vector<glm::vec3>& vertices) {
    glm::vec3 a = vertices[this->v1];
    glm::vec3 b = vertices[this->v2];
    glm::vec3 c = vertices[this->v3];

    glm::vec3 ba = b - a;
    glm::vec3 ca = c - a;

    this->plane_normal = glm::cross(ba, ca);
    this->D = - glm::dot(a, plane_normal);

    this->e0 = b - a;
    this->e1 = c - b;
    this->e2 = a - c;

}
