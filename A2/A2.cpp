#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <string>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/ext.hpp>

using namespace glm;

int screen_width;
int screen_height;

const string interaction_mode_name[] = {
    "Rotate View (O)",
    "Translate View (N)",
    "Perspective (P)",
    "Rotate Model (R)",
    "Translate Model (T)",
    "Scale Model (S)",
    "Viewport (V)"
};

//----------------------------------------------------------------------------------------
// Constructor
VertexData::VertexData()
	: numVertices(0),
	  index(0)
{
	positions.resize(kMaxVertices);
	colours.resize(kMaxVertices);
}

//----------------------------------------------------------------------------------------
LineData::LineData(int x, int y, float r, float g, float b)
    : a(x), b(y), colour(vec3(r, g, b))
{
}

//----------------------------------------------------------------------------------------
Line::Line(vec4 a, vec4 b, LineData* data)
    : a(a), b(b), data(data), outside(false)
{}

//----------------------------------------------------------------------------------------
// Constructor
A2::A2()
	: m_currentLineColour(vec3(0.0f)),
      show_detailed(false)
{
    for (int i = 0; i < 3; i++) {
        trackDragging[i] = false;
        lastMouseX[i] = 0;
    }

}

//----------------------------------------------------------------------------------------
// Destructor
A2::~A2()
{

}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A2::init()
{
	// Set the background colour.
	glClearColor(0.3, 0.5, 0.7, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao);

	enableVertexAttribIndices();

	generateVertexBuffers();

	mapVboDataToVertexAttributeLocation();

    glfwGetWindowSize(m_window, &screen_width, &screen_height);

    initDrawModel();

    initTransformation();
}

void A2::initDrawModel() {
    // Cube
    for (int b = 0; b < 8 ; b ++) {
        int y = (b & 4) != 0 ? 1 : -1; // Down, then Up
        int x = (b & 2) != 0 ? 1 : -1; // Left, then Right
        int z = (b & 1) != 0 ? 1 : -1; // Far, then Near

        glm::vec3 v(x, y, z);
        points.push_back(v);
    }
    // Local Coordinates: start at 8
    points.push_back(vec3(0, 0, 0)); // origin
    points.push_back(vec3(1, 0, 0)); // x
    points.push_back(vec3(0, 1, 0)); // y
    points.push_back(vec3(0, 0, 1)); // z

    // Global Coodinates: start at 12
    points.push_back(vec3(0, 0, 0)); // origin
    points.push_back(vec3(1, 0, 0)); // x
    points.push_back(vec3(0, 1, 0)); // y
    points.push_back(vec3(0, 0, 1)); // z

    // Now setup lines
    // Cube
    int down_left_top = 0;
    int down_left_bottom = 1;
    int down_right_top = 2;
    int down_right_bottom = 3;

    int up_left_top = 4;
    int up_left_bottom = 5;
    int up_right_top = 6;
    int up_right_bottom = 7;

    // Lines for bottom
    linesData.push_back(
        LineData(down_left_top, down_left_bottom, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_left_top, down_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_right_bottom, down_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_right_bottom, down_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_right_bottom, down_left_bottom, 0, 0, 0)
    );

    // Sides
    linesData.push_back(
        LineData(down_left_top, up_left_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_right_top, up_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_left_bottom, up_left_bottom, 0, 0, 0)
    );

    linesData.push_back(
        LineData(down_right_bottom, up_right_bottom, 0, 0, 0)
    );

    // Top
    linesData.push_back(
        LineData(up_left_top, up_left_bottom, 0, 0, 0)
    );

    linesData.push_back(
        LineData(up_left_top, up_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(up_right_bottom, up_right_top, 0, 0, 0)
    );

    linesData.push_back(
        LineData(up_right_bottom, up_left_bottom, 0, 0, 0)
    );

    // Local coordinates
    linesData.push_back(LineData(8, 9, 1, 0, 0));
    linesData.push_back(LineData(8, 10, 0, 1, 0));
    linesData.push_back(LineData(8, 11, 0, 0, 1));

    // Global coordinates
    linesData.push_back(LineData(12, 13, 1, 0.2, 0.2));
    linesData.push_back(LineData(12, 14, 0.2, 1, 0.2));
    linesData.push_back(LineData(12, 15, 0.2, 0.2, 1));
}

void A2::initTransformation() {
    model_rotation = vec3(0, 0, 0);
    model_translation = vec3(0, 0, 0);
    model_scale = vec3(1, 1, 1);

    // View
    view_rotation = vec3(0, 0, 0);
    view_translation = vec3(0, 1, 9);
    perspective = vec3(30.0f, 1.0f, 15.0f);

    // Viewport

    view_port_origin = vec2(screen_width * 0.05, screen_height * 0.05);
    view_port_size = vec2(screen_width * 0.9, screen_height * 0.9); // width, height

    // Interaction
    interaction_mode = 3;
}

//----------------------------------------------------------------------------------------
void A2::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();
}

