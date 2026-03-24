CXX = g++
CFLAGS = -DNATIVE_TEST -DSIMULATOR -I../include $(shell /opt/homebrew/bin/sdl2-config --cflags) -std=c++17 -O2
LDFLAGS = $(shell /opt/homebrew/bin/sdl2-config --libs) -lSDL2_ttf

GAME_SRCS = ../src/Game/LunarPhysics.cpp ../src/Game/LunarTerrain.cpp ../src/Game/LunarState.cpp ../src/Game/LunarRenderer.cpp ../src/Game/LunarScoreboard.cpp
SRCS = sim_main.cpp $(GAME_SRCS)
TARGET = lunarlander_sim

$(TARGET): $(SRCS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
