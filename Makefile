.PHONY: test clean debug build

STB_INCLUDE_PATH = /home/martin/libraries/stb
EXEC = VulkanTest
LIB = entrypoint.so
ENTRYPOINT  = entrypoint.cpp
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH) -include types.hpp -fno-gnu-unique
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lfreetype

all: debug

release: CFLAGS  += -O2 -DNDEBUG
release: $(EXEC)

debug: CFLAGS += -g -O0
debug: $(EXEC)

build: CFLAGS += -g -O0
build: 
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) profiler/TracyClient.cpp $(LDFLAGS) 

test: $(EXEC)
	./$(EXEC)

clean:
	rm -f $(EXEC) && rm -f $(LIB)

$(LIB): main.cpp profiler/TracyClient.cpp
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) profiler/TracyClient.cpp $(LDFLAGS) 

$(EXEC): $(LIB)
	g++ -o $(EXEC) main.cpp profiler/TracyClient.cpp $(CFLAGS) $(LDFLAGS) 

