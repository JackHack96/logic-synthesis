# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_COMMAND = /home/matteo/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/182.4505.18/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/matteo/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/182.4505.18/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/matteo/Dropbox/sis

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/matteo/Dropbox/sis/cmake-build-debug

# Include any dependencies generated for this target.
include sis/minimize/CMakeFiles/minimize.dir/depend.make

# Include the progress variables for this target.
include sis/minimize/CMakeFiles/minimize.dir/progress.make

# Include the compile flags for this target's objects.
include sis/minimize/CMakeFiles/minimize.dir/flags.make

sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.o: sis/minimize/CMakeFiles/minimize.dir/flags.make
sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.o: ../sis/minimize/dcsimp.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/minimize.dir/dcsimp.c.o   -c /home/matteo/Dropbox/sis/sis/minimize/dcsimp.c

sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/minimize.dir/dcsimp.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/minimize/dcsimp.c > CMakeFiles/minimize.dir/dcsimp.c.i

sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/minimize.dir/dcsimp.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/minimize/dcsimp.c -o CMakeFiles/minimize.dir/dcsimp.c.s

sis/minimize/CMakeFiles/minimize.dir/minimize.c.o: sis/minimize/CMakeFiles/minimize.dir/flags.make
sis/minimize/CMakeFiles/minimize.dir/minimize.c.o: ../sis/minimize/minimize.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object sis/minimize/CMakeFiles/minimize.dir/minimize.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/minimize.dir/minimize.c.o   -c /home/matteo/Dropbox/sis/sis/minimize/minimize.c

sis/minimize/CMakeFiles/minimize.dir/minimize.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/minimize.dir/minimize.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/minimize/minimize.c > CMakeFiles/minimize.dir/minimize.c.i

sis/minimize/CMakeFiles/minimize.dir/minimize.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/minimize.dir/minimize.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/minimize/minimize.c -o CMakeFiles/minimize.dir/minimize.c.s

sis/minimize/CMakeFiles/minimize.dir/ros.c.o: sis/minimize/CMakeFiles/minimize.dir/flags.make
sis/minimize/CMakeFiles/minimize.dir/ros.c.o: ../sis/minimize/ros.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object sis/minimize/CMakeFiles/minimize.dir/ros.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/minimize.dir/ros.c.o   -c /home/matteo/Dropbox/sis/sis/minimize/ros.c

sis/minimize/CMakeFiles/minimize.dir/ros.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/minimize.dir/ros.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/minimize/ros.c > CMakeFiles/minimize.dir/ros.c.i

sis/minimize/CMakeFiles/minimize.dir/ros.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/minimize.dir/ros.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/minimize/ros.c -o CMakeFiles/minimize.dir/ros.c.s

# Object files for target minimize
minimize_OBJECTS = \
"CMakeFiles/minimize.dir/dcsimp.c.o" \
"CMakeFiles/minimize.dir/minimize.c.o" \
"CMakeFiles/minimize.dir/ros.c.o"

# External object files for target minimize
minimize_EXTERNAL_OBJECTS =

sis/minimize/libminimize.a: sis/minimize/CMakeFiles/minimize.dir/dcsimp.c.o
sis/minimize/libminimize.a: sis/minimize/CMakeFiles/minimize.dir/minimize.c.o
sis/minimize/libminimize.a: sis/minimize/CMakeFiles/minimize.dir/ros.c.o
sis/minimize/libminimize.a: sis/minimize/CMakeFiles/minimize.dir/build.make
sis/minimize/libminimize.a: sis/minimize/CMakeFiles/minimize.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C static library libminimize.a"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && $(CMAKE_COMMAND) -P CMakeFiles/minimize.dir/cmake_clean_target.cmake
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/minimize.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
sis/minimize/CMakeFiles/minimize.dir/build: sis/minimize/libminimize.a

.PHONY : sis/minimize/CMakeFiles/minimize.dir/build

sis/minimize/CMakeFiles/minimize.dir/clean:
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize && $(CMAKE_COMMAND) -P CMakeFiles/minimize.dir/cmake_clean.cmake
.PHONY : sis/minimize/CMakeFiles/minimize.dir/clean

sis/minimize/CMakeFiles/minimize.dir/depend:
	cd /home/matteo/Dropbox/sis/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matteo/Dropbox/sis /home/matteo/Dropbox/sis/sis/minimize /home/matteo/Dropbox/sis/cmake-build-debug /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize /home/matteo/Dropbox/sis/cmake-build-debug/sis/minimize/CMakeFiles/minimize.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sis/minimize/CMakeFiles/minimize.dir/depend

