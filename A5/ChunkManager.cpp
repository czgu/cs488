#include "ChunkManager.hpp"
#include <cstdlib>
#include <iostream>

ChunkManager::ChunkManager(ShaderProgram* m_shader) : m_shader(m_shader) {
    m_player_position = glm::vec3(999, 999, 999);

    chunks = new std::vector<Chunk*>();
}

ChunkManager::~ChunkManager() {
    for (Chunk* chunk : *chunks) {
        delete chunk;
    }
    for (Chunk* chunk : unloadList) {
        delete chunk;
    }

    // Since its a pointer of vectors
    delete chunks;
}

void ChunkManager::render() {
    for (Chunk* chunk : *chunks) {
        chunk->render();
    }
}

glm::vec3 toChunkCoord(glm::vec3 v) {
    return glm::vec3(
        ((int)v.x) / CHUNK_SIZE,
        ((int)v.y) / CHUNK_SIZE,
        ((int)v.z) / CHUNK_SIZE
    );
}

glm::vec3 toNormalCoord(glm::vec3 v) {
    return glm::vec3(
        ((int)v.x) * CHUNK_SIZE,
        ((int)v.y) * CHUNK_SIZE,
        ((int)v.z) * CHUNK_SIZE
    );
}

void ChunkManager::update(glm::vec3& player_position) {
    glm::vec3 chunk_player_position = toChunkCoord(player_position);

    if (chunk_player_position != m_player_position) {
        m_player_position = chunk_player_position;
        updatePlayerPosition();
    }

    updateLoadList();
    updateUnloadList();

}

void ChunkManager::updatePlayerPosition() {
    bool chunkLoaded[CHUNK_LIST_X][CHUNK_LIST_Y][CHUNK_LIST_Z];
    for (int x = 0; x < CHUNK_LIST_X; x++) {
        for (int y = 0; y < CHUNK_LIST_Y; y++) {
            for (int z = 0; z < CHUNK_LIST_Z; z++) {
                chunkLoaded[x][y][z] = false;
            }
        }
    }

    glm::vec3 origin = m_player_position -
        glm::vec3(CHUNK_LIST_X / 2, CHUNK_LIST_Y / 2, CHUNK_LIST_Z / 2);

    std::vector<Chunk*>* remaining_chunks = new std::vector<Chunk*>();
    for (Chunk* chunk : *chunks) {
        glm::vec3 chunkPos = toChunkCoord(chunk->getPosition()) - origin;
        if (chunkPos.x < 0 ||
            chunkPos.y < 0 ||
            chunkPos.z < 0 ||
            chunkPos.x >= CHUNK_LIST_X ||
            chunkPos.y >= CHUNK_LIST_Y ||
            chunkPos.z >= CHUNK_LIST_Z) {
            unloadList.push_back(chunk);
        } else {
            chunkLoaded[(int)chunkPos.x][(int)chunkPos.y][(int)chunkPos.z] = true;
            remaining_chunks->push_back(chunk);
        }
    }

    for (int x = 0; x < CHUNK_LIST_X; x++) {
        for (int y = 0; y < CHUNK_LIST_Y; y++) {
            for (int z = 0; z < CHUNK_LIST_Z; z++) {
                if (chunkLoaded[x][y][z] == false) {
                    loadList.push_back(origin + glm::vec3(x, y, z));
                }
            }
        }
    }

    // Clean up
    delete chunks;
    chunks = remaining_chunks;
}

void ChunkManager::updateLoadList() {
    /*
    if (loadList.empty()) {
        return;
    }

    glm::vec3 coord = loadList[0];
    loadList.erase(loadList.begin());
    */
    for (glm::vec3& coord : loadList) {
        Chunk* chunk = new Chunk(m_shader, toNormalCoord(coord));

        if (coord.y == 0) {
            for (int i = 0; i < CHUNK_SIZE; i++) {
                for (int j = 0; j < rand() % 10; j++) {
                    for (int k = 0; k <  CHUNK_SIZE; k++) {
                        chunk->setBlock(i, j, k, (BlockType)(rand() % BlockType::NUM_BLOCKS));
                    }
                }
            }
        }

        chunks->push_back(chunk);
    }
    loadList.clear();
}

void ChunkManager::updateUnloadList() {
    /*
    if (unloadList.empty()) {
        return;
    }

    Chunk* chunk = unloadList[0];
    unloadList.erase(unloadList.begin());
    */
    for (Chunk* chunk : unloadList) {
        delete chunk;
    }
    unloadList.clear();


}
