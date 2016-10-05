#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

static const size_t DIM = 16;
static const size_t NUM_COLOURS = 8;
static const float PI = 3.141592653f;

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: active_cell( 0 ),
      current_col( 0 )
{
    this->trackDragging = false;
    this->lastMouseX = 0;

    this->rotation = 0;
    this->zoom = 1;

    this->border = false;
    this->surface = true;

    initColour();
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{
    delete[] cubes;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Set the background colour.
	glClearColor( 0.3, 0.5, 0.7, 1.0 );

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	col_uni = m_shader.getUniformLocation( "colour" );

	initGrid();

    // initialize cube 3d model
    initCube();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt( 
		glm::vec3( 0.0f, float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );

	proj = glm::perspective( 
		glm::radians( 45.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::initGrid()
{
	size_t sz = 3 * 2 * 2 * (DIM+3) + 3*6;

	float *verts = new float[ sz ];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
		verts[ ct ] = -1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = idx-1;
		verts[ ct+3 ] = DIM+1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = idx-1;
		ct += 6;

		verts[ ct ] = idx-1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1;
		verts[ ct+3 ] = idx-1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = DIM+1;
		ct += 6;
	}

    // Unit Square at 0,0 (x,z)
    verts[ ct ] = 0;
    verts[ ct+1 ] = 0;
    verts[ ct+2 ] = 0;

    verts[ ct+3 ] = 1;
    verts[ ct+4 ] = 0;
    verts[ ct+5 ] = 0;

    verts[ ct+6 ] = 0;
    verts[ ct+7 ] = 0;
    verts[ ct+8 ] = 1;

    verts[ ct+9 ] = 1;
    verts[ ct+10 ] = 0;
    verts[ ct+11 ] = 1;

    verts[ ct+12 ] = 1;
    verts[ ct+13 ] = 0;
    verts[ ct+14 ] = 0;

    verts[ ct+15 ] = 0;
    verts[ ct+16 ] = 0;
    verts[ ct+17 ] = 1;

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_grid_vao );
	glBindVertexArray( m_grid_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz*sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete [] verts;

	CHECK_GL_ERRORS;
}


void A1::initCube() {
    cubes = new Cube[DIM * DIM];
    for (int i = 0; i < DIM * DIM; i++) {
        cubes[i].colour = 0;
        cubes[i].height = 0;
    }

    // xyz * 8 points per cube
    size_t vertex_sz = 3 * 8;
	float *verts = new float[ vertex_sz ];

	size_t ct = 0;

    // Add the cube vertices
    for (int b = 0; b < 8; b++) {
        int y = (b & 4) >> 2; // Down, then Up
        int x = (b & 2) >> 1; // Left, then Right
        int z = (b & 1); // Top, then Bottom
        verts[ct] = x;
        verts[ct + 1] = y;
        verts[ct + 2] = z;
        ct += 3;
    }

    // triangle (3) * 2 per surface * 6 surfaces + 2 point * 12 lines
    size_t surface_sz = 3 * 2 * 6 + 2 * 12;
    unsigned int* surface_elements = new unsigned int[surface_sz];

    // Create the cube surface relationship
    int down_left_top = 0;
    int down_left_bottom = 1;
    int down_right_top = 2;
    int down_right_bottom = 3;

    int up_left_top = 4;
    int up_left_bottom = 5;
    int up_right_top = 6;
    int up_right_bottom = 7;

    //Bottom Surface
    surface_elements[0] = down_left_top;
    surface_elements[1] = down_left_bottom;
    surface_elements[2] = down_right_top;

    surface_elements[3] = down_right_top;
    surface_elements[4] = down_left_bottom;
    surface_elements[5] = down_right_bottom;

    // Middle West
    surface_elements[6] = down_left_top;
    surface_elements[7] = down_left_bottom;
    surface_elements[8] = up_left_top;

    surface_elements[9] = up_left_top;
    surface_elements[10] = up_left_bottom;
    surface_elements[11] = down_left_bottom;

    // Middle North
    surface_elements[12] = down_left_top;
    surface_elements[13] = down_right_top;
    surface_elements[14] = up_left_top;

    surface_elements[15] = up_left_top;
    surface_elements[16] = up_right_top;
    surface_elements[17] = down_right_top;

    // Middle East
    surface_elements[18] = down_right_top;
    surface_elements[19] = down_right_bottom;
    surface_elements[20] = up_right_top;

    surface_elements[21] = up_right_top;
    surface_elements[22] = up_right_bottom;
    surface_elements[23] = down_right_bottom;

    // Middle South
    surface_elements[24] = down_left_bottom;
    surface_elements[25] = down_right_bottom;
    surface_elements[26] = up_left_bottom;

    surface_elements[27] = up_left_bottom;
    surface_elements[28] = up_right_bottom;
    surface_elements[29] = down_right_bottom;

    // Top Surface
    surface_elements[30] = up_left_top;
    surface_elements[31] = up_left_bottom;
    surface_elements[32] = up_right_top;

    surface_elements[33] = up_right_top;
    surface_elements[34] = up_right_bottom;
    surface_elements[35] = up_left_bottom;

    // Lines for bottom
    surface_elements[36] = down_left_top;
    surface_elements[37] = down_left_bottom;

    surface_elements[38] = down_left_top;
    surface_elements[39] = down_right_top;

    surface_elements[40] = down_right_bottom;
    surface_elements[41] = down_right_top;

    surface_elements[42] = down_right_bottom;
    surface_elements[43] = down_left_bottom;

    // Sides
    surface_elements[44] = down_left_top;
    surface_elements[45] = up_left_top;

    surface_elements[46] = down_right_top;
    surface_elements[47] = up_right_top;

    surface_elements[48] = down_left_bottom;
    surface_elements[49] = up_left_bottom;

    surface_elements[50] = down_right_bottom;
    surface_elements[51] = up_right_bottom;

    // Top
    surface_elements[52] = up_left_top;
    surface_elements[53] = up_left_bottom;

    surface_elements[54] = up_left_top;
    surface_elements[55] = up_right_top;

    surface_elements[56] = up_right_bottom;
    surface_elements[57] = up_right_top;

    surface_elements[58] = up_right_bottom;
    surface_elements[59] = up_left_bottom;

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_cube_vao );
	glBindVertexArray( m_cube_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_cube_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_cube_vbo );
	glBufferData( GL_ARRAY_BUFFER, vertex_sz*sizeof(float),
		verts, GL_STATIC_DRAW );

    // Create the cube element buffer
    glGenBuffers(1, &m_cube_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cube_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, surface_sz*sizeof(unsigned int),
        surface_elements, GL_STATIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete[] verts;
    delete[] surface_elements;
    verts = NULL;
    surface_elements = NULL;

	CHECK_GL_ERRORS;
}

void A1::initColour(bool alloc) {
    if (alloc) {
        this->colours = new Colour[NUM_COLOURS];
    }

    // We will initialize the palettes to 3 bit rgb colours
    // Black
    this->colours[0].rgb[0] = 0.0f;
    this->colours[0].rgb[1] = 0.0f;
    this->colours[0].rgb[2] = 0.0f;

    // Blue
    this->colours[1].rgb[0] = 0.0f;
    this->colours[1].rgb[1] = 0.0f;
    this->colours[1].rgb[2] = 1.0f;

    // Green
    this->colours[2].rgb[0] = 0.0f;
    this->colours[2].rgb[1] = 1.0f;
    this->colours[2].rgb[2] = 0.0f;

    // Teal
    this->colours[3].rgb[0] = 0.0f;
    this->colours[3].rgb[1] = 1.0f;
    this->colours[3].rgb[2] = 1.0f;

    // Red
    this->colours[4].rgb[0] = 1.0f;
    this->colours[4].rgb[1] = 0.0f;
    this->colours[4].rgb[2] = 0.0f;

    // Pink
    this->colours[5].rgb[0] = 1.0f;
    this->colours[5].rgb[1] = 0.0f;
    this->colours[5].rgb[2] = 1.0f;

    // Yellow
    this->colours[6].rgb[0] = 1.0f;
    this->colours[6].rgb[1] = 1.0f;
    this->colours[6].rgb[2] = 0.0f;

    // White
    this->colours[7].rgb[0] = 1.0f;
    this->colours[7].rgb[1] = 1.0f;
    this->colours[7].rgb[2] = 1.0f;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for 
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		ImGui::Text( "Active Cell: Col %lu, Row %lu", active_cell / DIM, active_cell % DIM );

		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.
        ImGui::BeginGroup();
        for (int button_id = 0; button_id < 8; button_id ++) {
            ImGui::PushID(button_id);
            ImGui::ColorEdit3( "##Colour", this->colours[button_id].rgb );
            ImGui::SameLine();
            if( ImGui::RadioButton( "##Col", &current_col, button_id ) ) {
                // Select this colour.
                this->cubes[this->active_cell].colour = current_col;
            }
            ImGui::PopID();
        }
        ImGui::EndGroup();

		// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in 
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
			showTestWindow = !showTestWindow;
		}

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;
    vec3 center_v = vec3( -float(DIM)/2.0f, 0, -float(DIM)/2.0f );
	W = glm::translate( W, center_v );

    mat4 modifiedView = glm::scale(
        glm::rotate(view, this->rotation, vec3(0.0f, 1.0f, 0.0f)),
        vec3(zoom, zoom, zoom));

	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( modifiedView ) );

        // Draw all the cubes
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cube_ebo );
        glBindVertexArray( m_cube_vao );
        for (int i = 0; i < DIM; i++) {
            for (int j = 0; j < DIM; j++) {
                int height = this->cubes[i * DIM + j].height;
                float *rgb = this->colours[this->cubes[i * DIM + j].colour].rgb;

                for (int k = 0; k < height; k++) {
                    //Draw the Cube!
                    mat4 cube_location;
                    cube_location = glm::translate(
                        cube_location,
                        vec3(i, k, j)
                    );
                    cube_location = glm::translate(
                        cube_location,
                        center_v
                    );

                    glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( cube_location ) );

                    // Surfaces
                    if (this->surface) {
                        glUniform3f( col_uni, rgb[0], rgb[1], rgb[2] );
                        glDrawElements(GL_TRIANGLES, 3 * 6 * 2, GL_UNSIGNED_INT, 0);
                    }
                    // Borders
                    if (this->border) {
                        if (this->surface) {
                            glUniform3f( col_uni, 1 - rgb[0], 1 - rgb[1], 1 - rgb[2] );
                        } else {
                            glUniform3f( col_uni, rgb[0], rgb[1], rgb[2] );
                        }
                        glDrawElements(GL_LINES, 2 * 24 , GL_UNSIGNED_INT, (void *)(3 * 6 * 2 * sizeof(unsigned int)));
                    }

                }
            }
        }

		// Draw the grid now
        // --------------------------------------------------------
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );
		glBindVertexArray( m_grid_vao );
		glUniform3f( col_uni, 1, 1, 1 );
		glDrawArrays( GL_LINES, 0, (3+DIM)*4 );

        // Draw the indicator
        // indicator location
        mat4 indicator_location;
        indicator_location = glm::translate(
            indicator_location,
            vec3(active_cell / DIM, cubes[active_cell].height + 0.01f, active_cell % DIM)
        );
        indicator_location = glm::translate(
            indicator_location,
            center_v
        );
        glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( indicator_location ) );

        // Set the indicator colour
        vec3 indicatorColour = vec3(1.0f, 1.0f, 1.0f);
        if (this->cubes[active_cell].height > 0) {
            // look at the cube color
            float *rgb = this->colours[this->cubes[active_cell].colour].rgb;
            if (rgb[0] >= 0.8f && rgb[1] >= 0.8f && rgb[2] >= 0.8f) {
                indicatorColour = vec3(0.0f, 0.0f, 0.0f);
            }
        }
        glUniform3fv( col_uni, 1, value_ptr( indicatorColour ) );

		glDisable( GL_DEPTH_TEST );
		// Highlight the active square.
        glDrawArrays( GL_TRIANGLES, (3+DIM)*4, 18 );
	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (
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
bool A1::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
        if (this->trackDragging) {
            if (this->lastMouseX == -1) {
            } else {
                float delta = xPos - this->lastMouseX;

                rotation = rotation + (delta/m_windowWidth) * 2 * PI;

                // Make sure rotation is between [0, 2PI]
                while (rotation > 2 * PI) {
                    rotation -= 2 * PI;
                }
                while (rotation < 0) {
                    rotation += 2 * PI;
                }
            }
            this->lastMouseX = xPos;
        }
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods) {
	bool eventHandled(false);

    if (actions == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        if (!ImGui::IsMouseHoveringAnyWindow()) {
            // The user clicked in the window.  If it's the left
            // mouse button, initiate a rotation.
            this->trackDragging = true;
            this->lastMouseX = -1; // A invalid value, used for initialization in mouseMoveEvent
        }
    }
    if (actions == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
        if (!ImGui::IsMouseHoveringAnyWindow()) {
            // The user clicked in the window.  If it's the left
            // mouse button, initiate a rotation.
            this->trackDragging = false;

        }
    }

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet) {
	bool eventHandled(false);

    if (yOffSet != 0) {
        // Zoom in or out.
        // We only set scroll based on yOffSet
        // Moving Up means enlarge, down means shrink (on the lab mouse)
        this->zoom += (yOffSet * 0.5); // half offset to slow down zoom
        this->zoom = glm::clamp(zoom, 0.5f, 5.0f);

        eventHandled = true;
    }


	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if( action == GLFW_PRESS ) {
		// Respond to some key events.
        if (key == GLFW_KEY_SPACE) {
            // Only set the colour if it is a new cell
            if (this->cubes[this->active_cell].height == 0) {
                this->cubes[this->active_cell].colour = this->current_col;
            }

            this->cubes[this->active_cell].height ++;
            eventHandled = true;
        }

        if (key == GLFW_KEY_BACKSPACE) {
            if (this->cubes[this->active_cell].height > 0) {
                this->cubes[this->active_cell].height --;
            }
            eventHandled = true;
        }

        // Moving active cell
        int shiftPressed = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT);

        if (key == GLFW_KEY_UP) {
            setActiveCell(0, -1, shiftPressed);
            eventHandled = true;
        }

        if (key == GLFW_KEY_DOWN) {
            setActiveCell(0, 1, shiftPressed);
            eventHandled = true;
        }

        if (key == GLFW_KEY_LEFT) {
            setActiveCell(-1, 0, shiftPressed);
            eventHandled = true;
        }

        if (key == GLFW_KEY_RIGHT) {
            setActiveCell(1, 0, shiftPressed);
            eventHandled = true;
        }

        // Border and surfaces
        if (key == GLFW_KEY_B) {
            this->border = !this->border;
            eventHandled = true;
        }

        if (key == GLFW_KEY_S) {
            this->surface = !this->surface;
            eventHandled = true;
        }

        // Controls
        if (key == GLFW_KEY_Q) {
            glfwSetWindowShouldClose(m_window, GL_TRUE);
            eventHandled = true;
        }

        if (key == GLFW_KEY_R) {
            for (int i = 0; i < DIM * DIM; i++) {
                this->cubes[i].height = 0;
                this->cubes[i].colour = 0;
            }
            this->active_cell = 0;
            this->rotation = 0;
            this->zoom = 1.0f;

            this->border = false;
            this->surface = true;

            initColour(false);

            eventHandled = true;
        }
	}

	return eventHandled;
}

void A1::setActiveCell(int deltaX, int deltaY, bool shiftPressed) {
    int old_col = this->active_cell;
    int col = old_col / DIM + deltaX;
    int row = old_col % DIM + deltaY;

    if (col < 0) {
        col = 0;
    } else if (col >= DIM) {
        col = DIM - 1;
    }

    if (row < 0) {
        row = 0;
    } else if (row >= DIM) {
        row = DIM - 1;
    }

    int new_col = col * DIM + row;
    if (old_col != new_col) {
        if (shiftPressed) {
            this->cubes[new_col].height = this->cubes[old_col].height;
            this->cubes[new_col].colour = this->cubes[old_col].colour;
        }
        this->active_cell = new_col;
    }

    // Only set the colour if it is a new cell
    if (this->cubes[this->active_cell].height != 0) {
        this->current_col = this->cubes[this->active_cell].colour;
    }
}
