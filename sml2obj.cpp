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

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		filesystem::path file(argv[i]);
		Mesh* mesh = readSML(file);
		
		file.replace_extension(".obj");
		writeOBJ(file, mesh);
		
		delete mesh;
		printf("\n");
	}
	
	printf("\nAll done.\n");
}