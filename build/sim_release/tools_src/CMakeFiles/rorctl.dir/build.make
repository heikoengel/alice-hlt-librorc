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
CMAKE_BINARY_DIR = /mnt/data/005_git/crorc-driver-dma/build/sim_release

# Include any dependencies generated for this target.
include tools_src/CMakeFiles/rorctl.dir/depend.make

# Include the progress variables for this target.
include tools_src/CMakeFiles/rorctl.dir/progress.make

# Include the compile flags for this target's objects.
include tools_src/CMakeFiles/rorctl.dir/flags.make

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o: tools_src/CMakeFiles/rorctl.dir/flags.make
tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o: ../../tools_src/rorctl/rorctl.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/sim_release/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/rorctl/rorctl.cpp

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/rorctl/rorctl.cpp > CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.i

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/rorctl/rorctl.cpp -o CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.s

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.requires

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.provides: tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/rorctl.dir/build.make tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.provides

tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.provides.build: tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o

# Object files for target rorctl
rorctl_OBJECTS = \
"CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o"

# External object files for target rorctl
rorctl_EXTERNAL_OBJECTS =

tools_src/rorctl: tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o
tools_src/rorctl: tools_src/CMakeFiles/rorctl.dir/build.make
tools_src/rorctl: librorc.so.1.0.1
tools_src/rorctl: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/rorctl: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/rorctl: tools_src/CMakeFiles/rorctl.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable rorctl"
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rorctl.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools_src/CMakeFiles/rorctl.dir/build: tools_src/rorctl
.PHONY : tools_src/CMakeFiles/rorctl.dir/build

tools_src/CMakeFiles/rorctl.dir/requires: tools_src/CMakeFiles/rorctl.dir/rorctl/rorctl.cpp.o.requires
.PHONY : tools_src/CMakeFiles/rorctl.dir/requires

tools_src/CMakeFiles/rorctl.dir/clean:
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src && $(CMAKE_COMMAND) -P CMakeFiles/rorctl.dir/cmake_clean.cmake
.PHONY : tools_src/CMakeFiles/rorctl.dir/clean

tools_src/CMakeFiles/rorctl.dir/depend:
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma/tools_src /mnt/data/005_git/crorc-driver-dma/build/sim_release /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src /mnt/data/005_git/crorc-driver-dma/build/sim_release/tools_src/CMakeFiles/rorctl.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools_src/CMakeFiles/rorctl.dir/depend

