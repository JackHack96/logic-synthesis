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
include sis/io/CMakeFiles/io.dir/depend.make

# Include the progress variables for this target.
include sis/io/CMakeFiles/io.dir/progress.make

# Include the compile flags for this target's objects.
include sis/io/CMakeFiles/io.dir/flags.make

../sis/io/read_eqn.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating read_eqn.c"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/bison --output=/home/matteo/Dropbox/sis/sis/io/read_eqn.c -d /home/matteo/Dropbox/sis/sis/io/read_eqn.y

../sis/io/eqnlex.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating eqnlex.c"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/flex --outfile=/home/matteo/Dropbox/sis/sis/io/eqnlex.c /home/matteo/Dropbox/sis/sis/io/eqnlex.l

sis/io/CMakeFiles/io.dir/com_io.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/com_io.c.o: ../sis/io/com_io.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object sis/io/CMakeFiles/io.dir/com_io.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/com_io.c.o   -c /home/matteo/Dropbox/sis/sis/io/com_io.c

sis/io/CMakeFiles/io.dir/com_io.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/com_io.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/com_io.c > CMakeFiles/io.dir/com_io.c.i

sis/io/CMakeFiles/io.dir/com_io.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/com_io.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/com_io.c -o CMakeFiles/io.dir/com_io.c.s

sis/io/CMakeFiles/io.dir/plot_blif.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/plot_blif.c.o: ../sis/io/plot_blif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object sis/io/CMakeFiles/io.dir/plot_blif.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/plot_blif.c.o   -c /home/matteo/Dropbox/sis/sis/io/plot_blif.c

sis/io/CMakeFiles/io.dir/plot_blif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/plot_blif.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/plot_blif.c > CMakeFiles/io.dir/plot_blif.c.i

sis/io/CMakeFiles/io.dir/plot_blif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/plot_blif.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/plot_blif.c -o CMakeFiles/io.dir/plot_blif.c.s

sis/io/CMakeFiles/io.dir/read_blif.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_blif.c.o: ../sis/io/read_blif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object sis/io/CMakeFiles/io.dir/read_blif.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_blif.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_blif.c

sis/io/CMakeFiles/io.dir/read_blif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_blif.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_blif.c > CMakeFiles/io.dir/read_blif.c.i

sis/io/CMakeFiles/io.dir/read_blif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_blif.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_blif.c -o CMakeFiles/io.dir/read_blif.c.s

sis/io/CMakeFiles/io.dir/read_kiss.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_kiss.c.o: ../sis/io/read_kiss.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object sis/io/CMakeFiles/io.dir/read_kiss.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_kiss.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_kiss.c

sis/io/CMakeFiles/io.dir/read_kiss.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_kiss.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_kiss.c > CMakeFiles/io.dir/read_kiss.c.i

sis/io/CMakeFiles/io.dir/read_kiss.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_kiss.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_kiss.c -o CMakeFiles/io.dir/read_kiss.c.s

sis/io/CMakeFiles/io.dir/read_pla.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_pla.c.o: ../sis/io/read_pla.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object sis/io/CMakeFiles/io.dir/read_pla.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_pla.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_pla.c

sis/io/CMakeFiles/io.dir/read_pla.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_pla.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_pla.c > CMakeFiles/io.dir/read_pla.c.i

sis/io/CMakeFiles/io.dir/read_pla.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_pla.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_pla.c -o CMakeFiles/io.dir/read_pla.c.s

sis/io/CMakeFiles/io.dir/read_slif.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_slif.c.o: ../sis/io/read_slif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object sis/io/CMakeFiles/io.dir/read_slif.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_slif.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_slif.c

sis/io/CMakeFiles/io.dir/read_slif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_slif.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_slif.c > CMakeFiles/io.dir/read_slif.c.i

sis/io/CMakeFiles/io.dir/read_slif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_slif.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_slif.c -o CMakeFiles/io.dir/read_slif.c.s

sis/io/CMakeFiles/io.dir/read_util.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_util.c.o: ../sis/io/read_util.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object sis/io/CMakeFiles/io.dir/read_util.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_util.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_util.c

sis/io/CMakeFiles/io.dir/read_util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_util.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_util.c > CMakeFiles/io.dir/read_util.c.i

sis/io/CMakeFiles/io.dir/read_util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_util.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_util.c -o CMakeFiles/io.dir/read_util.c.s

