#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <filesystem>
#include "mesh.h"
#include "sml.h"
using namespace std;

Mesh* readSML(std::filesystem::path file) {
	Mesh* mesh = new Mesh();
	
	#ifdef _WIN32
		FILE* fp = _wfopen(file.c_str(), L"rb");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%ls' for reading: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		FILE* fp = fopen(file.c_str(), "r");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%s' for reading: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	char magic[4];
	size_t ret = fread(magic, 1, 4, fp);
	assert(ret == 4);
	assert(!memcmp(magic, "SML1", 4));
	
	uint32_t crc;
	ret = fread(&crc, 4, 1, fp);
	assert(ret == 1);
	
	// Check CRC
	{
		printf("Computing CRC32C...");
		fflush(stdout);
		size_t len;
		char* buffer = (char*)malloc(1*1024*1024);
		uint32_t crc2 = 0;
		while ((len = fread(buffer, 1, (1*1024*1024), fp)) > 0) {
			crc2 = crc32c(crc2, buffer, len);
		}
		free(buffer);
		printf("read %08x, calculated %08x\n", crc, crc2);
		assert(crc == crc2);
		
		fseek(fp, 8, SEEK_SET);
	}


	struct __attribute__((packed)) {
		uint8_t type;
		uint32_t length;
	} header;
	while (fread(&header, 5, 1, fp) == 1) {
		printf("Reading segment, type %hhu, length %u...", header.type, header.length);
		fflush(stdout);
		
		switch (header.type) {
			case 0: {
				char* buffer = (char*)malloc(header.length+1);
				fread(buffer, 1, header.length, fp);
				mesh->comments.emplace_back(buffer);
				free(buffer);
			} break;
			
			case 1: { // Vertex float list
				int verts = header.length / 12;
				for (int i = 0; i < verts; i++) {
					shared_ptr<Vertex> v = make_shared<Vertex>();
					v->read(fp);
					mesh->v.push_back(v);
				}
			} break;
			
			case 2: { // Vertex double list; here we're just dumping the extra precision and converting to floats.
				int verts = header.length / 24;
				for (int i = 0; i < verts; i++) {
					shared_ptr<Vertex> v = make_shared<Vertex>();
					v->read(fp);
					mesh->v.push_back(v);
				}				
			} break;
			
			case 3: { // Triangle list
				int tris = header.length / 12;
				for (int i = 0; i < tris; i++) {
					shared_ptr<Triangle> t = make_shared<Triangle>();
					t->read(fp);
					mesh->t.push_back(t);
				}
			} break;
			
			case 4: { // Quad list
				int quads = header.length / 16;
				for (int i = 0; i < quads; i++) {
					shared_ptr<Quad> q = make_shared<Quad>();
					q->read(fp);
					mesh->q.push_back(q);
				}
			} break;
			
			case 5: { // Triangle strip
				int points = header.length / 4;
				assert(points >= 3);
				
				uint32_t verts[3];
				
				fread(verts, 4, 3, fp);
				mesh->t.push_back(make_shared<Triangle>(verts));
				//printf("\nStrip: %u,%u,%u\n", verts[0], verts[1], verts[2]);
				
				for (int i = 3; i < points; i++) {
					/* i=0  0 1 2
					   i=3  0 2 3	2->1
					   i=4  3 2 4	2->0 */
					verts[i & 1] = verts[2];
					ret = fread(&verts[2], 4, 1, fp);
					assert(ret == 1);
					
					//printf("\t%u,%u,%u\n", verts[0], verts[1], verts[2]);
					mesh->t.push_back(make_shared<Triangle>(verts));
				}
			} break;
			
			default:
				fprintf(stderr, "Error: Unrecognized segment type '%hhu'\n", header.type);
				exit(__LINE__);
		}
		printf("Done.\n");
	}
	return mesh;
}



