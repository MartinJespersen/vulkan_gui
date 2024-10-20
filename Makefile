.PHONY: test clean debug build profiler profiler_clean

STB_INCLUDE_PATH = /home/martin/libraries/stb
EXEC = VulkanTest
DIR = build
PROFILER_EXEC = $(DIR)/profiler_exe
PROFILER_LIB = $(DIR)/libprofiler.so
LIB = entrypoint.so
ENTRYPOINT  = entrypoint.cpp
CFLAGS = -std=c++17 -I$(STB_INCLUDE_PATH) -include types.hpp -Wall -Werror -Werror=unused-variable
LDFLAGS = -lglfw -lvulkan -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lfreetype

all: debug

release: CFLAGS  += -O2 -DNDEBUG
release: $(EXEC)

debug: CFLAGS += -g -O0
debug: $(EXEC)

build: CFLAGS += -g -O0
build: 
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) profiler/TracyClient.cpp $(LDFLAGS)

profiler: CFLAGS += -g -O0 
profiler: $(PROFILER_EXEC)

run_profiler: profiler
	./$(PROFILER_EXEC)

profiler_clean: 
	rm -f $(PROFILER_EXEC) || rm -f $(PROFILER_LIB)
 	
$(PROFILER_LIB): profiler_clean
	g++ $(CFLAGS) -shared -fPIC -o $(PROFILER_LIB) $(ENTRYPOINT) profiler/TracyClient.cpp $(LDFLAGS) -DTRACY_ENABLE 

$(PROFILER_EXEC): $(PROFILER_LIB) 
	g++ -o $(PROFILER_EXEC) main.cpp profiler/TracyClient.cpp $(PROFILER_LIB) $(CFLAGS) $(LDFLAGS) -DPROFILING_ENABLE

clean:
	rm -f $(EXEC) || rm -f $(LIB)

$(LIB): clean
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) $(LDFLAGS)

$(EXEC): $(LIB)
	g++ -o $(EXEC) main.cpp $(CFLAGS) -lglfw -ldl