//----------------------------------------------------------------------------------------
void A2::enableVertexAttribIndices()
{
	glBindVertexArray(m_vao);

	// Enable the attribute index location for "position" when rendering.
	GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray(positionAttribLocation);

	// Enable the attribute index location for "colour" when rendering.
	GLint colourAttribLocation = m_shader.getAttribLocation( "colour" );
	glEnableVertexAttribArray(colourAttribLocation);

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A2::generateVertexBuffers()
{
	// Generate a vertex buffer to store line vertex positions
	{
		glGenBuffers(1, &m_vbo_positions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * kMaxVertices, nullptr,
				GL_DYNAMIC_DRAW);


		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	// Generate a vertex buffer to store line colors
	{
		glGenBuffers(1, &m_vbo_colours);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);

		// Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * kMaxVertices, nullptr,
				GL_DYNAMIC_DRAW);


		// Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A2::mapVboDataToVertexAttributeLocation()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao);

	// Tell GL how to map data from the vertex buffer "m_vbo_positions" into the
	// "position" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
	GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
	glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_colours" into the
	// "colour" vertex attribute index for any bound shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
	GLint colorAttribLocation = m_shader.getAttribLocation( "colour" );
	glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//---------------------------------------------------------------------------------------
void A2::initLineData()
{
	m_vertexData.numVertices = 0;
	m_vertexData.index = 0;
}

//---------------------------------------------------------------------------------------
void A2::setLineColour (
		const glm::vec3 & colour
) {
	m_currentLineColour = colour;
}

//---------------------------------------------------------------------------------------
void A2::drawLine(
		const glm::vec2 & v0,   // Line Start (NDC coordinate)
		const glm::vec2 & v1    // Line End (NDC coordinate)
) {

	m_vertexData.positions[m_vertexData.index] = v0;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;
	m_vertexData.positions[m_vertexData.index] = v1;
	m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
	++m_vertexData.index;

	m_vertexData.numVertices += 2;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A2::appLogic()
{
	// Place per frame, application logic here ...
    vector<vec4> translated_points;
    for (vec3 p: points) {
        translated_points.push_back(vec4(p, 1));
    }

    // Construct scale matrix
    mat4 local_scale_mat = transpose(mat4(
        model_scale.x, 0, 0, 0,
        0, model_scale.y, 0, 0,
        0, 0, model_scale.z, 0,
        0, 0, 0, 1
    ));

    // Apply scale to box points
    for (int i = 0; i < 8; i++) {
        translated_points[i] = local_scale_mat * translated_points[i];
    }

    // Construct translation and rotation matrices
    mat4 local_trans_mat = makeTranslationMatrix(model_translation);
    mat4 local_rotation_mat = makeRotationMatrix(model_rotation);
    mat4 local_trans_rot_mat = local_trans_mat * local_rotation_mat;

    // Apply local rotation/translation to cube points and local coord
    for (int i = 0; i < 12; i ++) {
        translated_points[i] = local_trans_rot_mat * translated_points[i];
    }

    // World to camera
    mat4 view_trans_mat = makeTranslationMatrix(view_translation);
    mat4 view_rotation_mat = makeRotationMatrix(view_rotation);
    mat4 camera_frame = inverse(view_trans_mat * view_rotation_mat);


    // Apply view change to all points
    for (int i = 0; i < 16; i++) {
        translated_points[i] = camera_frame * translated_points[i];

    }

    // Convert points into lines
    vector<Line> lines;
    for (LineData& ld : linesData) {
        lines.push_back(
            Line(
                translated_points[ld.a],
                translated_points[ld.b],
                &ld
            )
        );
    }

    float fov = perspective.x;
    float n = perspective.y;
    float f = perspective.z;

    mat4 projection_mat = makeProjectionMatrix(fov, n, f);

    // Clipping front surface
    for (Line& line: lines) {
        clip(line, vec3(0, 0, -1 * n), vec3(0, 0, -1));

        if (!line.outside) {
            // Project & homogenize
            line.a = homogenize(projection_mat * line.a);
            line.b = homogenize(projection_mat * line.b);
        }
    }


    // Clipping all other 5 sides
    for (Line& line: lines) {
        // Bottom surface
        clip(line, vec3(0, -1, 0), vec3(0, 1, 0));
        // Left surface
        clip(line, vec3(-1, 0, 0), vec3(1, 0, 0));
        // Front surface
        clip(line, vec3(0, 0, 1), vec3(0, 0, -1));
        // Right surface
        clip(line, vec3(1, 0, 0), vec3(-1, 0, 0));
        // Top surface
        clip(line, vec3(0, 1, 0), vec3(0, -1, 0));
    }

    // No point to draw if view port is emppty
    if (view_port_size.x == 0 || view_port_size.y == 0) {
        return;
    }

	// Call at the beginning of frame, before drawing lines:
	initLineData();

    int point = 0;
    for (Line& line: lines) {
        if (!line.outside) {
            drawViewPortLine(
                NDCWindowToViewPort(vec2(line.a)),
                NDCWindowToViewPort(vec2(line.b)),
                line.data->colour
            );
        }
    }

    // Draw the frame boundaries
    drawViewPortLine(
        view_port_origin,
        view_port_origin + vec2(view_port_size.x, 0),
        vec3(0.3, 0.3, 0.3)
    );

    drawViewPortLine(
        view_port_origin,
        view_port_origin + vec2(0, view_port_size.y),
        vec3(0.3, 0.3, 0.3)
    );

    drawViewPortLine(
        view_port_origin + vec2(0, view_port_size.y),
        view_port_origin + vec2(view_port_size.x, view_port_size.y),
        vec3(0.3, 0.3, 0.3)
    );

    drawViewPortLine(
        view_port_origin + vec2(view_port_size.x, 0),
        view_port_origin + vec2(view_port_size.x, view_port_size.y),
        vec3(0.3, 0.3, 0.3)
    );
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A2::guiLogic()
{
	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Properties", &showDebugWindow, ImVec2(100,100), opacity,
			windowFlags);


		// Add more gui elements here here ...


		// Create Button, and check if it was clicked:
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		if( ImGui::Button( "Reset" ) ) {
            initTransformation();
		}

        ImGui::BeginGroup();
            for (int btn_id = 0; btn_id < 7; btn_id++) {
                ImGui::PushID(btn_id);
                ImGui::Text("%s", interaction_mode_name[btn_id].c_str());
                ImGui::SameLine();
                if (ImGui::RadioButton("##Col", &interaction_mode, btn_id)) {

                }
                ImGui::PopID();
            }
        ImGui::EndGroup();

        ImGui::Text("FOV:%f Near:%f Far:%f", perspective.x, perspective.y, perspective.z);
        if (show_detailed) {
            ImGui::Text("View rotation %f %f %f", view_rotation.x, view_rotation.y, view_rotation.z);
            ImGui::Text("View translation %f %f %f",  view_translation.x, view_translation.y, view_translation.z);
            ImGui::Text("Model rotation %f %f %f", model_rotation.x, model_rotation.y, model_rotation.z);
            ImGui::Text("Model translation %f %f %f", model_translation.x, model_translation.y, model_translation.z);
            ImGui::Text("Model scale %f %f %f", model_scale.x, model_scale.y, model_scale.z);
            ImGui::Text("View port (%f,%f) size (%f,%f)", view_port_origin.x, view_port_origin.y, view_port_size.x, view_port_size.y);
        }


		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();
}

//----------------------------------------------------------------------------------------
void A2::uploadVertexDataToVbos() {

	//-- Copy vertex position data into VBO, m_vbo_positions:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * m_vertexData.numVertices,
				m_vertexData.positions.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}

	//-- Copy vertex colour data into VBO, m_vbo_colours:
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * m_vertexData.numVertices,
				m_vertexData.colours.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A2::draw()
{
	uploadVertexDataToVbos();

	glBindVertexArray(m_vao);

	m_shader.enable();
		glDrawArrays(GL_LINES, 0, m_vertexData.numVertices);
	m_shader.disable();

	// Restore defaults
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A2::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A2::cursorEnterWindowEvent (
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
bool A2::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
        for (int i = 0; i < 3; i++) {
            if (this->trackDragging[i]) {
                if (this->lastMouseX[i] == -1) {
                    if (interaction_mode == 6 && i == 0) {
                        // Set the origin on first click
                        view_port_origin = vec2(xPos, yPos);
                        view_port_size = vec2(0, 0);
                    }
                } else {
                    float delta = (xPos - this->lastMouseX[i]) / 5;

                    if (interaction_mode == 0) { // O
                        view_rotation[i] = cClamp(view_rotation[i], delta, 0, 2 * PI);
                    } else if (interaction_mode == 1) { // N
                        view_translation[i] += delta;
                    } else if (interaction_mode == 2) { // P
                        if (i == 0) { // Left
                            perspective[0] = cClamp(perspective[0], delta, 5, 160, false);
                        } else if (i == 1) { // Middle
                            perspective[1] = cClamp(perspective[1], delta, 0.05, perspective[2], false);
                        } else if(i == 2) { // Right
                            perspective[2] = cClamp(perspective[2], delta, perspective[1], 30, false);
                        }
                    } else if (interaction_mode == 3) { // R
                        model_rotation[i] = cClamp(model_rotation[i], delta, 0, 2 * PI);
                    } else if (interaction_mode == 4) { // T
                        mat3 local_rot_mat = mat3(makeRotationMatrix(model_rotation));
                        vec3 v;
                        v[i] = delta;
                        model_translation += local_rot_mat * v;
                    } else if (interaction_mode == 5) { // S
                        model_scale[i] += delta;
                    } else if (interaction_mode == 6) { // V
                        if (i == 0) { // L
                            // clamp xPos and yPos
                            int cx = glm::clamp((int)xPos, 0, screen_width);
                            int cy = glm::clamp((int)yPos, 0, screen_height);

                            if (cx > view_port_origin.x && cy > view_port_origin.y) {
                                view_port_size = vec2(
                                    cx - view_port_origin.x,
                                    cy - view_port_origin.y
                                );
                            }

                        }
                    }
                }
                this->lastMouseX[i] = xPos;
            }
        }
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A2::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	// Fill in with event handling code...
    if (actions == GLFW_PRESS) {
        if (!ImGui::IsMouseHoveringAnyWindow()) {
            // The user clicked in the window.  If it's the left
            // mouse button, initiate a rotation.
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                this->trackDragging[0] = true;
                this->lastMouseX[0] = -1; // A invalid value, used for initialization in mouseMoveEvent
            }
            if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                this->trackDragging[1] = true;
                this->lastMouseX[1] = -1; // A invalid value, used for initialization in mouseMoveEvent
            }
            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                this->trackDragging[2] = true;
                this->lastMouseX[2] = -1; // A invalid value, used for initialization in mouseMoveEvent
            }
        }
    }
    if (actions == GLFW_RELEASE) {
        if (!ImGui::IsMouseHoveringAnyWindow()) {
            // The user clicked in the window.  If it's the left
            // mouse button, initiate a rotation.
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                this->trackDragging[0] = false;
            }
            if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                this->trackDragging[1] = false;
            }
            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                this->trackDragging[2] = false;
            }
        }
    }



	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A2::mouseScrollEvent (
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
bool A2::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);

	// Fill in with event handling code...

    view_port_origin = vec2(
        (view_port_origin.x / screen_width) * width,
        (view_port_origin.y / screen_height) * height
    );

    view_port_size = vec2(
        (view_port_size.x / screen_width) * width,
        (view_port_size.y / screen_height) * height
    );

    screen_width = width;
    screen_height = height;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A2::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	// Fill in with event handling code...
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_O) {
            interaction_mode = 0;
            eventHandled = true;
        }
        if (key == GLFW_KEY_N) {
            interaction_mode = 1;
            eventHandled = true;
        }
        if (key == GLFW_KEY_P) {
            interaction_mode = 2;
            eventHandled = true;
        }
        if (key == GLFW_KEY_R) {
            interaction_mode = 3;
            eventHandled = true;
        }
        if (key == GLFW_KEY_T) {
            interaction_mode = 4;
            eventHandled = true;
        }
        if (key == GLFW_KEY_S) {
            interaction_mode = 5;
            eventHandled = true;
        }
        if (key == GLFW_KEY_V) {
            interaction_mode = 6;
            eventHandled = true;
        }

        if (key == GLFW_KEY_A) {
            initTransformation();
            eventHandled = true;
        }

        if (key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(m_window, GL_TRUE);
            eventHandled = true;
        }

        if (key == GLFW_KEY_D) {
            show_detailed = !show_detailed;
            eventHandled = true;
        }
    }

	return eventHandled;
}

