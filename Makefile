.PHONY: test clean debug build profiler run_profiler

STB_INCLUDE_PATH = /home/martin/libraries/stb

PROFILE_DIR = build/profiler
DEBUG_DIR = build/debug

TRACY = $(PROFILE_DIR)/tracy.o
HELPER_HPP = -include base/base.hpp
PROFILER_EXEC = $(PROFILE_DIR)/profiler_exe
PROFILER_LIB = $(PROFILE_DIR)/libprofiler.so
LIB = $(DEBUG_DIR)/entrypoint.so
EXEC = $(DEBUG_DIR)/VulkanTest
ENTRYPOINT  = entrypoint.cpp
MAIN = main.cpp
CXXFLAGS = -Wall -Wextra -Werror -pedantic -Wconversion -Wsign-conversion -Wno-missing-field-initializers -Wno-write-strings -Wno-class-memaccess -maes -msse4
CFLAGS = -std=c++20  $(HELPER_HPP) 
ENTRYPOINT_LDFLAGS = -I. -lglfw -lvulkan -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lfreetype -I$(STB_INCLUDE_PATH) 
EXEC_LDFLAGS = -lglfw -ldl -I./
all: debug

release: CFLAGS  += -O3 -DNDEBUG $(CXXFLAGS)
release: $(EXEC)

debug: CFLAGS += -g $(CXXFLAGS)
debug: $(EXEC)

build: CFLAGS += -g -O3 $(CXXFLAGS)
build: 
	g++ $(CFLAGS) -shared -fPIC -o $(LIB) $(ENTRYPOINT) $(ENTRYPOINT_LDFLAGS)

${TRACY}: profiler_clean
	mkdir -p $(PROFILE_DIR) && g++ -c -fPIC $(CFLAGS) -o $@ profiler/TracyClient.cpp -DTRACY_ENABLE 

profiler: CFLAGS += -O3 -march=native 
profiler: $(PROFILER_EXEC)

run_profiler: profiler
	./$(PROFILER_EXEC)

profiler_clean: 
	rm -f $(PROFILER_EXEC) || rm -f $(PROFILER_LIB)

 	
$(PROFILER_LIB): $(TRACY)
	mkdir -p $(PROFILE_DIR) && g++ $(CFLAGS) -shared -fPIC -o $@ $(ENTRYPOINT) $(TRACY) $(ENTRYPOINT_LDFLAGS) -DTRACY_ENABLE

$(PROFILER_EXEC): $(PROFILER_LIB) 
	mkdir -p $(PROFILE_DIR) && g++ -o $@ $(MAIN) $^ $(CFLAGS) $(ENTRYPOINT_LDFLAGS) -DPROFILING_ENABLE

clean:
	rm -f $(EXEC) || rm -f $(LIB)

$(LIB): clean
	mkdir -p $(DEBUG_DIR) && g++ $(CFLAGS) -shared -fPIC -o $@ $(ENTRYPOINT) $(ENTRYPOINT_LDFLAGS)

$(EXEC): $(LIB)
	mkdir -p $(DEBUG_DIR) && g++ -o $@ $(MAIN) $(CFLAGS) $(EXEC_LDFLAGS)

