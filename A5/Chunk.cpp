#include "Chunk.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Chunk::Chunk(ShaderProgram* m_shader, glm::vec3 position) :
    m_shader(m_shader),
    m_position(position) {

    numVertices = 0;
    requireUpdate = true;

    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_SIZE; k++) {
                blocks[i][j][k] = BlockType::EMPTY;
            }
        }
    }

    M_uni = m_shader->getUniformLocation("M");
}

Chunk::~Chunk() {
    deleteGraphicsMemory();
}

void Chunk::deleteGraphicsMemory() {
    if (numVertices > 0) {
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
    }
}

BlockType Chunk::getBlock(int x, int y, int z) {
    return blocks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    blocks[x][y][z] = type;
    requireUpdate = true;
}

void Chunk::render() {
    if (requireUpdate) {
        updateBlock();
    }

    if (numVertices == 0) {
        return;
    }

    glm::mat4 M = glm::translate(glm::mat4(), m_position);
    glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( M ) );

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
}

void setCubeVertex(float* verts ,int& i, float x, float y, float z, int type) {
    verts[i++] = x;
    verts[i++] = y;
    verts[i++] = z;
    verts[i++] = type;
}

void Chunk::updateBlock() {
    requireUpdate = false;
    // remove previous allocated memory if there is any
    deleteGraphicsMemory();

    size_t vertex_size = 4; // each vertex takes 4 floats
    size_t vertex_per_cube = 6 * 6;
    size_t numCubes = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;
    size_t verts_sz = vertex_size * vertex_per_cube * numCubes;
    float * verts = new float [verts_sz];

    numVertices = 0;
    int x = 0;
    for (int i = 0; i < CHUNK_SIZE; i++) {
        for (int j = 0; j < CHUNK_SIZE; j++) {
            for (int k = 0; k < CHUNK_SIZE; k++) {
                BlockType blocktype = blocks[i][j][k];
                if (blocktype == BlockType::EMPTY) {
                    continue;
                }

                // Front
                setCubeVertex(verts, x, i, j, k, blocktype);
                setCubeVertex(verts, x, i + 1, j, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k, blocktype);
                setCubeVertex(verts, x, i, j, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k, blocktype);
                setCubeVertex(verts, x, i, j + 1, k, blocktype);

                // Back
                setCubeVertex(verts, x, i + 1, j, k + 1, blocktype);
                setCubeVertex(verts, x, i, j, k + 1, blocktype);
                setCubeVertex(verts, x, i, j + 1, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j, k + 1, blocktype);
                setCubeVertex(verts, x, i, j + 1, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k + 1, blocktype);

                // Left
                setCubeVertex(verts, x, i, j, k + 1, blocktype);
                setCubeVertex(verts, x, i, j, k, blocktype);
                setCubeVertex(verts, x, i, j + 1, k, blocktype);
                setCubeVertex(verts, x, i, j, k + 1, blocktype);
                setCubeVertex(verts, x, i, j + 1, k, blocktype);
                setCubeVertex(verts, x, i, j + 1, k + 1, blocktype);

                // Right
                setCubeVertex(verts, x, i + 1, j, k, blocktype);
                setCubeVertex(verts, x, i + 1, j, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1 , k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k, blocktype);

                // Top
                setCubeVertex(verts, x, i, j + 1, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k + 1, blocktype);
                setCubeVertex(verts, x, i, j + 1, k, blocktype);
                setCubeVertex(verts, x, i + 1, j + 1, k + 1, blocktype);
                setCubeVertex(verts, x, i, j + 1, k + 1, blocktype);

                // Bottom
                setCubeVertex(verts, x, i, j, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j, k, blocktype);
                setCubeVertex(verts, x, i, j, k + 1, blocktype);
                setCubeVertex(verts, x, i + 1, j, k, blocktype);
                setCubeVertex(verts, x, i, j, k, blocktype);
            }
        }
    }
    numVertices = x / vertex_size;

    if (numVertices == 0) {
        delete [] verts;
        return;
    } else {
        glGenVertexArrays( 1, &m_vao );
        glBindVertexArray( m_vao );

        // Create the cube vertex buffer
        glGenBuffers( 1, &m_vbo );
        glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
        glBufferData( GL_ARRAY_BUFFER, numVertices * vertex_size * sizeof(float), verts, GL_STATIC_DRAW );

        GLint posAttrib = m_shader->getAttribLocation( "position" );
        glEnableVertexAttribArray( posAttrib );
        glVertexAttribPointer( posAttrib, 4, GL_FLOAT, GL_FALSE, 0, nullptr );

        delete [] verts;
        CHECK_GL_ERRORS;
    }
}

glm::vec3 Chunk::getPosition() {
    return m_position;
}