sis/io/CMakeFiles/io.dir/write_bdnet.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_bdnet.c.o: ../sis/io/write_bdnet.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object sis/io/CMakeFiles/io.dir/write_bdnet.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_bdnet.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_bdnet.c

sis/io/CMakeFiles/io.dir/write_bdnet.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_bdnet.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_bdnet.c > CMakeFiles/io.dir/write_bdnet.c.i

sis/io/CMakeFiles/io.dir/write_bdnet.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_bdnet.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_bdnet.c -o CMakeFiles/io.dir/write_bdnet.c.s

sis/io/CMakeFiles/io.dir/write_blif.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_blif.c.o: ../sis/io/write_blif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object sis/io/CMakeFiles/io.dir/write_blif.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_blif.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_blif.c

sis/io/CMakeFiles/io.dir/write_blif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_blif.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_blif.c > CMakeFiles/io.dir/write_blif.c.i

sis/io/CMakeFiles/io.dir/write_blif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_blif.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_blif.c -o CMakeFiles/io.dir/write_blif.c.s

sis/io/CMakeFiles/io.dir/write_eqn.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_eqn.c.o: ../sis/io/write_eqn.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object sis/io/CMakeFiles/io.dir/write_eqn.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_eqn.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_eqn.c

sis/io/CMakeFiles/io.dir/write_eqn.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_eqn.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_eqn.c > CMakeFiles/io.dir/write_eqn.c.i

sis/io/CMakeFiles/io.dir/write_eqn.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_eqn.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_eqn.c -o CMakeFiles/io.dir/write_eqn.c.s

sis/io/CMakeFiles/io.dir/write_kiss.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_kiss.c.o: ../sis/io/write_kiss.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building C object sis/io/CMakeFiles/io.dir/write_kiss.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_kiss.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_kiss.c

sis/io/CMakeFiles/io.dir/write_kiss.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_kiss.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_kiss.c > CMakeFiles/io.dir/write_kiss.c.i

sis/io/CMakeFiles/io.dir/write_kiss.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_kiss.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_kiss.c -o CMakeFiles/io.dir/write_kiss.c.s

sis/io/CMakeFiles/io.dir/write_pla.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_pla.c.o: ../sis/io/write_pla.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building C object sis/io/CMakeFiles/io.dir/write_pla.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_pla.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_pla.c

sis/io/CMakeFiles/io.dir/write_pla.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_pla.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_pla.c > CMakeFiles/io.dir/write_pla.c.i

sis/io/CMakeFiles/io.dir/write_pla.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_pla.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_pla.c -o CMakeFiles/io.dir/write_pla.c.s

sis/io/CMakeFiles/io.dir/write_slif.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_slif.c.o: ../sis/io/write_slif.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Building C object sis/io/CMakeFiles/io.dir/write_slif.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_slif.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_slif.c

sis/io/CMakeFiles/io.dir/write_slif.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_slif.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_slif.c > CMakeFiles/io.dir/write_slif.c.i

sis/io/CMakeFiles/io.dir/write_slif.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_slif.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_slif.c -o CMakeFiles/io.dir/write_slif.c.s

sis/io/CMakeFiles/io.dir/write_util.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/write_util.c.o: ../sis/io/write_util.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_16) "Building C object sis/io/CMakeFiles/io.dir/write_util.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/write_util.c.o   -c /home/matteo/Dropbox/sis/sis/io/write_util.c

sis/io/CMakeFiles/io.dir/write_util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/write_util.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/write_util.c > CMakeFiles/io.dir/write_util.c.i

sis/io/CMakeFiles/io.dir/write_util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/write_util.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/write_util.c -o CMakeFiles/io.dir/write_util.c.s

sis/io/CMakeFiles/io.dir/read_eqn.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/read_eqn.c.o: ../sis/io/read_eqn.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_17) "Building C object sis/io/CMakeFiles/io.dir/read_eqn.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/read_eqn.c.o   -c /home/matteo/Dropbox/sis/sis/io/read_eqn.c

sis/io/CMakeFiles/io.dir/read_eqn.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/read_eqn.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/read_eqn.c > CMakeFiles/io.dir/read_eqn.c.i

sis/io/CMakeFiles/io.dir/read_eqn.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/read_eqn.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/read_eqn.c -o CMakeFiles/io.dir/read_eqn.c.s

