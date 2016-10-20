#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#define NUM_JOINT 32

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

struct SnapShot {
    float angle_x[NUM_JOINT];
    float angle_y[NUM_JOINT];
};

struct CommandStack {
    std::vector<SnapShot> snapshots;
    int curr;
};

class A3 : public CS488Window {
public:
	A3(const std::string & luaSceneFile);
	virtual ~A3();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string & filename);
	void createShaderProgram();
	void enableVertexShaderInputSlots();
	void uploadVertexDataToVbos(const MeshConsolidator & meshConsolidator);
	void mapVboDataToVertexShaderInputLocations();
	void initViewMatrix();
	void initLightSources();

	void initPerspectiveMatrix();
	void uploadCommonSceneUniforms();
	void renderSceneGraph(const SceneNode &node, bool picking=false);
    void renderNode(const SceneNode* node, glm::mat4 trans, bool picking, const SceneNode* parent, bool isRoot=false);
	void renderArcCircle();

    void moveJoint(SceneNode* node, double delta_x, double delta_y);

	glm::mat4 m_perpsective;
	glm::mat4 m_view;
    glm::mat4 m_translate;
    glm::mat4 m_rotate;

	LightSource m_light;

	//-- GL resources for mesh geometry data:
	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	ShaderProgram m_shader;

	//-- GL resources for trackball circle geometry:
	GLuint m_vbo_arcCircle;
	GLuint m_vao_arcCircle;
	GLint m_arc_positionAttribLocation;
	ShaderProgram m_shader_arcCircle;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::shared_ptr<SceneNode> m_rootNode;

    // Menu control objects and functions
    void reset_position();
    void reset_orientation();
    void reset_joints();
    void reset_all();

    void undo();
    void redo();

    bool options_circle;
    bool options_z_buffer;
    bool options_backface_culling;
    bool options_frontface_culling;

    int interaction_mode;

    bool mouse_button_pressed[3];
    glm::vec2 mouse_position;

    bool picking_required;

    CommandStack command_stack;
    SnapShot takeSnapShot();
    void takeSnapShotHelper(SnapShot* snap, SceneNode* node, int* i);
    void applySnapShot(SnapShot* snap);
    void applySnapShotHelper(SnapShot* snap, SceneNode* node, int* i);
};
