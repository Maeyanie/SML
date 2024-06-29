#include "config.h"
#include <stdio.h>
#include <list>
#include <map>
#ifdef USE_SPARSEHASH
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>
using namespace google;
#else
#include <unordered_map>
#include <unordered_set>
#endif

#include "mesh.h"
#include "sml.h"

using namespace std;

void stripsearch_map(Mesh* mesh, list<Triangle*>& singles, list<list<Triangle*>>& strips) {
	list<Triangle*> queue;
	uint32_t queueSize = 0;
	for (auto& i : mesh->t) {
		queue.push_back(i.get());
		queueSize++;
	}
	#ifdef USE_SPARSEHASH
	dense_hash_set<Triangle*> used(queueSize);
	used.set_empty_key(NULL);
	#else
	unordered_set<Triangle*> used;
	used.reserve(queueSize);
	#endif
	
	const uint32_t mapSize = mesh->spatialMap.mapSize;
	uint32_t dirtiness = 0;
	list<Triangle*> strip;
	//map<uint32_t,uint32_t> striplengths;
	time_t tNow = time(NULL);
	time_t nextUpdate = tNow + 1;
	time_t nextCompact = tNow + 60;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		tNow = time(NULL);
		if (tNow > nextUpdate) {
			/*printf("queued=%lu ", queue.size());
			for (auto& i : striplengths) {
				printf("%u=%u ", i.first, i.second);
			}*/
			printf("%lu ", (unsigned long)queueSize);
			fflush(stdout);
			printf("\r");
			nextUpdate = tNow + 1;
		}
		if (tNow > nextCompact && dirtiness >= 10000) {
			printf("Compacting, dirtiness %u.\n", dirtiness);
			mesh->spatialMap.compact();
			dirtiness = 0;
			nextCompact = tNow + 60;
		}
		if (used.find(queue.front()) != used.end()) {
			queue.pop_front();
			queueSize--;
			continue;
		}
		Triangle* prev = queue.front();
		queue.pop_front();
		queueSize--;
		strip.push_back(prev);
		used.insert(prev);
		
		bool keepgoing;
		bool found;
		uint32_t count = 1;
		for (int o = 0; o < 2; o++) {
			do {
				//next:
				keepgoing = false;
				found = false;
				/* count=0  0 1 2
				   count=1  0 2 3  a==a, b==c
				   count=2	3 2 4  a==c, b==b */
				   
				if (count & 1) {
					list<vector<uint32_t>*> near = mesh->spatialMap.getNear(*mesh->v[prev->b]);
					near.splice(near.end(), mesh->spatialMap.getNear(*mesh->v[prev->c]));
					
					for (auto list : near) {
						for (auto j = list->begin(); j != list->end(); ++j) {
							if (*j == 0xFFFFFFFF) continue;
							Triangle* cur = mesh->t[*j].get();
							if (used.find(cur) != used.end()) {
								*j = 0xFFFFFFFF;
								dirtiness++;
								continue;
							}
							
							if (cur->a == prev->a && cur->b == prev->c) {
								// Everything looks good from here. (abc)
							} else if (cur->b == prev->a && cur->c == prev->c) {
								// Rotated +1 (a=b, b=c, c=a)
								uint32_t temp = cur->a;
								cur->a = cur->b;
								cur->b = cur->c;
								cur->c = temp;
							} else if (cur->c == prev->a && cur->a == prev->c) {
								// Rotated -1 (a=c, b=a, c=b)
								uint32_t temp = cur->c;
								cur->c = cur->b;
								cur->b = cur->a;
								cur->a = temp;
							} else {
								continue;
							}
							*j = 0xFFFFFFFF;
							dirtiness++;
							used.insert(cur);
							strip.push_back(cur);
							prev = cur;
							keepgoing = true;
							count++;
							//goto next;
							found = true;
							break;
						}
						if (found) break;
					}
				} else {
					list<vector<uint32_t>*> near = mesh->spatialMap.getNear(*mesh->v[prev->a]);
					near.splice(near.end(), mesh->spatialMap.getNear(*mesh->v[prev->c]));
					
					for (auto list : near) {
						for (auto j = list->begin(); j != list->end(); ++j) {
							if (*j == 0xFFFFFFFF) continue;
							Triangle* cur = mesh->t[*j].get();
							if (used.find(cur) != used.end()) {
								*j = 0xFFFFFFFF;
								dirtiness++;
								continue;
							}
							
							if (cur->a == prev->c && cur->b == prev->b) {
								// Yes, this is a fertile land. (abc)
							} else if (cur->b == prev->c && cur->c == prev->b) {
								// Rotated +1 (a=b, b=c, c=a)
								uint32_t temp = cur->a;
								cur->a = cur->b;
								cur->b = cur->c;
								cur->c = temp;
							} else if (cur->c == prev->c && cur->a == prev->b) {
								// Rotated -1 (a=c, b=a, c=b)
								uint32_t temp = cur->c;
								cur->c = cur->b;
								cur->b = cur->a;
								cur->a = temp;
							} else {
								//if (fails++ > 10000) break;
								continue;
							}
							*j = 0xFFFFFFFF;
							dirtiness++;
							used.insert(cur);
							strip.push_back(cur);
							prev = cur;
							keepgoing = true;
							count++;
							//goto next;
							found = true;
							break;
						}
						if (found) break;
					}
				}
			} while (keepgoing);
			
			if (count > 1) break;
			uint32_t temp = prev->a;
			prev->a = prev->b;
			prev->b = prev->c;
			prev->c = temp;
		}
		
		if (count == 1) {
			singles.push_back(prev);
		} else {
			strips.push_back(move(strip));
		}
		
		/*auto i = striplengths.find(count);
		if (i != striplengths.end()) {
			striplengths[count] = striplengths[count] + 1;
		} else {
			striplengths[count] = 1;
		}*/
		
		strip.clear();
	}
	printf("\nDone.\n");
}