sis/io/CMakeFiles/io.dir/eqnlex.c.o: sis/io/CMakeFiles/io.dir/flags.make
sis/io/CMakeFiles/io.dir/eqnlex.c.o: ../sis/io/eqnlex.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_18) "Building C object sis/io/CMakeFiles/io.dir/eqnlex.c.o"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/io.dir/eqnlex.c.o   -c /home/matteo/Dropbox/sis/sis/io/eqnlex.c

sis/io/CMakeFiles/io.dir/eqnlex.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/io.dir/eqnlex.c.i"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/matteo/Dropbox/sis/sis/io/eqnlex.c > CMakeFiles/io.dir/eqnlex.c.i

sis/io/CMakeFiles/io.dir/eqnlex.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/io.dir/eqnlex.c.s"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/matteo/Dropbox/sis/sis/io/eqnlex.c -o CMakeFiles/io.dir/eqnlex.c.s

# Object files for target io
io_OBJECTS = \
"CMakeFiles/io.dir/com_io.c.o" \
"CMakeFiles/io.dir/plot_blif.c.o" \
"CMakeFiles/io.dir/read_blif.c.o" \
"CMakeFiles/io.dir/read_kiss.c.o" \
"CMakeFiles/io.dir/read_pla.c.o" \
"CMakeFiles/io.dir/read_slif.c.o" \
"CMakeFiles/io.dir/read_util.c.o" \
"CMakeFiles/io.dir/write_bdnet.c.o" \
"CMakeFiles/io.dir/write_blif.c.o" \
"CMakeFiles/io.dir/write_eqn.c.o" \
"CMakeFiles/io.dir/write_kiss.c.o" \
"CMakeFiles/io.dir/write_pla.c.o" \
"CMakeFiles/io.dir/write_slif.c.o" \
"CMakeFiles/io.dir/write_util.c.o" \
"CMakeFiles/io.dir/read_eqn.c.o" \
"CMakeFiles/io.dir/eqnlex.c.o"

# External object files for target io
io_EXTERNAL_OBJECTS =

sis/io/libio.a: sis/io/CMakeFiles/io.dir/com_io.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/plot_blif.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_blif.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_kiss.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_pla.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_slif.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_util.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_bdnet.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_blif.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_eqn.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_kiss.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_pla.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_slif.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/write_util.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/read_eqn.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/eqnlex.c.o
sis/io/libio.a: sis/io/CMakeFiles/io.dir/build.make
sis/io/libio.a: sis/io/CMakeFiles/io.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/matteo/Dropbox/sis/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_19) "Linking C static library libio.a"
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && $(CMAKE_COMMAND) -P CMakeFiles/io.dir/cmake_clean_target.cmake
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/io.dir/link.txt --verbose=$(VERBOSE)
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && sed -i 's/yy/EQN_yy/g' /home/matteo/Dropbox/sis/sis/io/read_eqn.c
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && sed -i 's/yy/EQN_yy/g' /home/matteo/Dropbox/sis/sis/io/read_eqn.h
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && sed -i 's/yy/EQN_yy/g' /home/matteo/Dropbox/sis/sis/io/eqnlex.c

# Rule to build all files generated by this target.
sis/io/CMakeFiles/io.dir/build: sis/io/libio.a

.PHONY : sis/io/CMakeFiles/io.dir/build

sis/io/CMakeFiles/io.dir/clean:
	cd /home/matteo/Dropbox/sis/cmake-build-debug/sis/io && $(CMAKE_COMMAND) -P CMakeFiles/io.dir/cmake_clean.cmake
.PHONY : sis/io/CMakeFiles/io.dir/clean

sis/io/CMakeFiles/io.dir/depend: ../sis/io/read_eqn.c
sis/io/CMakeFiles/io.dir/depend: ../sis/io/eqnlex.c
	cd /home/matteo/Dropbox/sis/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/matteo/Dropbox/sis /home/matteo/Dropbox/sis/sis/io /home/matteo/Dropbox/sis/cmake-build-debug /home/matteo/Dropbox/sis/cmake-build-debug/sis/io /home/matteo/Dropbox/sis/cmake-build-debug/sis/io/CMakeFiles/io.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : sis/io/CMakeFiles/io.dir/depend

