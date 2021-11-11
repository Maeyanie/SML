#ifndef SML_H
#define SML_H

#include <filesystem>
#include "mesh.h"

extern "C" {
	uint32_t crc32c_sw(uint32_t crc, void const *buf, size_t len);
	uint32_t crc32c_hw(uint32_t crc, void const *buf, size_t len);
	uint32_t crc32c(uint32_t crc, void const *buf, size_t len);
};

enum SMLFlags {
	NONE				= 0,
	STRIP_NEXT			= 0b0001,
	STRIP_MAP			= 0b0010,
	STRIP_EXHAUSTIVE	= 0b0100,
	STRIP               = 0b0111
};

Mesh* readSML(std::filesystem::path file);
void writeSML(std::filesystem::path file, Mesh* mesh, uint32_t writeFlags = SMLFlags::NONE);

void stripsearch_map(Mesh* mesh, std::list<Triangle*>& singles, std::list<std::list<Triangle*>>& strips);
void stripsearch_next(Mesh* mesh, std::list<Triangle*>& singles, std::list<std::list<Triangle*>>& strips);
void stripsearch_exhaustive(Mesh* mesh, std::list<Triangle*>& singles, std::list<std::list<Triangle*>>& strips);

#endif