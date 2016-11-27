#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include <glm/glm.hpp>

#define CHUNK_SIZE 16

enum BlockType {
    EMPTY = 0,
    GRASS,
    DIRT,
    SAND,
    WATER,

    NUM_BLOCKS
};

class Chunk {
public:
    Chunk(ShaderProgram* m_shader, glm::vec3 position);
    ~Chunk();
    BlockType getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, BlockType type);
    void render();
    glm::vec3 getPosition();

private:
    void deleteGraphicsMemory();
    void updateBlock();
    bool requireUpdate;

    // Open Gl Variables
    unsigned int numVertices;
    ShaderProgram* m_shader;
    GLuint m_vbo;
    GLuint m_vao;
    GLint M_uni;
    glm::vec3 m_position;

    BlockType blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};

#endif
