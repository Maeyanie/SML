#include <stdio.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <list>
#include <vector>

#include "mesh.h"
#include "obj.h"
#include "sml.h"

using namespace std;

#ifndef _MSC_VER
#include <getopt.h>

static const struct option longopts[] = {
	{"rm",			no_argument,		0,   1 },
	{"help",		no_argument,		0,	'h'},
	{0, 0, 0, 0}
};
#endif

int main(int argc, char* argv[]) {
	bool rm = 0;
	
	#ifdef _MSC_VER
	int optind = 1;
	#else
	
	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "h", longopts, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 1:
				rm = 1;
				break;
			
			case 'h':
				printf("Usage: %s [options] <file.stl>...\n"
					"Options:\n"
					"--rm                         Remove original file after converting.\n"
					, argv[0]);
				return 1;
		}
	}
	#endif
	
	for (int i = optind; i < argc; i++) {
		filesystem::path file(argv[i]);
		Mesh* mesh = readSML(file);
		
		file.replace_extension(".obj");
		writeOBJ(file, mesh);
		
		delete mesh;
		if (rm) filesystem::remove(argv[i]);
		printf("\n");
	}
	
	printf("\nAll done.\n");
}