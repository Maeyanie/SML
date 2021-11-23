#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <filesystem>
#include "mesh.h"
#include "obj.h"
using namespace std;

Mesh* readOBJ(std::filesystem::path file) {
	Mesh* mesh = new Mesh();
	
	#ifdef _WIN32
		printf("Reading from %ls...", file.c_str());
		fflush(stdout);
		
		FILE* fp = _wfopen(file.c_str(), L"rt");
		if (!fp) {
			fprintf(stderr, "Could not open OBJ file '%ls' for reading: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}

		char* line = (char*)malloc(4096);
		while (fgets(line, 4096, fp) != NULL)
	#else
		printf("Reading from %s...", file.c_str());
		fflush(stdout);
	
		FILE* fp = fopen(file.c_str(), "rt");
		if (!fp) {
			fprintf(stderr, "Could not open OBJ file '%s' for reading: %m\n", file.c_str());
			exit(__LINE__);
		}
		
		char* line = NULL;
		size_t linelen = 0;
		while (getline(&line, &linelen, fp) != -1)
	#endif
	{
		if (line[0] == 0 || line[0] == '#') continue;
		
		char* type = strtok(line, " \r\n");
		if (type == NULL) continue;
		if (!strcmp(type, "v")) {
			char* x = strtok(NULL, " \r\n");
			char* y = strtok(NULL, " \r\n");
			char* z = strtok(NULL, " \r\n");
			mesh->add(make_shared<Vertex>(atof(x), atof(y), atof(z)));
		} else if (!strcmp(type, "f")) {
			vector<int> vertices;
			char* c;
			while ((c = strtok(NULL, " \r\n"))) {
				int i = atoi(c);
				if (i <= 0) {
					// TODO: Implement negative vertices.
					fprintf(stderr, "Unsupported OBJ file: Invalid vertex index %d (%s)\n", i, c);
					exit(__LINE__);
				}
				vertices.push_back(i);
			}

			switch (vertices.size()) {
				case 3:
					mesh->t.push_back(make_shared<Triangle>(vertices[0], vertices[1], vertices[2]));
					break;
				
				case 4:
					mesh->t.push_back(make_shared<Triangle>(vertices[0], vertices[1], vertices[2]));
					mesh->t.push_back(make_shared<Triangle>(vertices[0], vertices[2], vertices[3]));
					break;
				
				default:
					fprintf(stderr, "Unsupported OBJ file: Contains a face with %d sides. Currently only 3 or 4 sides are supported.\n", (int)vertices.size());
					exit(__LINE__);
			}
		}
	}
	
	fclose(fp);
	if (line) free(line);
	printf("Done.\n");
	return mesh;
}



void writeOBJ(filesystem::path file, Mesh* mesh) {
	#ifdef _WIN32
		printf("Writing to %ls...\n", file.c_str());

		FILE* fp = _wfopen(file.c_str(), L"wt+");
		if (!fp) {
			fprintf(stderr, "Could not open OBJ file '%ls' for writing: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		printf("Writing to %s...\n", file.c_str());

		FILE* fp = fopen(file.c_str(), "wt+");
		if (!fp) {
			fprintf(stderr, "Could not open OBJ file '%s' for writing: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	for (auto& v : mesh->v) {
		fprintf(fp, "v %f %f %f\n", v->x, v->y, v->z);
	}
	
	for (auto& t : mesh->t) {
		fprintf(fp, "f %u %u %u\n", t->a+1, t->b+1, t->c+1);
	}
	
	for (auto& q : mesh->q) {
		fprintf(fp, "f %u %u %u %u\n", q->a+1, q->b+1, q->c+1, q->d+1);
	}
	
	fclose(fp);
	printf("File written.\n");
}