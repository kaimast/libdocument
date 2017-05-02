project('libdocument', 'cpp', default_options: ['cpp_std=c++1z'])

incdir = include_directories('include')

gtest = dependency('gtest', main : true)
gmock = dependency('gmock')

doc = shared_library('document', ['src/Parser.cpp', 'src/Iterator.cpp', 'src/Writer.cpp', 'src/json.cpp'], include_directories: incdir, install: true)
doc_dep = declare_dependency(link_with: doc)

e = executable('libdocument-test', ['test/json.cpp'], dependencies : [doc_dep, gtest, gmock], include_directories: incdir)
test('libdocument-test', e)

install_subdir('include/json', install_dir : 'include')
