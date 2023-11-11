
CXX = g++
SRCS = $(wildcard *.cpp)
CFLAGS = --std=c++11 -Wall -g -O1 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`
ifeq ($(shell uname), Darwin)
LDFLAGS += -framework OpenGL
else
LDFLAGS += -lOpenGL
endif

vcnet_client: $(SRCS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf vcnet_client *.dSYM x64 .vs *.log *.raw
	rm -rf android-project/build/
	rm -rf android-project/.gradle/