mat4 A2::makeRotationMatrix(vec3 xyz) {
    mat4 rotationX = mat4(
        1, 0, 0, 0,
        0, cos(xyz.x), -sin(xyz.x), 0,
        0, sin(xyz.x), cos(xyz.x), 0,
        0, 0, 0, 1
    );

    mat4 rotationY = mat4(
        cos(xyz.y), 0, sin(xyz.y), 0,
        0, 1, 0, 0,
        -sin(xyz.y), 0, cos(xyz.y), 0,
        0, 0, 0, 1
    );

    mat4 rotationZ = mat4(
        cos(xyz.z), -sin(xyz.z), 0, 0,
        sin(xyz.z), cos(xyz.z), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    // So the actual order is: translate x, y, and lastly z.
    return transpose(rotationX * rotationY * rotationZ);
}

mat4 A2::makeTranslationMatrix(vec3 xyz) {
    return transpose(mat4(
        1, 0, 0, xyz.x,
        0, 1, 0, xyz.y,
        0, 0, 1, xyz.z,
        0, 0, 0, 1
    ));
}

float A2::cClamp(float orig, float delta, float min, float max, bool cycle) {
    float range = max - min;
    float ret = orig + (delta/100) * range;
    if (cycle) {
        while (ret < min) {
            ret += range;
        }
        while (ret > max) {
            ret -= range;
        }
    } else {
        if (ret < min) {
            ret = min;
        }

        if (ret > max) {
            ret = max;
        }
    }
    return ret;
}

void A2::clip(Line& line, vec3 point, vec3 normal) {
    if (line.outside) {
        return;
    }

    float wecA = dot(vec3(line.a) - point, normal);
    float wecB = dot(vec3(line.b) - point, normal);

    if (wecA < 0 && wecB < 0) { // outside
        line.outside = true;
        return;
    }

    if (wecA >= 0 && wecB >= 0) { // inside
        return;
    }

    // intersection
    vec4 intersection_point = line.a + (wecA / (wecA - wecB)) * (line.b - line.a);

    if (wecA < 0) {
        line.a = intersection_point;
    } else { // (wecB < 0)
        line.b = intersection_point;
    }
}

mat4 A2::makeProjectionMatrix(float fov, float n, float f) {
    return transpose(mat4(
        1/tan(radians(fov/2)), 0, 0, 0,
        0, 1/tan(radians(fov/2)), 0, 0,
        0, 0, -1 * (f+n) / (f-n), -2 * f * n / (f-n),
        0, 0, -1, 0
    ));
}

vec4 A2::homogenize(vec4 v) {
    return vec4(
        v.x / v.w,
        v.y / v.w,
        v.z / v.w,
        1
    );
}

void A2::drawViewPortLine (
    const glm::vec2& v0,
    const glm::vec2&& v1,
    const glm::vec3& colour
) {
    setLineColour(colour);
    drawLine(
        viewPortToGLCoord(v0),
        viewPortToGLCoord(v1)
    );
}

glm::vec2 A2::NDCWindowToViewPort(
    const glm::vec2& v
) {
    float px = (v.x + 1) / 2;
    float py = 1 - (v.y + 1) / 2;
    return view_port_origin + vec2(px * view_port_size.x, py * view_port_size.y);
}

glm::vec2 A2::viewPortToGLCoord(
    const glm::vec2& v
) {
    return vec2(
        (v.x / screen_width) * 2 - 1,
        -1 * ((v.y / screen_height) * 2 - 1)
    );
}