void stripsearch_next(Mesh* mesh, list<Triangle*>& singles, list<list<Triangle*>>& strips) {
	list<Triangle*> queue;
	for (auto& i : mesh->t) {
		queue.push_back(i.get());
	}		
	
	list<Triangle*> strip;
	//map<uint32_t,uint32_t> striplengths;
	time_t nextUpdate = time(NULL) + 1;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		Triangle* prev = queue.front();
		queue.pop_front();
		strip.push_back(prev);
		
		if (time(NULL) > nextUpdate) {
			/*printf("queued=%lu ", queue.size());
			for (auto& i : striplengths) {
				printf("%u=%u ", i.first, i.second);
			}*/
			printf("%lu ", queue.size());
			fflush(stdout);
			printf("\r");
			nextUpdate++;
		}

		
		bool keepgoing;
		uint32_t fails;
		uint32_t count = 1;
		for (int o = 0; o < 2; o++) {
			do {
				keepgoing = false;
				fails = 0;

				if (count & 1) {
					for (auto j = queue.begin(); j != queue.end(); j++) {
						Triangle* cur = *j;
						
						if (cur->a == prev->a && cur->b == prev->c) {
							// Everything looks good from here. (abc)
						} else if (cur->b == prev->a && cur->c == prev->c) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->a && cur->a == prev->c) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							if (fails++ > 1000) break;
							continue;
						}
						j = queue.erase(j);
						j--;
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						break;
					}
				} else {
					for (auto j = queue.begin(); j != queue.end(); j++) {
						Triangle* cur = *j;
							
						if (cur->a == prev->c && cur->b == prev->b) {
							// Yes, this is a fertile land. (abc)
						} else if (cur->b == prev->c && cur->c == prev->b) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->c && cur->a == prev->b) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							if (fails++ > 1000) break;
							continue;
						}
						j = queue.erase(j);
						j--;
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						break;
					}
				}
			} while (keepgoing);
			
			if (count > 1) break;
			uint32_t temp = prev->a;
			prev->a = prev->b;
			prev->b = prev->c;
			prev->c = temp;
		}
		
		if (count == 1) {
			singles.push_back(prev);
		} else {
			strips.push_back(move(strip));
		}
		
		/*auto i = striplengths.find(count);
		if (i != striplengths.end()) {
			striplengths[count] = striplengths[count] + 1;
		} else {
			striplengths[count] = 1;
		}*/
	}
	printf("\nDone.\n");
}



