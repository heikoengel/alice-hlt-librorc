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
include fli/CMakeFiles/FLI_OBJECTS.dir/depend.make

# Include the progress variables for this target.
include fli/CMakeFiles/FLI_OBJECTS.dir/progress.make

# Include the compile flags for this target's objects.
include fli/CMakeFiles/FLI_OBJECTS.dir/flags.make

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o: fli/CMakeFiles/FLI_OBJECTS.dir/flags.make
fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o: ../../fli/flisock.c
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/fli && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/FLI_OBJECTS.dir/flisock.c.o   -c /mnt/data/005_git/crorc-driver-dma/fli/flisock.c

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/FLI_OBJECTS.dir/flisock.c.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/fli && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/fli/flisock.c > CMakeFiles/FLI_OBJECTS.dir/flisock.c.i

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/FLI_OBJECTS.dir/flisock.c.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/fli && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/fli/flisock.c -o CMakeFiles/FLI_OBJECTS.dir/flisock.c.s

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.requires:
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.requires

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.provides: fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.requires
	$(MAKE) -f fli/CMakeFiles/FLI_OBJECTS.dir/build.make fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.provides.build
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.provides

fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.provides.build: fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o

FLI_OBJECTS: fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o
FLI_OBJECTS: fli/CMakeFiles/FLI_OBJECTS.dir/build.make
.PHONY : FLI_OBJECTS

# Rule to build all files generated by this target.
fli/CMakeFiles/FLI_OBJECTS.dir/build: FLI_OBJECTS
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/build

fli/CMakeFiles/FLI_OBJECTS.dir/requires: fli/CMakeFiles/FLI_OBJECTS.dir/flisock.c.o.requires
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/requires

fli/CMakeFiles/FLI_OBJECTS.dir/clean:
	cd /mnt/data/005_git/crorc-driver-dma/build/release/fli && $(CMAKE_COMMAND) -P CMakeFiles/FLI_OBJECTS.dir/cmake_clean.cmake
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/clean

fli/CMakeFiles/FLI_OBJECTS.dir/depend:
	cd /mnt/data/005_git/crorc-driver-dma/build/release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma/fli /mnt/data/005_git/crorc-driver-dma/build/release /mnt/data/005_git/crorc-driver-dma/build/release/fli /mnt/data/005_git/crorc-driver-dma/build/release/fli/CMakeFiles/FLI_OBJECTS.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : fli/CMakeFiles/FLI_OBJECTS.dir/depend

