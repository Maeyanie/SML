#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <filesystem>
#include <map>
#include <unordered_set>
#include "mesh.h"
#include "sml.h"
using namespace std;

Mesh* readSML(std::filesystem::path file) {
	Mesh* mesh = new Mesh();
	
	#ifdef _WIN32
		FILE* fp = _wfopen(file.c_str(), L"rb");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%ls' for reading: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		FILE* fp = fopen(file.c_str(), "r");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%s' for reading: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	char magic[4];
	size_t ret = fread(magic, 1, 4, fp);
	assert(ret == 4);
	assert(!memcmp(magic, "SML1", 4));
	
	uint32_t crc;
	ret = fread(&crc, 4, 1, fp);
	assert(ret == 1);
	
	// Check CRC
	{
		printf("Computing CRC32C...");
		fflush(stdout);
		size_t len;
		char* buffer = (char*)malloc(1*1024*1024);
		uint32_t crc2 = 0;
		while ((len = fread(buffer, 1, (1*1024*1024), fp)) > 0) {
			crc2 = crc32c(crc2, buffer, len);
		}
		free(buffer);
		printf("read %08x, calculated %08x\n", crc, crc2);
		assert(crc == crc2);
		
		fseek(fp, 8, SEEK_SET);
	}


	struct __attribute__((packed)) {
		uint8_t type;
		uint32_t length;
	} header;
	while (fread(&header, 5, 1, fp) == 1) {
		printf("Reading segment, type %hhu, length %u...", header.type, header.length);
		fflush(stdout);
		
		switch (header.type) {
			case 0:
				fseek(fp, header.length, SEEK_CUR);
				break;
			
			case 1: { // Vertex float list
				int verts = header.length / 12;
				for (int i = 0; i < verts; i++) {
					shared_ptr<Vertex> v = make_shared<Vertex>();
					v->read(fp);
					mesh->v.push_back(v);
				}
			} break;
			
			case 2: { // Vertex double list; here we're just dumping the extra precision and converting to floats.
				int verts = header.length / 24;
				for (int i = 0; i < verts; i++) {
					shared_ptr<Vertex> v = make_shared<Vertex>();
					v->read(fp);
					mesh->v.push_back(v);
				}				
			} break;
			
			case 3: { // Triangle list
				int tris = header.length / 12;
				for (int i = 0; i < tris; i++) {
					shared_ptr<Triangle> t = make_shared<Triangle>();
					t->read(fp);
					mesh->t.push_back(t);
				}
			} break;
			
			case 4: { // Quad list
				int quads = header.length / 16;
				for (int i = 0; i < quads; i++) {
					shared_ptr<Quad> q = make_shared<Quad>();
					q->read(fp);
					mesh->q.push_back(q);
				}
			} break;
			
			case 5: { // Triangle strip
				int points = header.length / 4;
				assert(points >= 3);
				
				uint32_t verts[3];
				
				fread(verts, 4, 3, fp);
				mesh->t.push_back(make_shared<Triangle>(verts));
				//printf("\nStrip: %u,%u,%u\n", verts[0], verts[1], verts[2]);
				
				for (int i = 3; i < points; i++) {
					/* i=0  0 1 2
					   i=3  0 2 3	2->1
					   i=4  3 2 4	2->0 */
					verts[i & 1] = verts[2];
					ret = fread(&verts[2], 4, 1, fp);
					assert(ret == 1);
					
					//printf("\t%u,%u,%u\n", verts[0], verts[1], verts[2]);
					mesh->t.push_back(make_shared<Triangle>(verts));
				}
			} break;
			
			default:
				fprintf(stderr, "Error: Unrecognized segment type '%hhu'\n", header.type);
				exit(__LINE__);
		}
		printf("Done.\n");
	}
	return mesh;
}



