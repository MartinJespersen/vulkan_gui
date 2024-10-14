.PHONY: test clean debug

STB_INCLUDE_PATH = /home/martin/libraries/stb
EXEC = VulkanTest
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH) -DTRACY_ENABLE -include types.hpp
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lfreetype

all: debug

release: CFLAGS  += -O2 -DNDEBUG
release: $(EXEC)

debug: CFLAGS += -g -O0
debug: $(EXEC)

test: $(EXEC)
	./$(EXEC)

clean:
	rm -f $(EXEC)



$(EXEC): main.cpp
	g++ $(CFLAGS) -o $(EXEC) main.cpp profiler/TracyClient.cpp $(LDFLAGS)

