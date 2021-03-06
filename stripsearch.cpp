#include "config.h"
#include <stdio.h>
#include <list>
#include <map>
#ifdef USE_SPARSEHASH
#include <sparsehash/dense_hash_set>
using namespace google;
#else
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
	time_t nextUpdate = time(NULL) + 1;
	printf("Finding strips:\n");
	while (!queue.empty()) {
		if (time(NULL) > nextUpdate) {
			/*printf("queued=%lu ", queue.size());
			for (auto& i : striplengths) {
				printf("%u=%u ", i.first, i.second);
			}*/
			printf("%lu ", (unsigned long)queueSize);
			fflush(stdout);
			printf("\r");
			nextUpdate = time(NULL) + 1;
		}
		if (dirtiness >= (mapSize*mapSize*mapSize)) {
			mesh->spatialMap.compact();
			dirtiness = 0;
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