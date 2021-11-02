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
	assert(84 + (triCount*50) == filesystem::file_size(file));
	
	Mesh* mesh = new Mesh();
	
	printf("Reading %u triangles...", triCount);
	fflush(stdout);
	STLTri stltri;
	for (uint32_t t = 0; t < triCount; t++) {
		fread(&stltri, 50, 1, fp);
		Triangle* tri = new Triangle(mesh, stltri);
		mesh->t.push_back(tri);
	}
	mesh->lookup.clear();
	printf("Done.\n");
	
	fclose(fp);
	return mesh;
}

void writeSTL(filesystem::path file, Mesh* mesh) {
	#ifdef _WIN32
		FILE* fp = _wfopen(file.c_str(), L"wb+");
		if (!fp) {
			fprintf(stderr, "Could not open STL file '%ls' for writing: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
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
	
	vector<Vertex*> verts(mesh->v.begin(), mesh->v.end());
	
	STLTri stltri;
	memset(&stltri, 0, sizeof(stltri));
	
	for (auto& i : mesh->t) {
		Vertex* v = verts[i->a];
		stltri.v[0][0] = v->fx();
		stltri.v[0][1] = v->fy();
		stltri.v[0][2] = v->fz();

		v = verts[i->b];
		stltri.v[1][0] = v->fx();
		stltri.v[1][1] = v->fy();
		stltri.v[1][2] = v->fz();

		v = verts[i->c];
		stltri.v[2][0] = v->fx();
		stltri.v[2][1] = v->fy();
		stltri.v[2][2] = v->fz();
		
		ret = fwrite(&stltri, sizeof(stltri), 1, fp);
		assert(ret == 1);
	}
	
	fclose(fp);
}

