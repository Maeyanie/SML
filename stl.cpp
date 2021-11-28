#include <string.h>
#include <assert.h>
#include <vector>
#include "mesh.h"
using namespace std;

Mesh* readSTL(filesystem::path file) {
	#ifdef _WIN32
		FILE* fp = _wfopen(file.c_str(), L"rb");
		if (!fp) {
			fprintf(stderr, "Could not open STL file '%ls' for reading: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		FILE* fp = fopen(file.c_str(), "r");
		if (!fp) {
			fprintf(stderr, "Could not open STL file '%s' for reading: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	fseek(fp, 80, SEEK_SET);
	
	uint32_t triCount;
	fread(&triCount, 4, 1, fp);
	if (84 + (triCount*50) != filesystem::file_size(file)) {
		fprintf(stderr, "File '%s' size did not match triangle count %u.\n", file.c_str(), triCount);
		exit(__LINE__);
	}
	
	Mesh* mesh = new Mesh();
	
	printf("Reading %u triangles...\n", triCount);
	mesh->v.reserve(triCount*3);
	mesh->t.reserve(triCount);
	
	STLTri stltri;
	time_t nextupdate = time(NULL) + 1;
	for (uint32_t t = 0; t < triCount; t++) {
		if (time(NULL) >= nextupdate) {
			printf("%u", t);
			fflush(stdout);
			printf("\r");
			nextupdate++;
		}
		size_t rc = fread(&stltri, 50, 1, fp);
		if (rc != 1) {
			fprintf(stderr, "Error reading %s: %m\n", file.c_str());
			exit(__LINE__);
		}
		shared_ptr<Triangle> tri = make_shared<Triangle>(mesh, stltri);
		mesh->t.push_back(tri);
	}
	mesh->lookup.clear();
	printf("\nLoaded %u vertices and %u triangles.\n", (uint32_t)mesh->v.size(), (uint32_t)mesh->t.size());
	
	fclose(fp);
	return mesh;
}

void writeSTL(filesystem::path file, Mesh* mesh) {
	#ifdef _WIN32
		printf("Writing %ls...", file.c_str());
		fflush(stdout);
		FILE* fp = _wfopen(file.c_str(), L"wb+");
		if (!fp) {
			fprintf(stderr, "Could not open STL file '%ls' for writing: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		printf("Writing %s...", file.c_str());
		fflush(stdout);
		FILE* fp = fopen(file.c_str(), "w+");
		if (!fp) {
			fprintf(stderr, "Could not open STL file '%s' for writing: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	char header[80];
	memset(header, 0, 80);
	size_t ret = fwrite(header, 1, 80, fp);
	assert(ret == 80);
	
	uint32_t triCount = mesh->t.size();
	fwrite(&triCount, 4, 1, fp);
	
	STLTri stltri;
	memset(&stltri, 0, sizeof(stltri));
	
	for (auto& i : mesh->t) {
		shared_ptr<Vertex> v = mesh->v[i->a];
		stltri.v[0][0] = v->x;
		stltri.v[0][1] = v->y;
		stltri.v[0][2] = v->z;

		v = mesh->v[i->b];
		stltri.v[1][0] = v->x;
		stltri.v[1][1] = v->y;
		stltri.v[1][2] = v->z;

		v = mesh->v[i->c];
		stltri.v[2][0] = v->x;
		stltri.v[2][1] = v->y;
		stltri.v[2][2] = v->z;
		
		ret = fwrite(&stltri, sizeof(stltri), 1, fp);
		assert(ret == 1);
	}
	
	fclose(fp);
	printf("Done.\n");
}

