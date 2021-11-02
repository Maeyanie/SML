#ifndef SML_H
#define SML_H

#include <filesystem>

extern "C" {
	uint32_t crc32c_sw(uint32_t crc, void const *buf, size_t len);
	uint32_t crc32c_hw(uint32_t crc, void const *buf, size_t len);
	uint32_t crc32c(uint32_t crc, void const *buf, size_t len);
};

Mesh* readSML(std::filesystem::path file);
void writeSML(std::filesystem::path file, Mesh* mesh);

#endif