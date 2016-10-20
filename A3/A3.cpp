#include "A3.hpp"
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

using namespace glm;

static bool show_gui = true;

const size_t CIRCLE_PTS = 48;

const string interaction_mode_description[] = {
    "Position/Orientation (P)",
    "Joints (J)"
};

const int button_types[] = {GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOUSE_BUTTON_RIGHT};

//----------------------------------------------------------------------------------------
// Constructor
A3::A3(const std::string & luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_vao_meshData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0),
	  m_vao_arcCircle(0),
	  m_vbo_arcCircle(0),
      options_circle(false),
      options_z_buffer(true),
      options_backface_culling(false),
      options_frontface_culling(false),
      picking_required(false)
{

}

//----------------------------------------------------------------------------------------
// Destructor
A3::~A3()
{

}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A3::init()
{
	// Set the background colour.
	glClearColor(0.35, 0.35, 0.35, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_arcCircle);
	glGenVertexArrays(1, &m_vao_meshData);
	enableVertexShaderInputSlots();

	processLuaSceneFile(m_luaSceneFile);


	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshConsolidator
	// class.
	unique_ptr<MeshConsolidator> meshConsolidator (new MeshConsolidator{
			getAssetFilePath("cube.obj"),
			getAssetFilePath("sphere.obj"),
			getAssetFilePath("suzanne.obj")
            });

	// Acquire the BatchInfoMap from the MeshConsolidator.
	meshConsolidator->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshConsolidator and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshConsolidator);

	mapVboDataToVertexShaderInputLocations();

	initPerspectiveMatrix();

	initViewMatrix();

    reset_all();

	initLightSources();

	// Exiting the current scope calls delete automatically on meshConsolidator freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
}

//----------------------------------------------------------------------------------------
void A3::processLuaSceneFile(const std::string & filename) {
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	// std::string assetFilePath = getAssetFilePath(filename.c_str());
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	m_rootNode = std::shared_ptr<SceneNode>(import_lua(filename));
	if (!m_rootNode) {
		std::cerr << "Could not open " << filename << std::endl;
	}
}


//----------------------------------------------------------------------------------------
void A3::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();

	m_shader_arcCircle.generateProgramObject();
	m_shader_arcCircle.attachVertexShader( getAssetFilePath("arc_VertexShader.vs").c_str() );
	m_shader_arcCircle.attachFragmentShader( getAssetFilePath("arc_FragmentShader.fs").c_str() );
	m_shader_arcCircle.link();
}

