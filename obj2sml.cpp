#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <filesystem>

#include "mesh.h"
#include "obj.h"
#include "sml.h"

using namespace std;

#ifndef _MSC_VER
#include <getopt.h>

static const struct option longopts[] = {
	{"comment",		required_argument,	0,	'c'},
	{"strip",		optional_argument,	0,	's'},
	{"help",		no_argument,		0,	'h'},
	{0, 0, 0, 0}
};
#endif

int main(int argc, char* argv[]) {
	uint32_t writeflags = 0;
	vector<string> comments;
	
	#ifdef _MSC_VER
	int optind = 1;
	#else
	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "c:s::h", longopts, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 'c': {
				comments.emplace_back(optarg);
			} break;
			
			case 's': {
				if (optarg && optarg[0]) {
					if (!strcasecmp(optarg, "map")) writeflags = SMLFlags::STRIP_MAP;
					if (!strcasecmp(optarg, "next")) writeflags = SMLFlags::STRIP_NEXT;
					if (!strcasecmp(optarg, "all")) writeflags = SMLFlags::STRIP_EXHAUSTIVE;
				} else {
					writeflags = SMLFlags::STRIP_MAP;
				}
			} break;
			
			case 'h':
				printf("Usage: %s [options] <file.stl>...\n"
					"Options:\n"
					"-c=<...> --comment=<...>     Add the specified text to the resulting SML file as a comment.\n"
					"                             Can be used more than once for multiple comments.\n"
					"-s[=mode] --strip[=mode]     Attempt to find triangle strips in the model.\n"
					"                             Mode can be one of: map (default), next, all\n"
					"                               map: The default, uses a spatial map to check nearby triangles.\n"
					"                               next: Checks the next 1000 triangles in the source file.\n"
					"                               all: Does an exhaustive scan of all triangles. Very slow.\n"
					, argv[0]);
				return 1;
		}
	}
	#endif
	
	for (int i = optind; i < argc; i++) {
		filesystem::path file(argv[i]);
		Mesh* mesh = readOBJ(file);
		
		file.replace_extension(".sml");
		mesh->comments = comments;
		
		writeSML(file, mesh, writeflags);
		
		delete mesh;
		printf("\n");
	}
	
	printf("\nAll done.\n");
	return 0;
}