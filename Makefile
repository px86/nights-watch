CXX=g++
CXXFLAGS=-Wall -Werror -Wpedantic -std=c++17 -Wno-nonnull
LIBS=

nwatch: nwatch.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o nwatch
