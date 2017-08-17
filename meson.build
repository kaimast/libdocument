project('libdocument', 'cpp', default_options: ['cpp_std=c++1z'])

inc_dirs = include_directories('include', '/usr/local/include')

gtest = dependency('gtest', main : true)
gmock = dependency('gmock')

cpp_files =  ['src/Parser.cpp', 'src/Iterator.cpp', 'src/Writer.cpp', 'src/json.cpp']

doc = shared_library('document', cpp_files, include_directories: inc_dirs, install: true)
doc_dep = declare_dependency(link_with: doc)

sgx_sdk_dir = '/opt/intel/sgxsdk'
sgx_library_path = sgx_sdk_dir + '/lib64'

sgx_tlibc_incdir = include_directories(sgx_sdk_dir+'/include/tlibc')
sgx_stdcxx_incdir = include_directories(sgx_sdk_dir+'/include/libcxx')
sgx_incdir = include_directories(sgx_sdk_dir + '/include')

cpp = meson.get_compiler('cpp')

sgx_tstdc_dep = cpp.find_library('sgx_tstdc', dirs: sgx_library_path)
sgx_tstdcxx_dep = cpp.find_library('sgx_tcxx', dirs: sgx_library_path)

doc_sgx = static_library('document_sgx', [cpp_files], cpp_args: ['-DIS_ENCLAVE', '-nostdinc', '-fPIC'], include_directories: [sgx_incdir, sgx_tlibc_incdir, sgx_stdcxx_incdir, inc_dirs], install: true, dependencies: [sgx_tstdc_dep, sgx_tstdcxx_dep])

e = executable('libdocument-test', ['test/json.cpp'], dependencies : [doc_dep, gtest, gmock], include_directories: inc_dirs)
test('libdocument-test', e)

install_subdir('include/json', install_dir : 'include')
