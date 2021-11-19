#include "mesh.h"
using namespace std;

static inline uint32_t findVertex(Mesh* mesh, shared_ptr<Vertex> v) {
	auto i = mesh->lookup.find(v);
	if (i != mesh->lookup.end()) {
		return i->second;
	}
	
	/* This is very slow and didn't seem to appreciably help anyhow.
	for (size_t index = 0; index < mesh->v.size(); index++) {
		if (v->closeTo(mesh->v[index].get())) {
			mesh->lookup.emplace(v, index);
			return index;
		}
	}
	*/

	uint32_t ret = mesh->v.size();
	mesh->add(v);
	#ifdef USE_SPARSEHASH
	mesh->lookup.insert({v, ret});
	#else
	mesh->lookup.emplace(v, ret);
	#endif
	return ret;
}

Triangle::Triangle(Mesh* mesh, STLTri tri) {
	shared_ptr<Vertex> v = make_shared<Vertex>(tri, 0);
	a = findVertex(mesh, v);

	v = make_shared<Vertex>(tri, 1);
	b = findVertex(mesh, v);

	v = make_shared<Vertex>(tri, 2);
	c = findVertex(mesh, v);
}



static inline float unlerp(float min, float max, float val) {
	return (val - min) / (max - min);
}

size_t SpatialMap::getPos(Vertex& v) {
	float x = unlerp(mesh->minX, mesh->maxX, v.x);
	float y = unlerp(mesh->minY, mesh->maxY, v.y);
	float z = unlerp(mesh->minZ, mesh->maxZ, v.z);
	
	uint32_t ix = round((mapSize-1) * x);
	uint32_t iy = round((mapSize-1) * y);
	uint32_t iz = round((mapSize-1) * z);
	
	return ix + (mapSize*iy) + (mapSize*mapSize*iz);
}

list<vector<uint32_t>*> SpatialMap::getNear(Vertex& v) {
	float x = unlerp(mesh->minX, mesh->maxX, v.x);
	float y = unlerp(mesh->minY, mesh->maxY, v.y);
	float z = unlerp(mesh->minZ, mesh->maxZ, v.z);
	
	uint32_t ix = round((mapSize-1) * x);
	uint32_t iy = round((mapSize-1) * y);
	uint32_t iz = round((mapSize-1) * z);

	// Only doing 6 checks, should be good enough almost all the time and faster than 26 checks.
	list<vector<uint32_t>*> ret;
	vector<uint32_t>* add = &map[ix + (mapSize*iy) + (mapSize*mapSize*iz)];
	ret.push_back(add);
	if (ix > 0) {
		add = &map[(ix-1) + (mapSize*iy) + (mapSize*mapSize*iz)];
		ret.push_back(add);
	}
	if (ix < mapSize-1) {
		add = &map[(ix+1) + (mapSize*iy) + (mapSize*mapSize*iz)];
		ret.push_back(add);
	}
	
	if (iy > 0) {
		add = &map[ix + (mapSize*(iy-1)) + (mapSize*mapSize*iz)];
		ret.push_back(add);
	}
	if (iy < mapSize-1) {
		add = &map[ix + (mapSize*(iy+1)) + (mapSize*mapSize*iz)];
		ret.push_back(add);
	}
	
	if (iz > 0) {
		add = &map[ix + (mapSize*iy) + (mapSize*mapSize*(iz-1))];
		ret.push_back(add);
	}
	if (iz < mapSize-1) {
		add = &map[ix + (mapSize*iy) + (mapSize*mapSize*(iz+1))];
		ret.push_back(add);
	}
	
	return ret;
}

void SpatialMap::build() {
	uint32_t i = 0;
	for (auto& t : mesh->t) {
		for (uint32_t iv : t->v) {
			size_t pos = getPos(*mesh->v[iv]);
			map[pos].push_back(i);
		}
		i++;
	}
}


