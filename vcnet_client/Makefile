
CXX = g++
SRCS = vcnet.cpp
CFLAGS = --std=c++11 -Wall -g -O1 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf
ifeq ($(shell uname), Darwin)
LDFLAGS += -framework OpenGL
else
LDFLAGS += -lOpenGL
endif

vcnet: $(SRCS)
	$(CXX) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf vcnet *.dSYM x64 .vs *.log *.ttf *.raw

