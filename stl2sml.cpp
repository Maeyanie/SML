#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <unordered_map>

#include "config.h"
#include "mesh.h"
#include "stl.h"
#include "sml.h"

using namespace std;

#ifndef _MSC_VER
#include <getopt.h>

static const struct option longopts[] = {
	{"comment",		required_argument,	0,	'c'},
	{"strip",		optional_argument,	0,	's'},
	{"rm",			no_argument,		0,   1 },
	{"help",		no_argument,		0,	'h'},
	{0, 0, 0, 0}
};
#endif

int main(int argc, char* argv[]) {
	uint32_t writeflags = 0;
	vector<string> comments;
	bool rm = 0;

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
					if (!strcasecmp(optarg, "link")) writeflags = SMLFlags::STRIP_LINK;
					#ifdef USE_BOOST
					if (!strcasecmp(optarg, "boost")) writeflags = SMLFlags::STRIP_BOOST;
					#endif
				} else {
					writeflags = SMLFlags::STRIP_LINK;
				}
			} break;
			
			case 1:
				rm = 1;
				break;
			
			case 'h':
				printf("Usage: %s [options] <file.stl>...\n"
					"Options:\n"
					"-c=<...> --comment=<...>     Add the specified text to the resulting SML file as a comment.\n"
					"                             Can be used more than once for multiple comments.\n"
					"-s --strip                   Attempt to find triangle strips in the model.\n"
					"--rm                         Remove original file after converting.\n"
					, argv[0]);
				return 1;
		}
	}
	#endif
	
	for (int i = optind; i < argc; i++) {
		filesystem::path file(argv[i]);
		Mesh* mesh = readSTL(file);
		
		file.replace_extension(".sml");
		mesh->comments = comments;
		
		writeSML(file, mesh, writeflags);
		
		delete mesh;
		if (rm) filesystem::remove(argv[i]);
		printf("\n");
	}
	
	printf("\nAll done.\n");
	return 0;
}