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
include sis/factor/CMakeFiles/factor.dir/depend.make

# Include the progress variables for this target.
include sis/factor/CMakeFiles/factor.dir/progress.make

# Include the compile flags for this target's objects.
include sis/factor/CMakeFiles/factor.dir/flags.make

sis/factor/CMakeFiles/factor.dir/alg_ft.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/alg_ft.c.o: ../sis/factor/alg_ft.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object sis/factor/CMakeFiles/factor.dir/alg_ft.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/alg_ft.c.o   -c /home/matteo/Dropbox/sis/sis/factor/alg_ft.c

sis/factor/CMakeFiles/factor.dir/alg_ft.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/alg_ft.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/alg_ft.c > CMakeFiles/factor.dir/alg_ft.c.i

sis/factor/CMakeFiles/factor.dir/alg_ft.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/alg_ft.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/alg_ft.c -o CMakeFiles/factor.dir/alg_ft.c.s

sis/factor/CMakeFiles/factor.dir/com_ft.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/com_ft.c.o: ../sis/factor/com_ft.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object sis/factor/CMakeFiles/factor.dir/com_ft.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/com_ft.c.o   -c /home/matteo/Dropbox/sis/sis/factor/com_ft.c

sis/factor/CMakeFiles/factor.dir/com_ft.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/com_ft.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/com_ft.c > CMakeFiles/factor.dir/com_ft.c.i

sis/factor/CMakeFiles/factor.dir/com_ft.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/com_ft.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/com_ft.c -o CMakeFiles/factor.dir/com_ft.c.s

sis/factor/CMakeFiles/factor.dir/elim.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/elim.c.o: ../sis/factor/elim.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object sis/factor/CMakeFiles/factor.dir/elim.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/elim.c.o   -c /home/matteo/Dropbox/sis/sis/factor/elim.c

sis/factor/CMakeFiles/factor.dir/elim.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/elim.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/elim.c > CMakeFiles/factor.dir/elim.c.i

sis/factor/CMakeFiles/factor.dir/elim.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/elim.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/elim.c -o CMakeFiles/factor.dir/elim.c.s

sis/factor/CMakeFiles/factor.dir/factor.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/factor.c.o: ../sis/factor/factor.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object sis/factor/CMakeFiles/factor.dir/factor.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/factor.c.o   -c /home/matteo/Dropbox/sis/sis/factor/factor.c

sis/factor/CMakeFiles/factor.dir/factor.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/factor.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/factor.c > CMakeFiles/factor.dir/factor.c.i

sis/factor/CMakeFiles/factor.dir/factor.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/factor.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/factor.c -o CMakeFiles/factor.dir/factor.c.s

sis/factor/CMakeFiles/factor.dir/ft_print.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/ft_print.c.o: ../sis/factor/ft_print.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object sis/factor/CMakeFiles/factor.dir/ft_print.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/ft_print.c.o   -c /home/matteo/Dropbox/sis/sis/factor/ft_print.c

sis/factor/CMakeFiles/factor.dir/ft_print.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/ft_print.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/ft_print.c > CMakeFiles/factor.dir/ft_print.c.i

sis/factor/CMakeFiles/factor.dir/ft_print.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/ft_print.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/ft_print.c -o CMakeFiles/factor.dir/ft_print.c.s

sis/factor/CMakeFiles/factor.dir/ft_util.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/ft_util.c.o: ../sis/factor/ft_util.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object sis/factor/CMakeFiles/factor.dir/ft_util.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/ft_util.c.o   -c /home/matteo/Dropbox/sis/sis/factor/ft_util.c

sis/factor/CMakeFiles/factor.dir/ft_util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/ft_util.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/ft_util.c > CMakeFiles/factor.dir/ft_util.c.i

sis/factor/CMakeFiles/factor.dir/ft_util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/ft_util.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/ft_util.c -o CMakeFiles/factor.dir/ft_util.c.s

sis/factor/CMakeFiles/factor.dir/ft_value.c.o: sis/factor/CMakeFiles/factor.dir/flags.make
sis/factor/CMakeFiles/factor.dir/ft_value.c.o: ../sis/factor/ft_value.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object sis/factor/CMakeFiles/factor.dir/ft_value.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/factor.dir/ft_value.c.o   -c /home/matteo/Dropbox/sis/sis/factor/ft_value.c

sis/factor/CMakeFiles/factor.dir/ft_value.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/factor.dir/ft_value.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/factor/ft_value.c > CMakeFiles/factor.dir/ft_value.c.i

sis/factor/CMakeFiles/factor.dir/ft_value.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/factor.dir/ft_value.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/factor/ft_value.c -o CMakeFiles/factor.dir/ft_value.c.s

# Object files for target factor
factor_OBJECTS = \
"CMakeFiles/factor.dir/alg_ft.c.o" \
"CMakeFiles/factor.dir/com_ft.c.o" \
"CMakeFiles/factor.dir/elim.c.o" \
"CMakeFiles/factor.dir/factor.c.o" \
"CMakeFiles/factor.dir/ft_print.c.o" \
"CMakeFiles/factor.dir/ft_util.c.o" \
"CMakeFiles/factor.dir/ft_value.c.o"

# External object files for target factor
factor_EXTERNAL_OBJECTS =

sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/alg_ft.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/com_ft.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/elim.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/factor.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/ft_print.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/ft_util.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/ft_value.c.o
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/build.make
sis/factor/libfactor.a: sis/factor/CMakeFiles/factor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking C static library libfactor.a"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && $(CMAKE_COMMAND) -P CMakeFiles/factor.dir/cmake_clean_target.cmake
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/factor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
sis/factor/CMakeFiles/factor.dir/build: sis/factor/libfactor.a

.PHONY : sis/factor/CMakeFiles/factor.dir/build

sis/factor/CMakeFiles/factor.dir/clean:
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor && $(CMAKE_COMMAND) -P CMakeFiles/factor.dir/cmake_clean.cmake
.PHONY : sis/factor/CMakeFiles/factor.dir/clean

sis/factor/CMakeFiles/factor.dir/depend:
	cd /home/matteo/Dropbox/sis/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matteo/Dropbox/sis /home/matteo/Dropbox/sis/sis/factor /home/matteo/Dropbox/sis/cmake-build-debug /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor /home/matteo/Dropbox/sis/cmake-build-debug/sis/factor/CMakeFiles/factor.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sis/factor/CMakeFiles/factor.dir/depend
