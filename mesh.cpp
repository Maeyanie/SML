#include "mesh.h"
using namespace std;

Triangle::Triangle(Mesh* mesh, STLTri tri) {
	FloatVertex* v = new FloatVertex(tri, 0);
	auto i = mesh->lookup.find(v);
	if (i != mesh->lookup.end()) {
		a = i->second;
	} else {
		a = mesh->lookup.size();
		mesh->v.push_back(v);
		mesh->lookup.emplace(v, a);
	}

	v = new FloatVertex(tri, 1);
	i = mesh->lookup.find(v);
	if (i != mesh->lookup.end()) {
		b = i->second;
	} else {
		b = mesh->lookup.size();
		mesh->v.push_back(v);
		mesh->lookup.emplace(v, b);
	}

	v = new FloatVertex(tri, 2);
	i = mesh->lookup.find(v);
	if (i != mesh->lookup.end()) {
		c = i->second;
	} else {
		c = mesh->lookup.size();
		mesh->v.push_back(v);
		mesh->lookup.emplace(v, c);
	}
}