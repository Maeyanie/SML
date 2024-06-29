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




typedef vector<Triangle*> TriVector;

#ifdef USE_SPARSEHASH
uint32_t dosearch_link(const Triangle* prev, const dense_hash_set<Triangle*>& used, const dense_hash_map<uint32_t, TriVector*>& links, 
		uint32_t count, list<Triangle*>* strip)
#else
uint32_t dosearch_link(const Triangle* prev, const unordered_set<Triangle*>& used, const unordered_map<uint32_t, TriVector*> links, 
		uint32_t count, list<Triangle*>* strip)
#endif
{
	#ifdef USE_SPARSEHASH
	dense_hash_set<Triangle*> stripused;
	stripused.set_empty_key(NULL);
	#else
	unordered_set<Triangle*> stripused;
	#endif
	bool keepgoing;
	bool found;
	do {
		keepgoing = false;
		found = false;
		/* count=0  0 1 2
		   count=1  0 2 3  a==a, b==c
		   count=2	3 2 4  a==c, b==b */
		   
		if (count & 1) {
			list<Triangle*> near;
			auto li = links.find(prev->b);
			if (li != links.end()) near.insert(near.end(), li->second->begin(), li->second->end());
			li = links.find(prev->c);
			if (li != links.end()) near.insert(near.end(), li->second->begin(), li->second->end());
			
			for (Triangle* cur : near) {
				if (used.find(cur) != used.end()) continue;
				if (stripused.find(cur) != stripused.end()) continue;
				
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
				stripused.insert(cur);
				strip->push_back(cur);
				prev = cur;
				keepgoing = true;
				count++;
				found = true;
				break;
			}
			if (found) break;
		} else {
			list<Triangle*> near;
			auto li = links.find(prev->a);
			if (li != links.end()) near.insert(near.end(), li->second->begin(), li->second->end());
			li = links.find(prev->c);
			if (li != links.end()) near.insert(near.end(), li->second->begin(), li->second->end());
			
			for (Triangle* cur : near) {
				if (used.find(cur) != used.end()) continue;
				if (stripused.find(cur) != stripused.end()) continue;
				
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
				stripused.insert(cur);
				strip->push_back(cur);
				prev = cur;
				keepgoing = true;
				count++;
				found = true;
				break;
			}
			if (found) break;
		}
	} while (keepgoing);
	
	return count;
}

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
		
		Triangle* tri = queue.front();
		queue.pop_front();
		queueSize--;
		used.insert(tri);
		
		list<Triangle*> strip[3];
		int best = 0;
		int bestcount = 0;
		
		for (int i = 0; i < 3; i++) {
			strip[i].clear();
			strip[i].push_back(tri);
		
			uint32_t count = dosearch_link(tri, used, links, 1, &(strip[i]));
			if (count > bestcount) {
				best = i;
				bestcount = count;
			}

			if (count > 0) break; // Reproduce old behaviour for comparison.
			tri->rotate(1);
		}
		if (bestcount == 1) {
			singles.push_back(tri);
		} else {
			for (Triangle* t : strip[best]) {
				used.insert(t);
			}
			strips.push_back(move(strip[best]));
		}
	}
	printf("\nDone.\n");
}