void stripsearch_exhaustive(Mesh* mesh, list<Triangle*>& singles, list<list<Triangle*>>& strips) {
	list<Triangle*> queue;
	for (auto& i : mesh->t) {
		queue.push_back(i.get());
	}
	
	list<Triangle*> strip;
	//map<uint32_t,uint32_t> striplengths;
	time_t nextUpdate = time(NULL) + 1;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		Triangle* prev = queue.front();
		queue.pop_front();
		strip.push_back(prev);
		
		if (time(NULL) > nextUpdate) {
			/*printf("queued=%lu ", queue.size());
			for (auto& i : striplengths) {
				printf("%u=%u ", i.first, i.second);
			}*/
			printf("%lu ", queue.size());
			fflush(stdout);
			printf("\r");
			nextUpdate++;
		}

		
		bool keepgoing;
		uint32_t count = 1;
		for (int o = 0; o < 2; o++) {
			do {
				keepgoing = false;

				if (count & 1) {
					for (auto j = queue.begin(); j != queue.end(); j++) {
						Triangle* cur = *j;
						
						if (cur->a == prev->a && cur->b == prev->c) {
							// Everything looks good from here. (abc)
						} else if (cur->b == prev->a && cur->c == prev->c) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->a && cur->a == prev->c) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							continue;
						}
						j = queue.erase(j);
						j--;
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						break;
					}
				} else {
					for (auto j = queue.begin(); j != queue.end(); j++) {
						Triangle* cur = *j;
							
						if (cur->a == prev->c && cur->b == prev->b) {
							// Yes, this is a fertile land. (abc)
						} else if (cur->b == prev->c && cur->c == prev->b) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->c && cur->a == prev->b) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							continue;
						}
						j = queue.erase(j);
						j--;
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						break;
					}
				}
			} while (keepgoing);

			if (count > 1) break;
			uint32_t temp = prev->a;
			prev->a = prev->b;
			prev->b = prev->c;
			prev->c = temp;
		}
		
		if (count == 1) {
			singles.push_back(prev);
		} else {
			strips.push_back(move(strip));
		}
		
		/*auto i = striplengths.find(count);
		if (i != striplengths.end()) {
			striplengths[count] = striplengths[count] + 1;
		} else {
			striplengths[count] = 1;
		}*/
		
		strip.clear();
	}
	printf("\nDone.\n");
}



#ifdef USE_BOOST
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp> 

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<float, 3, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
//typedef std::pair<point, uint32_t> value;

typedef std::pair<box, uint32_t> value;

//typedef bg::model::polygon<point, false> polygon;
//typedef std::pair<polygon, uint32_t> value;


