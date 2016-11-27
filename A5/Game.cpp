#include "Game.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/norm.hpp>

#include <cstdlib>

using namespace glm;

static bool show_gui = true;

const size_t CIRCLE_PTS = 48;

const int button_types[] = {GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOUSE_BUTTON_RIGHT};

//----------------------------------------------------------------------------------------
// Constructor
Game::Game()
{

}

//----------------------------------------------------------------------------------------
// Destructor
Game::~Game()
{

}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void Game::init()
{
	// Set the background colour.
	glClearColor(0.35, 0.35, 0.35, 1.0);

    initShader();
	initCamera();
    initGameWorld();
}

void Game::initGameWorld() {
    this->worldManager = new ChunkManager(&m_shader);
}

//----------------------------------------------------------------------------------------
void Game::initShader() {
	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

    // Set up uniforms
    PV_uni = m_shader.getUniformLocation("PV");
}

//----------------------------------------------------------------------------------------
void Game::initCamera()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perspective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);

    player_position = vec3(0.0f, 12.0f, 8.0f);
    player_facing = vec3(0.0, 0.0, 1.0f);
    player_right = vec3(1.0f, 0.0, 0.0f);
}


//----------------------------------------------------------------------------------------
void Game::updateViewMatrix() {
	m_view = glm::lookAt(player_position, player_facing + player_position, vec3(0.0f, 1.0f, 0.0f));

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void Game::appLogic()
{
	// Place per frame, application logic here ...
    worldManager->update(player_position);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void Game::guiLogic()
{
	if( !show_gui ) {
		return;
	}

	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Application", &showDebugWindow, ImVec2(100,100), opacity,
			windowFlags);

		// Create Button, and check if it was clicked:
		if( ImGui::Button( "Quit (Q)" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void Game::draw() {
    // Render world
    updateViewMatrix();
    mat4 PV = m_perspective * m_view;
    //* m_translate * m_rotate;
    m_shader.enable();
        glEnable( GL_DEPTH_TEST );
        glEnable( GL_CULL_FACE );
        //glCullFace( GL_FRONT );

		glUniformMatrix4fv( PV_uni, 1, GL_FALSE, value_ptr( PV ) );
        worldManager->render();

    m_shader.disable();

	CHECK_GL_ERRORS;
}


//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void Game::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool Game::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool Game::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

    vec2 new_mouse_position = vec2(xPos, yPos);
	// Fill in with event handling code...
	if (!ImGui::IsMouseHoveringAnyWindow()) {
        if (this->mouse_button_pressed[0]) {
            vec2 delta = new_mouse_position - mouse_position;
            m_rotate = m_rotate * glm::rotate(mat4(), (float)((delta.x/m_windowWidth) * 2 * PI), vec3(0.0f, 1.0f, 0.0f));
            player_facing = vec3(m_rotate * vec4(0, 0, 1, 1));
            player_right = glm::cross(player_facing, vec3(0.0f, 1.0f, 0.0f));
        }
    }

    mouse_position = new_mouse_position;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool Game::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	// Fill in with event handling code...
    if (!ImGui::IsMouseHoveringAnyWindow()) {
        if (actions == GLFW_PRESS || actions == GLFW_RELEASE) {
            for (int i = 0; i < 3; i++) {
                if (button_types[i] == button) {
                    mouse_button_pressed[i] = (actions == GLFW_PRESS);
                    eventHandled = true;
                }
            }
        }
    }

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool Game::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool Game::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initCamera();
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool Game::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if( action == GLFW_PRESS || action == GLFW_REPEAT ) {
        if (key == GLFW_KEY_SPACE) {
            // Player up
            player_position.y += 1.0f;
            eventHandled = true;
        }

        if (key == GLFW_KEY_LEFT_SHIFT) {
            // Player up
            player_position.y -= 1.0f;
            eventHandled = true;
        }

        if (key == GLFW_KEY_A) {
            // Player up
            player_position -= player_right;
            //eventHandled = true;
        }


        if (key == GLFW_KEY_D) {
            // Player up
            player_position += player_right;
            //eventHandled = true;
        }

        if (key == GLFW_KEY_W) {
            // Player up
            player_position += player_facing;
            eventHandled = true;
        }

        if (key == GLFW_KEY_S) {
            // Player up
            player_position -= player_facing;
            eventHandled = true;
        }

        if (key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(m_window, GL_TRUE);
            eventHandled = true;
        }
	}
	// Fill in with event handling code...

	return eventHandled;
}

