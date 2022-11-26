project('libgenpatten_test', 'c')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : true)
incdir = include_directories('include', 'test_images')
subdir('src')

executable('gp_test', src, include_directories : incdir, dependencies : m_dep)
