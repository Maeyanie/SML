#ifndef MESH_H
#define MESH_H

#include <string.h>
#include <math.h>
#include <list>
#include <vector>
#include <unordered_map>

struct Mesh;

#include "stl.h"

class Vertex {
public:
	union {
		struct {
			float x, y, z;
		};
		float c[3];
		uint32_t i[3];
	};
		
	
	Vertex() {}
	Vertex(const Vertex& rhs) {
		memcpy(c, rhs.c, sizeof(float) * 3);
	}
	Vertex(float X, float Y, float Z) {
		x = X;
		y = Y;
		z = Z;
	}
	Vertex(STLTri& tri, int offset) {
		x = tri.v[offset][0];
		y = tri.v[offset][1];
		z = tri.v[offset][2];
	}
	
	inline void operator=(const Vertex& rhs) {
		memcpy(c, rhs.c, sizeof(float) * 3);
	}
	inline bool operator==(const Vertex& rhs) const {
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
	
	bool read(FILE* fp) {
		return fread(c, 4, 3, fp) == 3;
	}
	bool write(FILE* fp) {
		return fwrite(c, 4, 3, fp) == 3;
	}
	
	
	inline float distanceTo(const Vertex& rhs) const {
		return fabs(x - rhs.x) + fabs(y - rhs.y) + fabs(z - rhs.z);
	}
	inline bool closeTo(const Vertex* rhs) const {
		bool c[3];
		c[0] = (i[0] & 0xFFFFFFF0) == (rhs->i[0] & 0xFFFFFFF0);
		c[1] = (i[1] & 0xFFFFFFF0) == (rhs->i[1] & 0xFFFFFFF0);
		c[2] = (i[2] & 0xFFFFFFF0) == (rhs->i[2] & 0xFFFFFFF0);
		
		/*bool cx = fabs(x - rhs->x) < 0.0001;
		bool cy = fabs(y - rhs->y) < 0.0001;
		bool cz = fabs(z - rhs->z) < 0.0001;*/
		return c[0] && c[1] && c[2];
	}
};


class VertexPtrHash {
public:
	size_t operator()(const std::shared_ptr<Vertex> v) const {
		return std::hash<float>()(v->x)
			^ std::hash<float>()(v->y)
			^ std::hash<float>()(v->z);
	}
};
class VertexPtrEquals {
public:
	bool operator()(const std::shared_ptr<Vertex> lhs, const std::shared_ptr<Vertex> rhs) const {
		return *lhs == *rhs;
	}
};





class Triangle {
public:
	union {
		struct {
			uint32_t a, b, c;
		};
		uint32_t v[3];
	};

	Triangle() {}
	Triangle(const Triangle& rhs) {
		memcpy(v, rhs.v, 12);
	}
	Triangle(Mesh* mesh, STLTri tri);
	Triangle(uint32_t* verts) {
		memcpy(v, verts, 12);
	}
	Triangle(uint32_t A, uint32_t B, uint32_t C) {
		a = A;
		b = B;
		c = C;
	}

	bool read(FILE* fp) {
		return fread(v, 4, 3, fp) == 3;
	}
	bool write(FILE* fp) {
		return fwrite(v, 4, 3, fp) == 3;
	}
};
class Quad {
public:
	union {
		struct {
			uint32_t a, b, c, d;
		};
		uint32_t v[4];
	};

	Quad() {}
	Quad(const Quad& rhs) {
		memcpy(v, rhs.v, 16);
	}
	
	int sides() const { return 4; }
	bool read(FILE* fp) {
		return fread(v, 4, 4, fp) == 4;
	}
	bool write(FILE* fp) {
		return fwrite(v, 4, 4, fp) == 4;
	}
};



class Mesh;

class SpatialMap {
protected:
	Mesh* mesh;
	std::vector<uint32_t>* map;
	uint32_t mapSize;

public:
	SpatialMap() {
		mesh = NULL;
		mapSize = 0;
		map = NULL;
	}
	SpatialMap(Mesh* m, uint32_t size = 128) {
		mesh = m;
		mapSize = size;
		map = new std::vector<uint32_t>[size*size*size];
	}
	~SpatialMap() {
		if (map) delete[] map;
	}
	
	void init(Mesh* m, uint32_t size = 128) {
		mesh = m;
		mapSize = size;
		if (map) delete[] map;
		map = new std::vector<uint32_t>[size*size*size];		
	}
	
	
	void build();
	size_t getPos(Vertex& v);
	std::vector<uint32_t>* get(size_t pos) {
		return &(map[pos]);
	}
	std::vector<uint32_t>* get(Vertex& v) {
		return &(map[getPos(v)]);
	}
	std::list<std::vector<uint32_t>*> getNear(Vertex& v);
};

class Mesh {
public:
	float minX, maxX, minY, maxY, minZ, maxZ;
	
	std::vector<std::shared_ptr<Vertex>> v;
	std::vector<std::shared_ptr<Triangle>> t;
	std::vector<std::shared_ptr<Quad>> q;
	
	SpatialMap spatialMap;
	std::unordered_map<std::shared_ptr<Vertex>,size_t,VertexPtrHash,VertexPtrEquals> lookup;
	
	Mesh() {
		minX = minY = minZ = std::numeric_limits<float>::max();
		maxX = maxY = maxZ = -std::numeric_limits<float>::max();
		spatialMap.init(this);
	}
	inline void add(std::shared_ptr<Vertex> V) {
		if (V->x < minX) minX = V->x;
		if (V->x > maxX) maxX = V->x;
		if (V->y < minY) minY = V->y;
		if (V->y > maxY) maxY = V->y;
		if (V->z < minZ) minZ = V->z;
		if (V->z > maxZ) maxZ = V->z;
		v.push_back(V);
	}
	inline void add(std::shared_ptr<Triangle> T) {
		t.push_back(T);
	}
	inline void add(std::shared_ptr<Quad> Q) {
		q.push_back(Q);
	}
};


#endif