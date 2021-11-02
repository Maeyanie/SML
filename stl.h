#ifndef STL_H
#define STL_H

#include <filesystem>

struct __attribute__((packed)) STLTri {
	struct {
		float n[3];
		float v[3][3];
	};
	uint16_t abc;
};

Mesh* readSTL(std::filesystem::path file);
void writeSTL(std::filesystem::path file, Mesh* mesh);

#endif