all: stl2sml sml2stl

CFLAGS=-Wall -ggdb -O3 -march=native -flto -fuse-linker-plugin
CXXFLAGS=-std=c++17 -Wall -ggdb -Og -march=native -flto -fuse-linker-plugin

clean:
	rm -f *.o stl2sml sml2stl

crc32c.o: crc32c.c
	$(CC) $(CFLAGS) -c -o $@ $^

stl2sml: stl2sml.o crc32c.o mesh.o stl.o sml.o stripsearch.o
	$(CXX) $(CXXFLAGS) -o $@ $^
sml2stl: sml2stl.o crc32c.o mesh.o stl.o sml.o stripsearch.o
	$(CXX) $(CXXFLAGS) -o $@ $^
obj2sml: obj2sml.o crc32c.o mesh.o obj.o sml.o stripsearch.o
	$(CXX) $(CXXFLAGS) -o $@ $^
