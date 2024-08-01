.PHONY: test clean debug

STB_INCLUDE_PATH = /home/martin/libraries/stb
EXEC = VulkanTest
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH)
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

all: debug

release: CFLAGS  += -O2 -DNDEBUG
release: $(EXEC)

debug: CFLAGS += -g
debug: $(EXEC)

test: $(EXEC)
	./$(EXEC)

clean:
	rm -f $(EXEC)



$(EXEC): main.cpp
	g++ $(CFLAGS) -o $(EXEC) main.cpp $(LDFLAGS)

