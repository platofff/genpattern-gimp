project('libgenpatten_test', 'c', default_options : ['c_std=c18', 'b_lto=true'])
cc = meson.get_compiler('c')
if get_option('buildtype') == 'release'
    add_global_arguments('-Ofast', language : 'c')
endif
m_dep = cc.find_library('m', required : true)
thread_dep = dependency('threads', required: true)
incdir = include_directories('include', 'test_images')
subdir('src')

executable('gp_test', src, include_directories : incdir, dependencies : [m_dep, thread_dep])
