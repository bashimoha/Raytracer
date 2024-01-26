CXX = g++
CXXFLAGS = -Wall -Wextra    -std=c++20 -g
LDFLAGS = -lncurses
PROG = raytracer
all: 
	$(CXX) $(CXXFLAGS) -o $(PROG) src/*.cpp 
clean:
	rm -f $(PROG)