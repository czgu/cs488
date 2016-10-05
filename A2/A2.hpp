#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include <glm/glm.hpp>

#include <vector>

// Set a global maximum number of vertices in order to pre-allocate VBO data
// in one shot, rather than reallocating each frame.
const GLsizei kMaxVertices = 1000;
#define PI 3.141592653
#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)<(b)?(b):(a))
#define ABS(a) ((a)>0?(a):-(a))

// Convenience class for storing vertex data in CPU memory.
// Data should be copied over to GPU memory via VBO storage before rendering.
class VertexData {
public:
	VertexData();

	std::vector<glm::vec2> positions;
	std::vector<glm::vec3> colours;
	GLuint index;
	GLsizei numVertices;
};

class LineData {
public:
    LineData(int x, int y, float r, float g, float b);
    int a;
    int b;
    glm::vec3 colour;
};
class Line {
public:
    Line(glm::vec4 a, glm::vec4 b, LineData* data);
    glm::vec4 a;
    glm::vec4 b;
    bool outside;
    LineData* data;
};

class A2 : public CS488Window {
public:
	A2();
	virtual ~A2();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	void createShaderProgram();
	void enableVertexAttribIndices();
	void generateVertexBuffers();
	void mapVboDataToVertexAttributeLocation();
	void uploadVertexDataToVbos();

	void initLineData();

	void setLineColour(const glm::vec3 & colour);

	void drawLine (
			const glm::vec2 & v0,
			const glm::vec2 & v1
	);

    void drawViewPortLine (
        const glm::vec2& v0,
        const glm::vec2&& v1,
        const glm::vec3& colour
    );

    glm::vec2 NDCWindowToViewPort(
        const glm::vec2& v
    );

    glm::vec2 viewPortToGLCoord(
        const glm::vec2& v
    );

    void initDrawModel();
    void initTransformation();

    void clip(Line& line, glm::vec3 point, glm::vec3 normal);

    float cClamp(float orig, float delta, float min, float max, bool cycle=true);

    glm::mat4 makeScaleMatrix(glm::vec3 xyz);
    glm::mat4 makeRotationMatrix(glm::vec3 xyz);
    glm::mat4 makeTranslationMatrix(glm::vec3 xyz);
    glm::mat4 makeProjectionMatrix(float fov, float n, float f);
    glm::vec4 homogenize(glm::vec4 v);

	ShaderProgram m_shader;

	GLuint m_vao;            // Vertex Array Object
	GLuint m_vbo_positions;  // Vertex Buffer Object
	GLuint m_vbo_colours;    // Vertex Buffer Object

	VertexData m_vertexData;

	glm::vec3 m_currentLineColour;


    // Local
    glm::mat4 model_rotation;
    glm::mat4 model_translation;
    glm::mat4 model_scale;

    // View
    glm::mat4 view_rotation;
    glm::mat4 view_translation;
    glm::vec3 perspective; // fov, near, far

    // Viewport
    glm::vec2 view_port_origin;
    glm::vec2 view_port_size; // width, height
    glm::vec2 view_port_corner;

    // Drawing informations
    std::vector<glm::vec3> points;
    std::vector<LineData> linesData;

    // enum mapping for key pressed:
    // 0 - O, 1 - N, 2 - P, 3 - R, 4 - T 5 - S, 6 - V
    int interaction_mode;

    bool trackDragging[3];
    float lastMouseX[3];
};
