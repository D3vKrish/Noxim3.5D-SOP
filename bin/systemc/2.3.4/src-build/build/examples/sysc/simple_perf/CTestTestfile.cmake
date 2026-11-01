# CMake generated Testfile for 
# Source directory: /home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/systemc-2.3.4/examples/sysc/simple_perf
# Build directory: /home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/build/examples/sysc/simple_perf
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(examples/sysc/simple_perf/simple_perf "/usr/bin/cmake" "-DTEST_EXE=/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/build/examples/sysc/simple_perf/simple_perf" "-DTEST_DIR=/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/build/examples/sysc/simple_perf" "-DTEST_INPUT=" "-DTEST_GOLDEN=" "-DTEST_FILTER=" "-DDIFF_COMMAND=/usr/bin/diff" "-DDIFF_OPTIONS=-u" "-P" "/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/systemc-2.3.4/cmake/run_test.cmake")
set_tests_properties(examples/sysc/simple_perf/simple_perf PROPERTIES  FAIL_REGULAR_EXPRESSION "^[*][*][*]ERROR" _BACKTRACE_TRIPLES "/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/systemc-2.3.4/examples/CMakeLists.txt;137;add_test;/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/systemc-2.3.4/examples/sysc/simple_perf/CMakeLists.txt;44;configure_and_add_test;/home/taksh/simulators/noxim-3.5d/bin/systemc/2.3.4/src-build/systemc-2.3.4/examples/sysc/simple_perf/CMakeLists.txt;0;")