void writeSML(filesystem::path file, Mesh* mesh, uint32_t flags) {
	if (flags & SMLFlags::STRIP_MAP) {
		printf("Building spatial map...");
		fflush(stdout);
		mesh->spatialMap.build();
		printf("Done.\n");
	}
	
	#ifdef _WIN32
		printf("Writing to %ls...\n", file.c_str());

		FILE* fp = _wfopen(file.c_str(), L"wb+");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%ls' for writing: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		printf("Writing to %s...\n", file.c_str());

		FILE* fp = fopen(file.c_str(), "w+");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%s' for writing: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	fwrite("SML1", 4, 1, fp); // Identifying header
	// Skip CRC until the end.
	fseek(fp, 8, SEEK_SET);
	
	uint8_t type;
	
	if (!mesh->comments.empty()) {
		printf("Writing %u commment%s...", (uint32_t)mesh->comments.size(), mesh->comments.size() == 1 ? "" : "s");
		fflush(stdout);
		type = 0;
		for (auto& str : mesh->comments) {
			fwrite(&type, 1, 1, fp);
			uint32_t len = str.length()+1;
			fwrite(&len, 4, 1, fp);
			fwrite(str.c_str(), str.length()+1, 1, fp);
		}
		printf("Done.\n");
	}

	
	if (!mesh->v.empty()) { // Hey, you never know...
		size_t vertCount = mesh->v.size();
		assert(vertCount <= 357913941);
		printf("Writing %u vertices...", (uint32_t)vertCount);
		fflush(stdout);
		type = 1;
		fwrite(&type, 1, 1, fp);
		uint32_t vertLength = vertCount * 12;
		fwrite(&vertLength, 4, 1, fp);
		for (auto& i : mesh->v) {
			i->write(fp);
		}
		printf("Done.\n");
	}
	
	
	if (!mesh->t.empty()) {
		if (flags & SMLFlags::STRIP) {
			list<Triangle*> singles;
			list<list<Triangle*>> strips;
			
			if (flags & SMLFlags::STRIP_MAP) {
				stripsearch_map(mesh, singles, strips);
			} else if (flags & SMLFlags::STRIP_NEXT) {
				stripsearch_next(mesh, singles, strips);
			} else if (flags & SMLFlags::STRIP_EXHAUSTIVE) {
				stripsearch_exhaustive(mesh, singles, strips);
			}
			
			for (auto& strip : strips) {
				type = 5;
				fwrite(&type, 1, 1, fp);
				
				size_t striplen = strip.size();
				assert(striplen <= 1073741821);
				uint32_t stripLength = (striplen+2) * sizeof(uint32_t);
				fwrite(&stripLength, 4, 1, fp);

				Triangle* first = strip.front();
				strip.pop_front();
				first->write(fp);
				
				for (auto& i : strip) {
					fwrite(&(i->c), 4, 1, fp);
				}
			}
			
			size_t triCount = singles.size();
			assert(triCount <= 357913941);
			if (triCount > 0) {
				printf("Writing %u lone triangles...", (uint32_t)triCount);
				fflush(stdout);
				type = 3;
				fwrite(&type, 1, 1, fp);
				uint32_t triLength = triCount * 12;
				fwrite(&triLength, 4, 1, fp);
				for (auto& i : singles) {
					i->write(fp);
				}
				printf("Done.\n");
			}
		} else {
			type = 3;
			fwrite(&type, 1, 1, fp);
			
			size_t triCount = mesh->t.size();
			assert(triCount <= 357913941);
			printf("Writing %u triangles...", (uint32_t)triCount);
			fflush(stdout);
			uint32_t triLength = triCount * 12;
			fwrite(&triLength, 4, 1, fp);
			for (auto& i : mesh->t) {
				i->write(fp);
			}
			printf("Done.\n");
		}
	}
	
	
	if (!mesh->q.empty()) {
		type = 4;
		fwrite(&type, 1, 1, fp);
		
		size_t quadCount = mesh->q.size();
		assert(quadCount <= 268435455);
		printf("Writing %u quads...", (uint32_t)quadCount);
		fflush(stdout);
		uint32_t quadLength = quadCount * 16;
		fwrite(&quadLength, 4, 1, fp);
		for (auto& i : mesh->q) {
			i->write(fp);
		}
		printf("Done.\n");
	}
	
	
	// Write CRC
	printf("Computing CRC32C...");
	fflush(stdout);
	fseek(fp, 8, SEEK_SET);
	size_t len;
	char* buffer = (char*)malloc(1*1024*1024);
	uint32_t crc = 0;
	while ((len = fread(buffer, 1, (1*1024*1024), fp)) > 0) {
		crc = crc32c(crc, buffer, len);
	}
	free(buffer);
	fseek(fp, 4, SEEK_SET);
	fwrite(&crc, 4, 1, fp);
	printf("%08x\n", crc);
	fclose(fp);
	
	printf("File written.\n");
}