void writeSML(filesystem::path file, Mesh* mesh) {
	printf("Building spatial map...");
	fflush(stdout);
	mesh->spatialMap.build();
	printf("Done.\n");
	
	#ifdef _WIN32
		printf("Writing to %ls...\n", file.c_str());

		FILE* fp = _wfopen(file.c_str(), L"wb+");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%ls' for writing: %s\n", file.c_str(), strerror(errno));
			exit(__LINE__);
		}
	#else
		printf("Writing to %s...\n", file.c_str());

		FILE* fp = fopen(file.c_str(), "w+");
		if (!fp) {
			fprintf(stderr, "Could not open SML file '%s' for writing: %m\n", file.c_str());
			exit(__LINE__);
		}
	#endif
	
	fwrite("SML1", 4, 1, fp); // Identifying header
	// Skip CRC until the end.
	fseek(fp, 8, SEEK_SET);
	
	uint8_t type = 1;
	fwrite(&type, 1, 1, fp);
	
	size_t vertCount = mesh->v.size();
	assert(vertCount <= 357913941);
	printf("Writing %u vertices...", (uint32_t)vertCount);
	fflush(stdout);
	uint32_t vertLength = vertCount * 12;
	fwrite(&vertLength, 4, 1, fp);
	for (auto& i : mesh->v) {
		i->write(fp);
	}
	printf("Done.\n");
	
	
	if (!mesh->t.empty()) {
		#ifdef NO_STRIP
			type = 3;
			fwrite(&type, 1, 1, fp);
			
			size_t triCount = mesh->t.size();
			assert(triCount <= 357913941);
			printf("Writing %u triangles...", (uint32_t)triCount);
			fflush(stdout);
			uint32_t triLength = triCount * 12;
			fwrite(&triLength, 4, 1, fp);
			for (auto& i : mesh->t) {
				i->write(fp);
			}
			printf("Done.\n");			
		#else
			//list<shared_ptr<Triangle>> queue(mesh->t);
			list<Triangle*> queue;
			for (auto& i : mesh->t) {
				queue.push_back(i.get());
			}
			unordered_set<Triangle*> used;			
			list<Triangle*> singles;
			
			list<Triangle*> strip;
			map<uint32_t,uint32_t> striplengths;
			time_t nextUpdate = time(NULL) + 1;
			while (!queue.empty()) {
				if (used.find(queue.front()) != used.end()) {
					queue.pop_front();
					continue;
				}
				Triangle* prev = queue.front();
				queue.pop_front();
				strip.push_back(prev);
				used.insert(prev);
				
				if (time(NULL) > nextUpdate) {
					printf("Writing strips: queued=%lu ", queue.size());
					for (auto& i : striplengths) {
						printf("%u=%u ", i.first, i.second);
					}
					fflush(stdout);
					printf("\r");
					nextUpdate++;
				}

				
				bool keepgoing;
				uint32_t count = 3; // Start from 3 to match reading, above. Doesn't really matter.
				do {
					next:
					keepgoing = false;
					/* count=0  0 1 2
					   count=3  0 2 3  a==a, b==c
					   count=4	3 2 4  a==c, b==b */
					   
					//uint32_t fails = 0;
					if (count & 1) {
						//printf("Strip check 1: count=%u tri=%u,%u,%u\n", count, prev->a, prev->b, prev->c);
						
						list<vector<uint32_t>*> near = mesh->spatialMap.getNear(*mesh->v[prev->b]);
						{
							auto n = mesh->spatialMap.getNear(*mesh->v[prev->c]);
							near.splice(near.end(), n);
						}
						
						for (auto list : near) {
							for (auto j : *list) {
								Triangle* cur = mesh->t[j].get();
								if (used.find(cur) != used.end()) continue;
								//printf("\ttri=%u,%u,%u\n", cur->a, cur->b, cur->c);
								
								if (cur->a == prev->a && cur->b == prev->c) {
									//printf("\tStraight match (a==a, b==c): %u,%u,%u\n", cur->a, cur->b, cur->c);
									// Everything looks good from here. (abc)
								} else if (cur->b == prev->a && cur->c == prev->c) {
									//printf("\tRotated +1 match (b==a, c==c): %u,%u,%u\n", cur->a, cur->b, cur->c);
									// Rotated +1 (a=b, b=c, c=a)
									uint32_t temp = cur->a;
									cur->a = cur->b;
									cur->b = cur->c;
									cur->c = temp;
								} else if (cur->c == prev->a && cur->a == prev->c) {
									//printf("\tRotated -1 match (c==a, a==c): %u,%u,%u\n", cur->a, cur->b, cur->c);
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
								goto next;
							}
						}
					} else {
						//printf("Strip check 0: count=%u tri=%u,%u,%u\n", count, prev->a, prev->b, prev->c);
						
						list<vector<uint32_t>*> near = mesh->spatialMap.getNear(*mesh->v[prev->a]);
						{
							auto n = mesh->spatialMap.getNear(*mesh->v[prev->c]);
							near.splice(near.end(), n);
						}
						
						for (auto list : near) {
							for (auto j : *list) {
								Triangle* cur = mesh->t[j].get();
								if (used.find(cur) != used.end()) continue;
								//printf("\ttri=%u,%u,%u\n", cur->a, cur->b, cur->c);
								
								if (cur->a == prev->c && cur->b == prev->b) {
									//printf("\tStraight match (a==c, b==b): %u,%u,%u\n", cur->a, cur->b, cur->c);
									// Yes, this is a fertile land. (abc)
								} else if (cur->b == prev->c && cur->c == prev->b) {
									//printf("\tRotated +1 match (b==a, c==c): %u,%u,%u\n", cur->a, cur->b, cur->c);
									// Rotated +1 (a=b, b=c, c=a)
									uint32_t temp = cur->a;
									cur->a = cur->b;
									cur->b = cur->c;
									cur->c = temp;
								} else if (cur->c == prev->c && cur->a == prev->b) {
									//printf("\tRotated -1 match (c==a, a==c): %u,%u,%u\n", cur->a, cur->b, cur->c);
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
								goto next;
							}
						}
					}
				} while (keepgoing);
				
				uint32_t striplen = (uint32_t)strip.size();
				if (striplen == 1) {
					singles.push_back(prev);
				} else {
					{
						auto i = striplengths.find(striplen);
						if (i != striplengths.end()) {
							striplengths[striplen] = striplengths[striplen] + 1;
						} else {
							striplengths[striplen] = 1;
						}
					}
					
					type = 5;
					fwrite(&type, 1, 1, fp);
					
					assert(striplen <= 1073741821);
					uint32_t stripLength = (striplen+2) * sizeof(uint32_t);
					fwrite(&stripLength, 4, 1, fp);

					Triangle* first = strip.front();
					strip.pop_front();
					first->write(fp);
					
					for (auto& i : strip) {
						fwrite(&(i->c), 4, 1, fp);
					}
				}
				strip.clear();
			}
			if (!striplengths.empty()) printf("\n");

			
			
			size_t triCount = singles.size();
			assert(triCount <= 357913941);
			if (triCount > 0) {
				printf("Writing %u lone triangles...", (uint32_t)triCount);
				fflush(stdout);
				type = 3;
				fwrite(&type, 1, 1, fp);
				uint32_t triLength = triCount * 12;
				fwrite(&triLength, 4, 1, fp);
				for (auto& i : singles) {
					i->write(fp);
				}
				printf("Done.\n");
			}
		#endif
	}
	if (!mesh->q.empty()) {
		type = 4;
		fwrite(&type, 1, 1, fp);
		
		size_t quadCount = mesh->q.size();
		assert(quadCount <= 268435455);
		printf("Writing %u quads...", (uint32_t)quadCount);
		fflush(stdout);
		uint32_t quadLength = quadCount * 16;
		fwrite(&quadLength, 4, 1, fp);
		for (auto& i : mesh->q) {
			i->write(fp);
		}
		printf("Done.\n");
	}
	
	
	// Write CRC
	printf("Computing CRC32C...");
	fflush(stdout);
	fseek(fp, 8, SEEK_SET);
	size_t len;
	char* buffer = (char*)malloc(1*1024*1024);
	uint32_t crc = 0;
	while ((len = fread(buffer, 1, (1*1024*1024), fp)) > 0) {
		crc = crc32c(crc, buffer, len);
	}
	free(buffer);
	fseek(fp, 4, SEEK_SET);
	fwrite(&crc, 4, 1, fp);
	printf("%08x\n", crc);
	fclose(fp);
	
	printf("File written.\n");
}