cd simulator

# uncomment to build the debug version
# increase [4] to use more threads
# /usr/bin/scons build/X86/gem5.debug -j4

# increase [4] to use more threads
/usr/bin/scons build/X86/gem5.opt -j4