void stripsearch_boost(Mesh* mesh, list<Triangle*>& singles, list<list<Triangle*>>& strips) {
	bgi::rtree<value, bgi::quadratic<16>> rt;
	list<Triangle*> queue;
	uint32_t queueSize = 0;
	
	printf("Building spatial index...");
	fflush(stdout);
	for (size_t tri = 0; tri < mesh->t.size(); tri++) {
		Triangle* t = mesh->t[tri].get();
		queue.push_back(t);
		queueSize++;
		
		/*for (int v = 0; v < 3; v++) {
			shared_ptr<Vertex> vert = mesh->v[t->v[v]];
			rt.insert(std::make_pair(point(vert->x, vert->y, vert->z), tri));
		}*/
		
		float minX = mesh->v[t->v[0]]->x;
		float maxX = minX;
		float minY = mesh->v[t->v[0]]->y;
		float maxY = minY;
		float minZ = mesh->v[t->v[0]]->z;
		float maxZ = minZ;
		
		for (int v = 1; v < 3; v++) {
			minX = std::min(minX, mesh->v[t->v[v]]->x);
			maxX = std::max(maxX, mesh->v[t->v[v]]->x);
			minY = std::min(minY, mesh->v[t->v[v]]->y);
			maxY = std::max(maxY, mesh->v[t->v[v]]->y);
			minZ = std::min(minZ, mesh->v[t->v[v]]->z);
			maxZ = std::max(maxZ, mesh->v[t->v[v]]->z);
		}
		rt.insert(std::make_pair(box(point(minX,minY,minZ), point(maxX,maxY,maxZ)), tri));
		
		/*polygon p{
				{mesh->v[t->v[0]]->x, mesh->v[t->v[0]]->y, mesh->v[t->v[0]]->z},
				{mesh->v[t->v[1]]->x, mesh->v[t->v[1]]->y, mesh->v[t->v[1]]->z},
				{mesh->v[t->v[2]]->x, mesh->v[t->v[2]]->y, mesh->v[t->v[2]]->z},
				{mesh->v[t->v[0]]->x, mesh->v[t->v[0]]->y, mesh->v[t->v[20]->z}
			};
		rt.insert(std::make_pair(p, tri));*/

	}
	printf("Done.\n");
	
	#ifdef USE_SPARSEHASH
	dense_hash_set<Triangle*> used(queueSize);
	used.set_empty_key(NULL);
	#else
	unordered_set<Triangle*> used;
	used.reserve(queueSize);
	#endif
	
	
	
	list<Triangle*> strip;
	time_t nextUpdate = time(NULL) + 1;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		if (time(NULL) > nextUpdate) {
			printf("%lu ", queueSize);
			fflush(stdout);
			printf("\r");
			nextUpdate++;
		}
		
		if (used.find(queue.front()) != used.end()) {
			queue.pop_front();
			queueSize--;
			continue;
		}

		Triangle* prev = queue.front();
		queue.pop_front();
		strip.push_back(prev);
		used.insert(prev);

		bool found;
		bool keepgoing;
		uint32_t count = 1;
		for (int o = 0; o < 2; o++) {
			do {
				keepgoing = false;
				found = false;
				/* count=0  0 1 2
				   count=1  0 2 3  a==a, b==c
				   count=2	3 2 4  a==c, b==b */
				   
				if (count & 1) {
					vector<value> near;
					near.reserve(64);

					shared_ptr<Vertex> vb = mesh->v[prev->b];
					point pb = point(vb->x, vb->y, vb->z);
					bgi::query(rt, bgi::nearest(pb, 32), std::back_inserter(near));
					
					shared_ptr<Vertex> vc = mesh->v[prev->c];
					point pc = point(vc->x, vc->y, vc->z);
					bgi::query(rt, bgi::nearest(pc, 32), std::back_inserter(near));
					
					for (auto& j : near) {
						Triangle* cur = mesh->t[j.second].get();
						if (cur == prev) continue;
						if (used.find(cur) != used.end()) continue;
						
						if (cur->a == prev->a && cur->b == prev->c) {
							// Everything looks good from here. (abc)
						} else if (cur->b == prev->a && cur->c == prev->c) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->a && cur->a == prev->c) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							continue;
						}
						used.insert(cur);
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						found = true;
						break;
					}
					if (found) break;
				} else {
					vector<value> near;
					near.reserve(64);
					
					shared_ptr<Vertex> va = mesh->v[prev->a];
					point pa = point(va->x, va->y, va->z);
					bgi::query(rt, bgi::nearest(pa, 32), std::back_inserter(near));
					
					shared_ptr<Vertex> vc = mesh->v[prev->c];
					point pc = point(vc->x, vc->y, vc->z);
					bgi::query(rt, bgi::nearest(pc, 32), std::back_inserter(near));
					
					for (auto j : near) {
						Triangle* cur = mesh->t[j.second].get();
						if (cur == prev) continue;
						if (used.find(cur) != used.end()) continue;
						
						if (cur->a == prev->c && cur->b == prev->b) {
							// Yes, this is a fertile land. (abc)
						} else if (cur->b == prev->c && cur->c == prev->b) {
							// Rotated +1 (a=b, b=c, c=a)
							uint32_t temp = cur->a;
							cur->a = cur->b;
							cur->b = cur->c;
							cur->c = temp;
						} else if (cur->c == prev->c && cur->a == prev->b) {
							// Rotated -1 (a=c, b=a, c=b)
							uint32_t temp = cur->c;
							cur->c = cur->b;
							cur->b = cur->a;
							cur->a = temp;
						} else {
							continue;
						}
						used.insert(cur);
						strip.push_back(cur);
						prev = cur;
						keepgoing = true;
						count++;
						found = true;
						break;
					}
					if (found) break;
				}
			} while (keepgoing);
			
			if (count > 1) break;
			uint32_t temp = prev->a;
			prev->a = prev->b;
			prev->b = prev->c;
			prev->c = temp;
		}
		
		if (count == 1) {
			singles.push_back(prev);
		} else {
			strips.push_back(move(strip));
		}
		
		strip.clear();
	}
	printf("\nDone.\n");
}
#endif



typedef vector<Triangle*> TriVector;

