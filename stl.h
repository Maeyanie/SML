#ifndef STL_H
#define STL_H

#include "config.h"

#ifdef _MSC_VER
#pragma pack(push, 1)
struct STLTri {
#else
struct __attribute__((packed)) STLTri {
#endif
	struct {
		float n[3];
		float v[3][3];
	};
	uint16_t abc;
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif

Mesh* readSTL(std::filesystem::path file);
void writeSTL(std::filesystem::path file, Mesh* mesh);

#endif