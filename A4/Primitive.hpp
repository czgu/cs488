#pragma once

#include <glm/glm.hpp>

class Primitive {
public:
  virtual ~Primitive();
  virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) = 0;
};

class Sphere : public Primitive {
public:
  virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) override;
  virtual ~Sphere();
};

class Cube : public Primitive {
public:
  virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) override;
  virtual ~Cube();
};

class NonhierSphere : public Primitive {
public:
  virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) override;
  NonhierSphere(const glm::vec3& pos, double radius)
    : m_pos(pos), m_radius(radius)
  {
  }
  virtual ~NonhierSphere();

private:
  glm::vec3 m_pos;
  double m_radius;
};

class NonhierBox : public Primitive {
public:
  virtual bool intersect(glm::vec3 orig, glm::vec3 dir, glm::vec3& p, glm::vec3& n, double& t) override;
  NonhierBox(const glm::vec3& pos, double size)
    : m_pos(pos), m_size(size)
  {
  }
  
  virtual ~NonhierBox();

private:
  glm::vec3 m_pos;
  double m_size;
};
