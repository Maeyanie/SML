#ifndef OBJ_H
#define OBJ_H

#include <filesystem>

Mesh* readOBJ(std::filesystem::path file);
void writeOBJ(std::filesystem::path file, Mesh* mesh);

#endif