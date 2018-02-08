project('libdocument', 'cpp')

inc_dirs = include_directories('/usr/local/include', 'include/', get_option('prefix')+'/include')

gtest = dependency('gtest')
gmock = dependency('gmock')

compile_args = ['-std=c++17', '-Wextra', '-Werror', '-Wno-implicit-exception-spec-mismatch'] # Remove me once clang issue is fixed in SGX SDK

if get_option('use_geo') == true
    compile_args = compile_args + ['-DUSE_GEO']
endif

subdir('src')

doc = shared_library('document', cpp_files, include_directories: inc_dirs, install: true, cpp_args: compile_args)
doc_dep = declare_dependency(link_with: doc)

sgx_sdk_dir = '/opt/intel/sgxsdk'
sgx_library_path = sgx_sdk_dir + '/lib64'

sgx_tlibc_incdir = include_directories(sgx_sdk_dir+'/include/tlibc')
sgx_stdcxx_incdir = include_directories(sgx_sdk_dir+'/include/libcxx')
sgx_incdir = include_directories(sgx_sdk_dir + '/include')

cpp = meson.get_compiler('cpp')

sgx_tstdc_dep = cpp.find_library('sgx_tstdc', dirs: sgx_library_path)
sgx_tstdcxx_dep = cpp.find_library('sgx_tcxx', dirs: sgx_library_path)
sgx_trts_dep = cpp.find_library('sgx_trts', dirs: sgx_library_path)

sgx_compile_args = compile_args + ['-DIS_ENCLAVE', '-nostdinc', '-fpie', '-fstack-protector', '-std=c++14', '-nostdinc++']
 
doc_sgx = static_library('document_sgx', cpp_files,
    c_args: sgx_compile_args,
    cpp_args: sgx_compile_args,
    link_args:['-Wl,--no-undefined','-nostdlib','-nodefaultlibs','-L'+sgx_sdk_dir+'/lib64',
    '-Wl,--whole-archive','-lsgx_trts','-Wl,--no-whole-archive',
    '-Wl,--start-group', '-lsgx_tstdc','-lsgx_tcxx', '-Wl,--end-group',
    ],
    include_directories: [sgx_incdir, sgx_tlibc_incdir, sgx_stdcxx_incdir, inc_dirs], install: true)

subdir('test')

e = executable('libdocument-test', test_files, dependencies : [doc_dep, gtest, gmock], include_directories: inc_dirs)
test('libdocument-test', e)

install_subdir('include/json', install_dir : 'include')
