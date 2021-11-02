#ifndef MESH_H
#define MESH_H

#include <string.h>
#include <list>
#include <unordered_map>

struct Mesh;

#include "stl.h"

class Vertex {
public:
	virtual ~Vertex() {};
	
	virtual void operator=(const Vertex& rhs) = 0;
	virtual bool operator==(const Vertex& rhs) const = 0;
	
	virtual double dx() const { return 0.0; }
	virtual double dy() const { return 0.0; }
	virtual double dz() const { return 0.0; }
	
	virtual float fx() const { return 0.0; }
	virtual float fy() const { return 0.0; }
	virtual float fz() const { return 0.0; }
	
	virtual bool read(FILE* fp) = 0;
	virtual bool write(FILE* fp) = 0;
};

class FloatVertex : public Vertex {
private:
	union {
		struct {
			float x, y, z;
		};
		float c[3];
	};
		
	
public:
	FloatVertex() {}
	FloatVertex(const FloatVertex& rhs) {
		memcpy(c, rhs.c, sizeof(float) * 3);

	}
	FloatVertex(const Vertex& rhs) {
		x = rhs.fx();
		y = rhs.fy();
		z = rhs.fz();
	}
	FloatVertex(STLTri& tri, int offset) {
		x = tri.v[offset][0];
		y = tri.v[offset][1];
		z = tri.v[offset][2];
	}
	virtual ~FloatVertex() {}
	
	virtual void operator=(const Vertex& rhs) {
		x = rhs.fx();
		y = rhs.fy();
		z = rhs.fz();
	}
	virtual bool operator==(const Vertex& rhs) const {
		return x == rhs.fx() && y == rhs.fy() && z == rhs.fz();
	}
	
	virtual bool read(FILE* fp) {
		return fread(c, 4, 3, fp) == 3;
	}
	virtual bool write(FILE* fp) {
		return fwrite(c, 4, 3, fp) == 3;
	}
	
	virtual float fx() const { return x; }
	virtual float fy() const { return y; }
	virtual float fz() const { return z; }
	virtual double dx() const { return x; }
	virtual double dy() const { return y; }
	virtual double dz() const { return z; }
};
class DoubleVertex : public Vertex {
private:
	union {
		struct {
			double x, y, z;
		};
		double c[3];
	};
	
public:
	DoubleVertex() {}
	DoubleVertex(const DoubleVertex& rhs) {
		memcpy(c, rhs.c, sizeof(double) * 3);
	}
	DoubleVertex(const Vertex& rhs) {
		x = rhs.dx();
		y = rhs.dy();
		z = rhs.dz();
	}
	virtual ~DoubleVertex() {}
	
	virtual void operator=(const Vertex& rhs) {
		x = rhs.dx();
		y = rhs.dy();
		z = rhs.dz();
	}
	virtual bool operator==(const Vertex& rhs) const {
		return x == rhs.dx() && y == rhs.dy() && z == rhs.dz();
	}
	
	virtual bool read(FILE* fp) {
		return fread(c, 8, 3, fp) == 3;
	}
	virtual bool write(FILE* fp) {
		return fwrite(c, 8, 3, fp) == 3;
	}
	
	virtual float fx() const { return x; }
	virtual float fy() const { return y; }
	virtual float fz() const { return z; }
	virtual double dx() const { return x; }
	virtual double dy() const { return y; }
	virtual double dz() const { return z; }

};

class VertexPtrHash {
public:
	size_t operator()(const Vertex* v) const {
		return std::hash<float>()(v->fx())
			^ std::hash<float>()(v->fy())
			^ std::hash<float>()(v->fz());
	}
};
class VertexPtrEquals {
public:
	bool operator()(const Vertex* const lhs, const Vertex* const rhs) const {
		return *lhs == *rhs;
	}
};




class Face {
public:
	virtual ~Face() {}

	virtual int sides() const = 0;
	virtual bool read(FILE* fp) = 0;
	virtual bool write(FILE* fp) = 0;
};
class Triangle : public Face {
public:
	union {
		struct {
			uint32_t a, b, c;
		};
		uint32_t v[3];
	};

	Triangle() {}
	Triangle(const Triangle& rhs) {
		a = rhs.a;
		b = rhs.b;
		c = rhs.c;
	}
	Triangle(Mesh* mesh, STLTri tri);
	virtual ~Triangle() {}

	virtual int sides() const { return 3; }
	virtual bool read(FILE* fp) {
		return fread(v, 4, 3, fp) == 3;
	}
	virtual bool write(FILE* fp) {
		return fwrite(v, 4, 3, fp) == 3;
	}
};
class Quad : public Face {
public:
	union {
		struct {
			uint32_t a, b, c, d;
		};
		uint32_t v[4];
	};

	Quad() {}
	Quad(const Quad& rhs) {
		a = rhs.a;
		b = rhs.b;
		c = rhs.c;
		d = rhs.d;
	}
	virtual ~Quad() {}
	
	virtual int sides() const { return 4; }
	virtual bool read(FILE* fp) {
		return fread(v, 4, 4, fp) == 4;
	}
	virtual bool write(FILE* fp) {
		return fwrite(v, 4, 4, fp) == 4;
	}
};



struct Mesh {
	std::list<Vertex*> v;
	std::list<Triangle*> t;
	std::list<Quad*> q;
	
	std::unordered_map<Vertex*,size_t,VertexPtrHash,VertexPtrEquals> lookup;
	
	~Mesh() {
		for (auto& i : v) delete &*i;
		for (auto& i : t) delete &*i;
		for (auto& i : q) delete &*i;
	}
};


#endif