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
CMAKE_BINARY_DIR = /mnt/data/005_git/crorc-driver-dma/build/sim_debug

# Include any dependencies generated for this target.
include CMakeFiles/rorc.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/rorc.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/rorc.dir/flags.make

# Object files for target rorc
rorc_OBJECTS =

# External object files for target rorc
rorc_EXTERNAL_OBJECTS = \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/device.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/bar.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/buffer.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/dma_channel.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/link.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/flash.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/sysmon.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/refclk.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/microcontroller.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/sim_bar.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/event_stream.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/event_sanity_checker.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/patterngenerator.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/fastclusterfinder.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/ddl.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/diu.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/siu.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/eventfilter.cpp.o" \
"/mnt/data/005_git/crorc-driver-dma/build/sim_debug/src/CMakeFiles/LIBRORC_OBJECTS.dir/event_generator.cpp.o"

librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/device.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/bar.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/buffer.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/dma_channel.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/link.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/flash.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/sysmon.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/refclk.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/microcontroller.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/sim_bar.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/event_stream.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/event_sanity_checker.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/patterngenerator.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/fastclusterfinder.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/ddl.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/diu.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/siu.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/eventfilter.cpp.o
librorc.so.1.0.1: src/CMakeFiles/LIBRORC_OBJECTS.dir/event_generator.cpp.o
librorc.so.1.0.1: CMakeFiles/rorc.dir/build.make
librorc.so.1.0.1: /opt/packages/pda/7.0.1/lib/libpda.so
librorc.so.1.0.1: CMakeFiles/rorc.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX shared library librorc.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/rorc.dir/link.txt --verbose=$(VERBOSE)
	$(CMAKE_COMMAND) -E cmake_symlink_library librorc.so.1.0.1 librorc.so.0 librorc.so

librorc.so.0: librorc.so.1.0.1

librorc.so: librorc.so.1.0.1

# Rule to build all files generated by this target.
CMakeFiles/rorc.dir/build: librorc.so
.PHONY : CMakeFiles/rorc.dir/build

CMakeFiles/rorc.dir/requires:
.PHONY : CMakeFiles/rorc.dir/requires

CMakeFiles/rorc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/rorc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/rorc.dir/clean

CMakeFiles/rorc.dir/depend:
	cd /mnt/data/005_git/crorc-driver-dma/build/sim_debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma/build/sim_debug /mnt/data/005_git/crorc-driver-dma/build/sim_debug /mnt/data/005_git/crorc-driver-dma/build/sim_debug/CMakeFiles/rorc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/rorc.dir/depend

