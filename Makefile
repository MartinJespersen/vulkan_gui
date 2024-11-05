.PHONY: test clean debug build profiler run_profiler

STB_INCLUDE_PATH = /home/martin/libraries/stb

PROFILE_DIR = build/profiler
DEBUG_DIR = build/debug

HELPER_HPP = -include constants.hpp -include types.hpp -include error.hpp
PROFILER_EXEC = $(PROFILE_DIR)/profiler_exe
PROFILER_LIB = $(PROFILE_DIR)/libprofiler.so
LIB = $(DEBUG_DIR)/entrypoint.so
EXEC = $(DEBUG_DIR)/VulkanTest
ENTRYPOINT  = entrypoint.cpp
CXXFLAGS = -Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion 
# -Wshadow
CFLAGS = -std=c++20 -I$(STB_INCLUDE_PATH) $(HELPER_HPP) $(CXXFLAGS)
LDFLAGS = -lglfw -lvulkan -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lfreetype

all: debug

release: CFLAGS  += -O2 -DNDEBUG
release: $(EXEC)

debug: CFLAGS += -g -O0
debug: $(EXEC)

build: CFLAGS += -g -O0
build: 
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) $(LDFLAGS)

profiler: CFLAGS += -g -O0 
profiler: $(PROFILER_EXEC)

run_profiler: profiler
	./$(PROFILER_EXEC)

profiler_clean: 
	rm -f $(PROFILER_EXEC) || rm -f $(PROFILER_LIB)
 	
$(PROFILER_LIB): profiler_clean
	mkdir -p $(PROFILE_DIR) && g++ $(CFLAGS) -shared -fPIC -o $(PROFILER_LIB) $(ENTRYPOINT) profiler/TracyClient.cpp $(LDFLAGS) -DTRACY_ENABLE 

$(PROFILER_EXEC): $(PROFILER_LIB) 
	mkdir -p $(PROFILE_DIR) && g++ -o $(PROFILER_EXEC) main.cpp profiler/TracyClient.cpp $(PROFILER_LIB) $(CFLAGS) $(LDFLAGS) -DPROFILING_ENABLE

clean:
	rm -f $(EXEC) || rm -f $(LIB)

$(LIB): clean
	mkdir -p $(DEBUG_DIR) && g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) $(LDFLAGS)

$(EXEC): $(LIB)
	mkdir -p $(DEBUG_DIR) && g++ -o $(EXEC) main.cpp $(CFLAGS) -lglfw -ldl