void stripsearch_link(Mesh* mesh, list<Triangle*>& singles, list<list<Triangle*>>& strips) {
	#ifdef USE_SPARSEHASH
	dense_hash_map<uint32_t, TriVector*> links;
	links.set_empty_key(0xFFFFFFFF);
	#else
	unordered_map<uint32_t, TriVector*> links;
	#endif

	list<Triangle*> queue;
	uint32_t queueSize = 0;
	for (auto& i : mesh->t) {
		Triangle* t = i.get();
		queue.push_back(t);
		queueSize++;
		
		for (int vi = 0; vi < 3; vi++) {
			uint32_t v = t->v[vi];
			auto link = links.find(v);
			if (link == links.end()) {
				links.insert(std::make_pair(v, new TriVector({t})));
			} else {
				link->second->push_back(t);
			}
		}
	}

	#ifdef USE_SPARSEHASH
	dense_hash_set<Triangle*> used(queueSize);
	used.set_empty_key(NULL);
	#else
	unordered_set<Triangle*> used;
	used.reserve(queueSize);
	#endif
	
	list<Triangle*> strip;
	time_t tNow = time(NULL);
	time_t nextUpdate = tNow + 1;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		tNow = time(NULL);
		if (tNow > nextUpdate) {
			printf("%lu ", (unsigned long)queueSize);
			fflush(stdout);
			printf("\r");
			nextUpdate = tNow + 1;
		}
		if (used.find(queue.front()) != used.end()) {
			queue.pop_front();
			queueSize--;
			continue;
		}
		Triangle* prev = queue.front();
		queue.pop_front();
		queueSize--;
		strip.push_back(prev);
		used.insert(prev);
		
		bool keepgoing;
		bool found;
		uint32_t count = 1;
		for (int o = 0; o < 2; o++) {
			do {
				//next:
				keepgoing = false;
				found = false;
				/* count=0  0 1 2
				   count=1  0 2 3  a==a, b==c
				   count=2	3 2 4  a==c, b==b */
				   
				if (count & 1) {
					list<TriVector*> near;
					auto li = links.find(prev->b);
					if (li != links.end()) near.push_back(li->second);
					li = links.find(prev->c);
					if (li != links.end()) near.push_back(li->second);
					
					for (auto list : near) {
						for (Triangle* cur : *list) {
							if (used.find(cur) != used.end()) continue;
							
							if (cur->a == prev->a && cur->b == prev->c) {
								// Everything looks good from here. (abc)
							} else if (cur->b == prev->a && cur->c == prev->c) {
								// Rotated +1 (a=b, b=c, c=a)
								uint32_t temp = cur->a;
								cur->a = cur->b;
								cur->b = cur->c;
								cur->c = temp;
							} else if (cur->c == prev->a && cur->a == prev->c) {
								// Rotated -1 (a=c, b=a, c=b)
								uint32_t temp = cur->c;
								cur->c = cur->b;
								cur->b = cur->a;
								cur->a = temp;
							} else {
								continue;
							}
							used.insert(cur);
							strip.push_back(cur);
							prev = cur;
							keepgoing = true;
							count++;
							found = true;
							break;
						}
						if (found) break;
					}
				} else {
					list<TriVector*> near;
					auto li = links.find(prev->a);
					if (li != links.end()) near.push_back(li->second);
					li = links.find(prev->c);
					if (li != links.end()) near.push_back(li->second);
					
					for (auto list : near) {
						for (Triangle* cur : *list) {
							if (used.find(cur) != used.end()) continue;
							
							if (cur->a == prev->c && cur->b == prev->b) {
								// Yes, this is a fertile land. (abc)
							} else if (cur->b == prev->c && cur->c == prev->b) {
								// Rotated +1 (a=b, b=c, c=a)
								uint32_t temp = cur->a;
								cur->a = cur->b;
								cur->b = cur->c;
								cur->c = temp;
							} else if (cur->c == prev->c && cur->a == prev->b) {
								// Rotated -1 (a=c, b=a, c=b)
								uint32_t temp = cur->c;
								cur->c = cur->b;
								cur->b = cur->a;
								cur->a = temp;
							} else {
								//if (fails++ > 10000) break;
								continue;
							}
							used.insert(cur);
							strip.push_back(cur);
							prev = cur;
							keepgoing = true;
							count++;
							found = true;
							break;
						}
						if (found) break;
					}
				}
			} while (keepgoing);
			
			if (count > 1) break;
			uint32_t temp = prev->a;
			prev->a = prev->b;
			prev->b = prev->c;
			prev->c = temp;
		}
		
		if (count == 1) {
			singles.push_back(prev);
		} else {
			strips.push_back(move(strip));
		}
		
		strip.clear();
	}
	printf("\nDone.\n");
}