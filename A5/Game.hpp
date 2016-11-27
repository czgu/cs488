#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "Chunk.hpp"
#include "ChunkManager.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#define NUM_JOINT 32

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

class Game : public CS488Window {
public:
	Game();
	virtual ~Game();

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
    void initShader();
    void initGameWorld();
	void initCamera();

    // -- Open GL variables
    ShaderProgram m_shader;
    GLint PV_uni;

    // -- Render variables
    glm::mat4 m_perspective;
	glm::mat4 m_view;
    glm::mat4 m_translate;
    glm::mat4 m_rotate;

	LightSource m_light;
    bool mouse_button_pressed[3];
    glm::vec2 mouse_position;

    ChunkManager* worldManager;

	void updateViewMatrix();
    // Game logic variables
    glm::vec3 player_position;
    glm::vec3 player_facing;
    glm::vec3 player_right;
    // Control Variables
};
