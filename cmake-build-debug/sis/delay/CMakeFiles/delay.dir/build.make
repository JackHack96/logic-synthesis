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
include sis/delay/CMakeFiles/delay.dir/depend.make

# Include the progress variables for this target.
include sis/delay/CMakeFiles/delay.dir/progress.make

# Include the compile flags for this target's objects.
include sis/delay/CMakeFiles/delay.dir/flags.make

sis/delay/CMakeFiles/delay.dir/com_delay.c.o: sis/delay/CMakeFiles/delay.dir/flags.make
sis/delay/CMakeFiles/delay.dir/com_delay.c.o: ../sis/delay/com_delay.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object sis/delay/CMakeFiles/delay.dir/com_delay.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/delay.dir/com_delay.c.o   -c /home/matteo/Dropbox/sis/sis/delay/com_delay.c

sis/delay/CMakeFiles/delay.dir/com_delay.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/delay.dir/com_delay.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/delay/com_delay.c > CMakeFiles/delay.dir/com_delay.c.i

sis/delay/CMakeFiles/delay.dir/com_delay.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/delay.dir/com_delay.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/delay/com_delay.c -o CMakeFiles/delay.dir/com_delay.c.s

sis/delay/CMakeFiles/delay.dir/delay.c.o: sis/delay/CMakeFiles/delay.dir/flags.make
sis/delay/CMakeFiles/delay.dir/delay.c.o: ../sis/delay/delay.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object sis/delay/CMakeFiles/delay.dir/delay.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/delay.dir/delay.c.o   -c /home/matteo/Dropbox/sis/sis/delay/delay.c

sis/delay/CMakeFiles/delay.dir/delay.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/delay.dir/delay.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/delay/delay.c > CMakeFiles/delay.dir/delay.c.i

sis/delay/CMakeFiles/delay.dir/delay.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/delay.dir/delay.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/delay/delay.c -o CMakeFiles/delay.dir/delay.c.s

sis/delay/CMakeFiles/delay.dir/mapdelay.c.o: sis/delay/CMakeFiles/delay.dir/flags.make
sis/delay/CMakeFiles/delay.dir/mapdelay.c.o: ../sis/delay/mapdelay.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object sis/delay/CMakeFiles/delay.dir/mapdelay.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/delay.dir/mapdelay.c.o   -c /home/matteo/Dropbox/sis/sis/delay/mapdelay.c

sis/delay/CMakeFiles/delay.dir/mapdelay.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/delay.dir/mapdelay.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/delay/mapdelay.c > CMakeFiles/delay.dir/mapdelay.c.i

sis/delay/CMakeFiles/delay.dir/mapdelay.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/delay.dir/mapdelay.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/delay/mapdelay.c -o CMakeFiles/delay.dir/mapdelay.c.s

sis/delay/CMakeFiles/delay.dir/tdc_delay.c.o: sis/delay/CMakeFiles/delay.dir/flags.make
sis/delay/CMakeFiles/delay.dir/tdc_delay.c.o: ../sis/delay/tdc_delay.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object sis/delay/CMakeFiles/delay.dir/tdc_delay.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/delay.dir/tdc_delay.c.o   -c /home/matteo/Dropbox/sis/sis/delay/tdc_delay.c

sis/delay/CMakeFiles/delay.dir/tdc_delay.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/delay.dir/tdc_delay.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/delay/tdc_delay.c > CMakeFiles/delay.dir/tdc_delay.c.i

sis/delay/CMakeFiles/delay.dir/tdc_delay.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/delay.dir/tdc_delay.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/delay/tdc_delay.c -o CMakeFiles/delay.dir/tdc_delay.c.s

# Object files for target delay
delay_OBJECTS = \
"CMakeFiles/delay.dir/com_delay.c.o" \
"CMakeFiles/delay.dir/delay.c.o" \
"CMakeFiles/delay.dir/mapdelay.c.o" \
"CMakeFiles/delay.dir/tdc_delay.c.o"

# External object files for target delay
delay_EXTERNAL_OBJECTS =

sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/com_delay.c.o
sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/delay.c.o
sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/mapdelay.c.o
sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/tdc_delay.c.o
sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/build.make
sis/delay/libdelay.a: sis/delay/CMakeFiles/delay.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Linking C static library libdelay.a"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && $(CMAKE_COMMAND) -P CMakeFiles/delay.dir/cmake_clean_target.cmake
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/delay.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
sis/delay/CMakeFiles/delay.dir/build: sis/delay/libdelay.a

.PHONY : sis/delay/CMakeFiles/delay.dir/build

sis/delay/CMakeFiles/delay.dir/clean:
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay && $(CMAKE_COMMAND) -P CMakeFiles/delay.dir/cmake_clean.cmake
.PHONY : sis/delay/CMakeFiles/delay.dir/clean

sis/delay/CMakeFiles/delay.dir/depend:
	cd /home/matteo/Dropbox/sis/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matteo/Dropbox/sis /home/matteo/Dropbox/sis/sis/delay /home/matteo/Dropbox/sis/cmake-build-debug /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay /home/matteo/Dropbox/sis/cmake-build-debug/sis/delay/CMakeFiles/delay.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sis/delay/CMakeFiles/delay.dir/depend

