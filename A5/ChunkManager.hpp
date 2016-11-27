#pragma once

#include "Chunk.hpp"

#include <vector>

#include "cs488-framework/ShaderProgram.hpp"
#include <glm/glm.hpp>

#define CHUNK_LIST_X 15
#define CHUNK_LIST_Y 9
#define CHUNK_LIST_Z 15

class ChunkManager {
public:
    ChunkManager(ShaderProgram* m_shader);
    ~ChunkManager();

    void update(glm::vec3& player_position);
    void render();


private:
    void updatePlayerPosition();
    void updateLoadList();
    void updateUnloadList();

    std::vector<Chunk*>* chunks;
    std::vector<glm::vec3> loadList;
    std::vector<Chunk*> unloadList;

    glm::vec3 m_player_position;

    ShaderProgram* m_shader;
};