//----------------------------------------------------------------------------------------
void A3::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

		CHECK_GL_ERRORS;
	}

	//-- Enable input slots for m_vao_arcCircle:
	{
		glBindVertexArray(m_vao_arcCircle);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_arc_positionAttribLocation = m_shader_arcCircle.getAttribLocation("position");
		glEnableVertexAttribArray(m_arc_positionAttribLocation);

		CHECK_GL_ERRORS;
	}


	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void A3::uploadVertexDataToVbos (
		const MeshConsolidator & meshConsolidator
) {
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexPositionBytes(),
				meshConsolidator.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

    // Generate VBO to store all vertex normal data
    {
        glGenBuffers(1, &m_vbo_vertexNormals);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

        glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexNormalBytes(),
            meshConsolidator.getVertexNormalDataPtr(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERRORS;
    }

	// Generate VBO to store the trackball circle.
	{
		glGenBuffers( 1, &m_vbo_arcCircle );
		glBindBuffer( GL_ARRAY_BUFFER, m_vbo_arcCircle );

		float *pts = new float[ 2 * CIRCLE_PTS ];
		for( size_t idx = 0; idx < CIRCLE_PTS; ++idx ) {
			float ang = 2.0 * M_PI * float(idx) / CIRCLE_PTS;
			pts[2*idx] = cos( ang );
			pts[2*idx+1] = sin( ang );
		}

		glBufferData(GL_ARRAY_BUFFER, 2*CIRCLE_PTS*sizeof(float), pts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A3::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;

	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_arcCircle);

	// Tell GL how to map data from the vertex buffer "m_vbo_arcCircle" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_arcCircle);
	glVertexAttribPointer(m_arc_positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A3::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
}


//----------------------------------------------------------------------------------------
void A3::initViewMatrix() {
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
			vec3(0.0f, 1.0f, 0.0f));
}

//----------------------------------------------------------------------------------------
void A3::initLightSources() {
	// World-space position
	m_light.position = vec3(-2.0f, 5.0f, 0.5f);
	m_light.rgbIntensity = vec3(0.8f); // White light
}

//----------------------------------------------------------------------------------------
void A3::uploadCommonSceneUniforms() {
	m_shader.enable();
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
		CHECK_GL_ERRORS;


		//-- Set LightSource uniform for the scene:
		{
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.position));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;
		}

		//-- Set background light ambient intensity
		{
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.05f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
		}
	}
	m_shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A3::appLogic()
{
	// Place per frame, application logic here ...

	uploadCommonSceneUniforms();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A3::guiLogic()
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


		// Add more gui elements here here ...

		if( ImGui::Button( "Reset Position (I)" ) ) {
            reset_position();
		}

		if( ImGui::Button( "Reset Orientation (O)" ) ) {
            reset_orientation();
		}

		if( ImGui::Button( "Reset Joints (N)" ) ) {
            reset_joints();
		}

		if( ImGui::Button( "Reset All (A)" ) ) {
            reset_all();
		}

		// Create Button, and check if it was clicked:
		if( ImGui::Button( "Quit (Q)" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	ImGui::Begin("Edit", &showDebugWindow, ImVec2(100,100), opacity,
			windowFlags);
        if ( ImGui::Button( "Undo (U)" ) ) {
            undo();
        }

        if ( ImGui::Button( "Redo (R)" ) ) {
            redo();
        }

	ImGui::End();

	ImGui::Begin("Options", &showDebugWindow, ImVec2(100,100), opacity,
			windowFlags);
        ImGui::Checkbox("Circle (C)", &options_circle);
        ImGui::Checkbox("Z-buffer (Z)", &options_z_buffer);
        ImGui::Checkbox("Backface culling (B)", &options_backface_culling);
        ImGui::Checkbox("Frontface culling (F)", &options_frontface_culling);

        ImGui::BeginGroup();
            for (int i = 0; i < 2; i++) {
                ImGui::PushID(i);
                if (ImGui::RadioButton("##Col", &interaction_mode, i)) {
                }
                ImGui::SameLine();
                ImGui::Text("%s", interaction_mode_description[i].c_str());
                ImGui::PopID();
            }

        ImGui::EndGroup();

	ImGui::End();
}

//----------------------------------------------------------------------------------------
//
static vec3 invertColour(vec3 rgb) {
    return vec3(1, 1, 1) - rgb;
}

// Update mesh specific shader uniforms:
static void updateShaderUniforms(
		const ShaderProgram & shader,
		const GeometryNode & node,
		const glm::mat4 & viewMatrix,
        const SceneNode* parent,
        const bool picking
) {

	shader.enable();
	{
		//-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = viewMatrix;
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;

		//-- Set NormMatrix:
		location = shader.getUniformLocation("NormalMatrix");
		mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
		glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
		CHECK_GL_ERRORS;


		//-- Set Material values:
		location = shader.getUniformLocation("material.kd");
		vec3 kd = node.material.kd;
        if (parent->isSelected) {
            kd = invertColour(kd);
        }

		glUniform3fv(location, 1, value_ptr(kd));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.ks");
		vec3 ks = node.material.ks;
		glUniform3fv(location, 1, value_ptr(ks));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.shininess");
		glUniform1f(location, node.material.shininess);
		CHECK_GL_ERRORS;

        location = shader.getUniformLocation("picking");
        glUniform1i(location, picking);
        CHECK_GL_ERRORS;

        location = shader.getUniformLocation("objectId");
        glUniform1i(location, node.m_nodeId);
        CHECK_GL_ERRORS;
	}
	shader.disable();

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A3::draw() {

	if (options_z_buffer) {
        glEnable( GL_DEPTH_TEST );
    }

    if (options_backface_culling || options_frontface_culling) {
        glEnable( GL_CULL_FACE );

        if (options_backface_culling && options_frontface_culling) {
            glCullFace( GL_FRONT_AND_BACK );
        } else if (options_backface_culling) {
            glCullFace( GL_BACK );
        } else { // frontface
            glCullFace( GL_FRONT );
        }
    }

    if (picking_required) {
	    renderSceneGraph(*m_rootNode, true);
        glFlush();
        glFinish();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        unsigned char rgb[3];
        glReadPixels((int)mouse_position.x, m_windowHeight - (int)mouse_position.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);

        // background green colour is not 0
        if (rgb[1] == 0) {
            int objectId = rgb[0];
            m_rootNode->selectSceneNode(objectId);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        picking_required = false;
    }

	renderSceneGraph(*m_rootNode);

	if (options_z_buffer) {
        glDisable( GL_DEPTH_TEST );
    }

    if (options_backface_culling || options_frontface_culling) {
        glDisable( GL_CULL_FACE );
    }

    if (options_circle) {
	    renderArcCircle();
    }
}

//----------------------------------------------------------------------------------------
void A3::renderSceneGraph(const SceneNode & root, bool picking) {

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);

	// This is emphatically *not* how you should be drawing the scene graph in
	// your final implementation.  This is a non-hierarchical demonstration
	// in which we assume that there is a list of GeometryNodes living directly
	// underneath the root node, and that we can draw them in a loop.  It's
	// just enough to demonstrate how to get geometry and materials out of
	// a GeometryNode and onto the screen.

	// You'll want to turn this into recursive code that walks over the tree.
	// You can do that by putting a method in SceneNode, overridden in its
	// subclasses, that renders the subtree rooted at every node.  Or you
	// could put a set of mutually recursive functions in this class, which
	// walk down the tree from nodes of different types.
    //
    renderNode(&root, m_view * m_translate, picking, NULL, true);

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void A3::renderNode(const SceneNode* node, glm::mat4 trans, bool picking, const SceneNode* parent, bool isRoot) {
    trans = trans * node->trans;
    if (isRoot) {
        trans = trans * m_rotate;
    }

    if (node->m_nodeType == NodeType::SceneNode) {
    }
    else if (node->m_nodeType == NodeType::GeometryNode) {
        const GeometryNode* geometryNode = static_cast<const GeometryNode *>(node);
		updateShaderUniforms(m_shader, *geometryNode, trans, parent, picking);

		// Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
		BatchInfo batchInfo = m_batchInfoMap[geometryNode->meshId];

		//-- Now render the mesh:
		m_shader.enable();
		glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
		m_shader.disable();
    } else { // JointNode
        const JointNode* jointNode = static_cast<const JointNode *>(node);
        trans = trans * jointNode->get_x_rotate() * jointNode->get_y_rotate();
    }

    for (const SceneNode* children : node->children) {
        renderNode(children, trans, picking, node);
    }
}

//----------------------------------------------------------------------------------------
// Draw the trackball circle.
void A3::renderArcCircle() {
	glBindVertexArray(m_vao_arcCircle);

	m_shader_arcCircle.enable();
		GLint m_location = m_shader_arcCircle.getUniformLocation( "M" );
		float aspect = float(m_framebufferWidth)/float(m_framebufferHeight);
		glm::mat4 M;
		if( aspect > 1.0 ) {
			M = glm::scale( glm::mat4(), glm::vec3( 0.5/aspect, 0.5, 1.0 ) );
		} else {
			M = glm::scale( glm::mat4(), glm::vec3( 0.5, 0.5*aspect, 1.0 ) );
		}
		glUniformMatrix4fv( m_location, 1, GL_FALSE, value_ptr( M ) );
		glDrawArrays( GL_LINE_LOOP, 0, CIRCLE_PTS );
	m_shader_arcCircle.disable();

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A3::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A3::cursorEnterWindowEvent (
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
bool A3::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

    vec2 new_mouse_position = vec2(xPos, yPos);
	// Fill in with event handling code...
    for (int i = 0; i < 3; i++) {
        if (mouse_button_pressed[i]) {
            vec2 delta = (mouse_position - new_mouse_position);
            if (interaction_mode == 0) { // Position/Orientation
                delta = delta / 200;
                if (i == 0) { // left
                    m_translate = glm::translate(m_translate, vec3(-delta.x/3, delta.y/3, 0));
                } else if (i == 1) { // middle
                    m_translate = glm::translate(m_translate, vec3(0, 0, -delta.y));
                } else { // right
                    // ArcBall model
                    // Imagine the sphere is in the middle of the screen
                    // Map coords to ndc
		            float aspect = float(m_framebufferWidth)/float(m_framebufferHeight);
                    vec2 new_mouse_ndc = vec2(aspect * (new_mouse_position.x / m_framebufferWidth * 2 - 1), -(new_mouse_position.y / m_framebufferHeight * 2 - 1));
                    vec2 mouse_ndc = vec2(aspect * (mouse_position.x / m_framebufferWidth * 2 - 1), -(mouse_position.y / m_framebufferHeight * 2 - 1));
                    float radius = 0.5;
                    // Make sure both start and end are in the circle
                    if (glm::length2(new_mouse_ndc) >= radius * radius ||
                        glm::length2(mouse_ndc) >= radius * radius) {
                        continue;
                    }

                    float z1 = glm::sqrt(radius * radius - mouse_ndc.x * mouse_ndc.x - mouse_ndc.y * mouse_ndc.y);
                    float z2 = glm::sqrt(radius * radius - new_mouse_ndc.x * new_mouse_ndc.x - new_mouse_ndc.y * new_mouse_ndc.y);
                    vec3 s = glm::normalize(vec3(mouse_ndc, z1));
                    vec3 t = glm::normalize(vec3(new_mouse_ndc, z2));

                    vec3 p = glm::cross(s, t);
                    float angle = glm::acos(glm::abs(glm::dot(s, t)));

                    m_rotate = glm::rotate(mat4(), angle, p) * m_rotate;
                }
            } else if (interaction_mode == 1) { // Joints
                delta = delta / 10;
                if (i == 1) {
                    moveJoint(&*m_rootNode, delta.y, 0);
                } else if (i == 2) {
                    moveJoint(&*m_rootNode, 0, delta.x);
                }
            }
        }
    }
    mouse_position = new_mouse_position;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A3::mouseButtonInputEvent (
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

            if (interaction_mode == 1 && button_types[0] == button && actions == GLFW_PRESS) {
                picking_required = true;
            }
            if (interaction_mode == 1 && button_types[0] != button && actions == GLFW_RELEASE) {
                if (command_stack.curr + 1 < command_stack.snapshots.size()) {
                    command_stack.snapshots.erase(
                        command_stack.snapshots.begin() + command_stack.curr + 1,
                        command_stack.snapshots.end());

                }
                command_stack.snapshots.push_back(takeSnapShot());
                command_stack.curr++;
            }
        }
    }

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A3::mouseScrollEvent (
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
bool A3::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initPerspectiveMatrix();
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A3::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if( action == GLFW_PRESS ) {
		if( key == GLFW_KEY_M ) {
			show_gui = !show_gui;
			eventHandled = true;
		} else if (key == GLFW_KEY_I) {
            reset_position();
			eventHandled = true;
        } else if (key == GLFW_KEY_O) {
            reset_orientation();
			eventHandled = true;
        } else if (key == GLFW_KEY_N) {
            reset_joints();
			eventHandled = true;
        } else if (key == GLFW_KEY_A) {
            reset_all();
			eventHandled = true;
        } else if (key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(m_window, GL_TRUE);
			eventHandled = true;
        } else if (key == GLFW_KEY_U) {
            undo();
			eventHandled = true;
        } else if (key == GLFW_KEY_R) {
            redo();
			eventHandled = true;
        } else if (key == GLFW_KEY_C) {
            options_circle = !options_circle;
			eventHandled = true;
        } else if (key == GLFW_KEY_Z) {
            options_z_buffer = !options_z_buffer;
			eventHandled = true;
        } else if (key == GLFW_KEY_B) {
            options_backface_culling = !options_backface_culling;
			eventHandled = true;
        } else if (key == GLFW_KEY_F) {
            options_frontface_culling = !options_frontface_culling;
			eventHandled = true;
        } else if (key == GLFW_KEY_P) {
            interaction_mode = 0;
			eventHandled = true;
        } else if (key == GLFW_KEY_J) {
            interaction_mode = 1;
			eventHandled = true;
        }
	}
	// Fill in with event handling code...

	return eventHandled;
}

// Menu items and actions
void A3::reset_position() {
    m_translate = mat4();
}

void A3::reset_orientation() {
    m_rotate = mat4();
}

void resetJointTree(SceneNode* node) {
    if (node->m_nodeType == NodeType::JointNode) {
        JointNode* jointNode = static_cast<JointNode *>(node);
        jointNode->reset_joint();
    }

    for (SceneNode* children : node->children) {
        resetJointTree(children);
    }
}

void A3::reset_joints() {
    // iterate the tree
    resetJointTree(&*m_rootNode);
    command_stack.snapshots.clear();
    command_stack.curr = 0;
    command_stack.snapshots.push_back(takeSnapShot());
}

void A3::reset_all() {
    reset_position();
    reset_orientation();
    reset_joints();
}

void A3::undo() {
    if (command_stack.snapshots.size() > 0 && command_stack.curr > 0) {
        SnapShot s = command_stack.snapshots[--command_stack.curr];
        applySnapShot(&s);
    }
}

void A3::redo() {
    if (command_stack.snapshots.size() > 0 && command_stack.curr + 1 < command_stack.snapshots.size())
    {
        SnapShot s = command_stack.snapshots[++command_stack.curr];
        applySnapShot(&s);
    }
}

void A3::moveJoint(SceneNode* node, double delta_x, double delta_y) {
    if (node->isSelected) {
        JointNode* jointNode = static_cast<JointNode *>(node);

        jointNode->rotate_joint(jointNode->m_joint_x, delta_x);
        jointNode->rotate_joint(jointNode->m_joint_y, delta_y);
    }

    for (SceneNode* children : node->children) {
        moveJoint(children, delta_x, delta_y);
    }
}

SnapShot A3::takeSnapShot() {
    int i = 0;
    SnapShot s;
    takeSnapShotHelper(&s, &*m_rootNode, &i);

    return s;
}

void A3::takeSnapShotHelper(SnapShot* snap, SceneNode* node, int* i) {
    if (node->m_nodeType == NodeType::JointNode) {
        JointNode* jointNode = static_cast<JointNode *>(node);
        snap->angle_x[*i] = jointNode->m_joint_x.curr;
        snap->angle_y[*i] = jointNode->m_joint_y.curr;
        *i = *i + 1;
    }
    for (SceneNode* children : node->children) {
        takeSnapShotHelper(snap, children, i);
    }
}

void A3::applySnapShot(SnapShot* snap) {
    int i = 0;
    applySnapShotHelper(snap, &*m_rootNode, &i);
}

void A3::applySnapShotHelper(SnapShot* snap, SceneNode* node, int* i) {
    if (node->m_nodeType == NodeType::JointNode) {
        JointNode* jointNode = static_cast<JointNode *>(node);

        jointNode->m_joint_x.curr = snap->angle_x[*i];
        jointNode->m_joint_y.curr = snap->angle_y[*i];
        *i = *i + 1;
    }

    for (SceneNode* children : node->children) {
        applySnapShotHelper(snap, children, i);
    }
}
