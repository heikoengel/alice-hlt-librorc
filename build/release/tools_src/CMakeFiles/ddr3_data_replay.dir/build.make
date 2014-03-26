# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/data/005_git/crorc-driver-dma

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/data/005_git/crorc-driver-dma/build/release

# Include any dependencies generated for this target.
include tools_src/CMakeFiles/ddr3_data_replay.dir/depend.make

# Include the progress variables for this target.
include tools_src/CMakeFiles/ddr3_data_replay.dir/progress.make

# Include the compile flags for this target's objects.
include tools_src/CMakeFiles/ddr3_data_replay.dir/flags.make

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o: tools_src/CMakeFiles/ddr3_data_replay.dir/flags.make
tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o: ../../tools_src/linkctrl/ddr3_data_replay.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/linkctrl/ddr3_data_replay.cpp

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/linkctrl/ddr3_data_replay.cpp > CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.i

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/linkctrl/ddr3_data_replay.cpp -o CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.s

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.requires

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.provides: tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/ddr3_data_replay.dir/build.make tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.provides

tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.provides.build: tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o

# Object files for target ddr3_data_replay
ddr3_data_replay_OBJECTS = \
"CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o"

# External object files for target ddr3_data_replay
ddr3_data_replay_EXTERNAL_OBJECTS =

tools_src/ddr3_data_replay: tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o
tools_src/ddr3_data_replay: tools_src/CMakeFiles/ddr3_data_replay.dir/build.make
tools_src/ddr3_data_replay: librorc.so.1.0.1
tools_src/ddr3_data_replay: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/ddr3_data_replay: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/ddr3_data_replay: tools_src/CMakeFiles/ddr3_data_replay.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable ddr3_data_replay"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ddr3_data_replay.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools_src/CMakeFiles/ddr3_data_replay.dir/build: tools_src/ddr3_data_replay
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/build

tools_src/CMakeFiles/ddr3_data_replay.dir/requires: tools_src/CMakeFiles/ddr3_data_replay.dir/linkctrl/ddr3_data_replay.cpp.o.requires
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/requires

tools_src/CMakeFiles/ddr3_data_replay.dir/clean:
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && $(CMAKE_COMMAND) -P CMakeFiles/ddr3_data_replay.dir/cmake_clean.cmake
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/clean

tools_src/CMakeFiles/ddr3_data_replay.dir/depend:
	cd /mnt/data/005_git/crorc-driver-dma/build/release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma/tools_src /mnt/data/005_git/crorc-driver-dma/build/release /mnt/data/005_git/crorc-driver-dma/build/release/tools_src /mnt/data/005_git/crorc-driver-dma/build/release/tools_src/CMakeFiles/ddr3_data_replay.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools_src/CMakeFiles/ddr3_data_replay.dir/depend

