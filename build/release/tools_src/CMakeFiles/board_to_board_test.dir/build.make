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
include tools_src/CMakeFiles/board_to_board_test.dir/depend.make

# Include the progress variables for this target.
include tools_src/CMakeFiles/board_to_board_test.dir/progress.make

# Include the compile flags for this target's objects.
include tools_src/CMakeFiles/board_to_board_test.dir/flags.make

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o: tools_src/CMakeFiles/board_to_board_test.dir/flags.make
tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o: ../../tools_src/hwtest/board_to_board_test.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test.cpp

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test.cpp > CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.i

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test.cpp -o CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.s

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.requires

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.provides: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/board_to_board_test.dir/build.make tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.provides

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.provides.build: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o: tools_src/CMakeFiles/board_to_board_test.dir/flags.make
tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o: ../../tools_src/hwtest/board_to_board_test_modules.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test_modules.cpp

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test_modules.cpp > CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.i

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/board_to_board_test_modules.cpp -o CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.s

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.requires

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.provides: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/board_to_board_test.dir/build.make tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.provides

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.provides.build: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o: tools_src/CMakeFiles/board_to_board_test.dir/flags.make
tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o: ../../tools_src/dma/dma_handling.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_3)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/dma/dma_handling.cpp

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/dma/dma_handling.cpp > CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.i

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/dma/dma_handling.cpp -o CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.s

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.requires

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.provides: tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/board_to_board_test.dir/build.make tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.provides

tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.provides.build: tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o: tools_src/CMakeFiles/board_to_board_test.dir/flags.make
tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o: ../../tools_src/hwtest/boardtest_modules.cpp
	$(CMAKE_COMMAND) -E cmake_progress_report /mnt/data/005_git/crorc-driver-dma/build/release/CMakeFiles $(CMAKE_PROGRESS_4)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -o CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o -c /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/boardtest_modules.cpp

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.i"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -E /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/boardtest_modules.cpp > CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.i

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.s"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && /usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -S /mnt/data/005_git/crorc-driver-dma/tools_src/hwtest/boardtest_modules.cpp -o CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.s

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.requires:
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.requires

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.provides: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.requires
	$(MAKE) -f tools_src/CMakeFiles/board_to_board_test.dir/build.make tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.provides.build
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.provides

tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.provides.build: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o

# Object files for target board_to_board_test
board_to_board_test_OBJECTS = \
"CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o" \
"CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o" \
"CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o" \
"CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o"

# External object files for target board_to_board_test
board_to_board_test_EXTERNAL_OBJECTS =

tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o
tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o
tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o
tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o
tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/build.make
tools_src/board_to_board_test: librorc.so.1.0.1
tools_src/board_to_board_test: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/board_to_board_test: /opt/packages/pda/7.0.1/lib/libpda.so
tools_src/board_to_board_test: tools_src/CMakeFiles/board_to_board_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable board_to_board_test"
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/board_to_board_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools_src/CMakeFiles/board_to_board_test.dir/build: tools_src/board_to_board_test
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/build

tools_src/CMakeFiles/board_to_board_test.dir/requires: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test.cpp.o.requires
tools_src/CMakeFiles/board_to_board_test.dir/requires: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/board_to_board_test_modules.cpp.o.requires
tools_src/CMakeFiles/board_to_board_test.dir/requires: tools_src/CMakeFiles/board_to_board_test.dir/dma/dma_handling.cpp.o.requires
tools_src/CMakeFiles/board_to_board_test.dir/requires: tools_src/CMakeFiles/board_to_board_test.dir/hwtest/boardtest_modules.cpp.o.requires
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/requires

tools_src/CMakeFiles/board_to_board_test.dir/clean:
	cd /mnt/data/005_git/crorc-driver-dma/build/release/tools_src && $(CMAKE_COMMAND) -P CMakeFiles/board_to_board_test.dir/cmake_clean.cmake
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/clean

tools_src/CMakeFiles/board_to_board_test.dir/depend:
	cd /mnt/data/005_git/crorc-driver-dma/build/release && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/data/005_git/crorc-driver-dma /mnt/data/005_git/crorc-driver-dma/tools_src /mnt/data/005_git/crorc-driver-dma/build/release /mnt/data/005_git/crorc-driver-dma/build/release/tools_src /mnt/data/005_git/crorc-driver-dma/build/release/tools_src/CMakeFiles/board_to_board_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools_src/CMakeFiles/board_to_board_test.dir/depend

