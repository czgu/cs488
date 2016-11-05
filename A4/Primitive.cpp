#include "Primitive.hpp"
#include "polyroots.hpp"

#include <glm/ext.hpp>
#include <iostream>
#include <cfloat>

Primitive::~Primitive()
{
}

Sphere::~Sphere()
{
}

Cube::~Cube()
{
}

NonhierSphere::~NonhierSphere()
{
}

NonhierBox::~NonhierBox()
{
}

void swap(double& a, double& b) {
    double temp = b;
    b = a;
    a = temp;
}

bool Sphere::intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) {
    double a = glm::dot(dir, dir);
    double b = 2 * glm::dot(dir, orig);
    double c = glm::dot(orig, orig) - 1;

    double roots[2];
    int num_root = quadraticRoots(a, b, c, roots);

    if (num_root < 1) {
        return false;
    }

    if (roots[0] > roots[1]) {
        swap(roots[0], roots[1]);
    }

    int i;
    for (i = 0; i < 2; i++) {
        if (roots[i] > 0) {
            break;
        }
    }
    if (i > 1) {
        // All negative
        return false;
    }

    t = roots[i];
    p = orig + (float)t * dir;
    n = glm::normalize(p);
    return true;
}

bool Cube::intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) {
    glm::vec3 p1 = glm::vec3(0, 0, 0);
    glm::vec3 p2 = glm::vec3(1, 1, 1);

    double txmin, txmax, tymin, tymax, tzmin, tzmax;
    double tmin = DBL_MIN, tmax = DBL_MAX;
    int normal_idx[2] = {0, 0};

    // X ------------------------------------
    if (dir.x != 0) {
        txmin = (p1.x - orig.x) / dir.x;
        txmax = (p2.x - orig.x) / dir.x;

        if (txmin > txmax) {
            swap(txmin, txmax);
        }

        if (txmin > tmin) {
            tmin = txmin;
            normal_idx[0] = 0;
        }

        if (txmax < tmax) {
            tmax = txmax;
            normal_idx[1] = 0;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.x > orig.x || p2.x < orig.x) {
        return false;
    }

    // Y -------------------------------------
    if (dir.y != 0) {
        tymin = (p1.y - orig.y) / dir.y;
        tymax = (p2.y - orig.y) / dir.y;

        if (tymin > tymax) {
            swap(tymin, tymax);
        }

        if (tymin > tmin) {
            tmin = tymin;
            normal_idx[0] = 1;
        }

        if (tymax < tmax) {
            tmax = tymax;
            normal_idx[1] = 1;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.y > orig.y || p2.y < orig.y) {
        return false;
    }
    // Z -------------------------------------
    if (dir.z != 0) {
        tzmin = (p1.z - orig.z) / dir.z;
        tzmax = (p2.z - orig.z) / dir.z;

        if (tzmin > tzmax) {
            swap(tzmin, tzmax);
        }

        if (tzmin > tmin) {
            tmin = tzmin;
            normal_idx[0] = 2;
        }

        if (tzmax < tmax) {
            tmax = tzmax;
            normal_idx[1] = 2;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.z > orig.z || p2.z < orig.z) {
        return false;
    }

    // Final Calculation
    double roots[2];
    roots[0] = tmin;
    roots[1] = tmax;

    int i;
    for (i = 0; i < 2; i++) {
        if (roots[i] > 0) {
            break;
        }
    }
    if (i > 1 || roots[i] == DBL_MIN || roots[i] == DBL_MAX) {
        // All negative
        return false;
    }

    t = roots[i];
    p = orig + (float)t * dir;

    // calculate normal
    glm::vec3 diff = p - glm::vec3(1 / 2, 1 / 2, 1 / 2);

    n = glm::vec3(0, 0, 0);
    n[normal_idx[i]] = 1.0f * (diff[normal_idx[i]] > 0 ? 1 : -1);

    if (glm::dot(n, dir) > 0) {
        n = -n;
    }

    return true;

}

bool NonhierSphere::intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) {
    double a = glm::dot(dir, dir);
    double b = 2 * glm::dot(dir, orig - m_pos);
    double c = glm::dot(orig - m_pos, orig - m_pos) - m_radius * m_radius;

    double roots[2];
    int num_root = quadraticRoots(a, b, c, roots);

    if (num_root < 1) {
        return false;
    }

    if (roots[0] > roots[1]) {
        swap(roots[0], roots[1]);
    }

    int i;
    for (i = 0; i < 2; i++) {
        if (roots[i] > 0) {
            break;
        }
    }
    if (i > 1) {
        // All negative
        return false;
    }

    t = roots[i];
    p = orig + (float)t * dir;
    n = glm::normalize(p - m_pos);
    return true;
}


bool NonhierBox::intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) {
    glm::vec3 p1 = m_pos;
    glm::vec3 p2 = m_pos + glm::vec3(m_size, m_size, m_size);

    double txmin, txmax, tymin, tymax, tzmin, tzmax;
    double tmin = DBL_MIN, tmax = DBL_MAX;
    int normal_idx[2] = {0, 0};

    // X ------------------------------------
    if (dir.x != 0) {
        txmin = (p1.x - orig.x) / dir.x;
        txmax = (p2.x - orig.x) / dir.x;

        if (txmin > txmax) {
            swap(txmin, txmax);
        }

        if (txmin > tmin) {
            tmin = txmin;
            normal_idx[0] = 0;
        }

        if (txmax < tmax) {
            tmax = txmax;
            normal_idx[1] = 0;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.x > orig.x || p2.x < orig.x) {
        return false;
    }

    // Y -------------------------------------
    if (dir.y != 0) {
        tymin = (p1.y - orig.y) / dir.y;
        tymax = (p2.y - orig.y) / dir.y;

        if (tymin > tymax) {
            swap(tymin, tymax);
        }

        if (tymin > tmin) {
            tmin = tymin;
            normal_idx[0] = 1;
        }

        if (tymax < tmax) {
            tmax = tymax;
            normal_idx[1] = 1;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.y > orig.y || p2.y < orig.y) {
        return false;
    }
    // Z -------------------------------------
    if (dir.z != 0) {
        tzmin = (p1.z - orig.z) / dir.z;
        tzmax = (p2.z - orig.z) / dir.z;

        if (tzmin > tzmax) {
            swap(tzmin, tzmax);
        }

        if (tzmin > tmin) {
            tmin = tzmin;
            normal_idx[0] = 2;
        }

        if (tzmax < tmax) {
            tmax = tzmax;
            normal_idx[1] = 2;
        }

        if (tmax < tmin || tmax < 0) {
            return false;
        }
    } else if (p1.z > orig.z || p2.z < orig.z) {
        return false;
    }

    // Final Calculation
    double roots[2];
    roots[0] = tmin;
    roots[1] = tmax;

    int i;
    for (i = 0; i < 2; i++) {
        if (roots[i] > 0) {
            break;
        }
    }
    if (i > 1 || roots[i] == DBL_MIN || roots[i] == DBL_MAX) {
        // All negative
        return false;
    }

    t = roots[i];
    p = orig + (float)t * dir;

    // calculate normal
    glm::vec3 diff = p - (m_pos + glm::vec3(m_size / 2, m_size / 2, m_size / 2));

    n = glm::vec3(0, 0, 0);
    n[normal_idx[i]] = 1.0f * (diff[normal_idx[i]] > 0 ? 1 : -1);

    if (glm::dot(n, dir) > 0) {
        n = -n;
    }

    return true;
}
