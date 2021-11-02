all: stl2sml sml2stl

clean:
	rm -f *.o stl2sml sml2stl

crc32c.o: crc32c.c
	$(CC) -O2 -c -o $@ $^

stl2sml.o: stl2sml.cpp
	$(CXX) -std=c++17 -Wall -g -Og -c -o $@ $^
mesh.o: mesh.cpp
	$(CXX) -std=c++17 -Wall -g -Og -c -o $@ $^
stl.o: stl.cpp
	$(CXX) -std=c++17 -Wall -g -Og -c -o $@ $^
sml.o: sml.cpp
	$(CXX) -std=c++17 -Wall -g -Og -c -o $@ $^

stl2sml: stl2sml.o crc32c.o mesh.o stl.o sml.o
	$(CXX) -g -Og -o $@ $^
sml2stl: sml2stl.o crc32c.o mesh.o stl.o sml.o
	$(CXX) -g -Og -o $@ $^