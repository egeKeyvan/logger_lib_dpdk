project('test project','cpp')
dpdk = dependency('libdpdk')

subdir('include')

sources = files('main.cpp')
incdir = include_directories('include')


# logger_lib = library('dpdk_logger_lib', 
#                                 'logger_lib.cpp', 
#                                 dependencies: dpdk, 
#                                 include_directories: incdir, 
#                                 install: true)

executable('demo', sources, link_with: [logger_lib], include_directories: incdir, dependencies: dpdk)