import os

#
# Set up environment
#
env = Environment(ENV = os.environ )

#
# Use c++11 and disable warnings in dral_server.h
#
env.Append(CCFLAGS = ['-std=c++0x', '-Wno-write-strings'])

#
# Add support for libasim/libdral
#
env.ParseConfig('pkg-config --cflags --libs libasim')

#
# Add support for embedding the adf in a class
# (currently used only by example7)
#
adf2c = Builder(action = 'utils/adf2c < $SOURCE > $TARGET')
env.Append(BUILDERS = {'Adf2c' : adf2c})

#
# Interate over the subdirectories
#
subdirs = ['src']

for subdir in subdirs:
    env.SConscript('%s/SConscript' % subdir, {'env': env})

