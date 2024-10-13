project('libdocument', 'cpp')

inc_dirs = include_directories('include/', get_option('prefix')+'/include', '/usr/local/include')

gtest = dependency('gtest')

compile_args = ['-std=c++20', '-Wextra', '-Wno-implicit-exception-spec-mismatch', '-Werror'] # Remove me once clang issue is fixed in SGX SDK

if get_option('use_geo') == true
    compile_args = compile_args + ['-DUSE_GEO']
endif

subdir('src')

doc = shared_library('document', cpp_files,
    include_directories: inc_dirs, install: true, cpp_args: compile_args)
doc_dep = declare_dependency(link_with: doc)

cpp = meson.get_compiler('cpp')

sgx_library_path = get_option('sgx_sdk_dir') + '/lib64'

sgx_tstdc_dep = cpp.find_library('sgx_tstdc', dirs: sgx_library_path, required: false)
sgx_tstdcxx_dep = cpp.find_library('sgx_tcxx', dirs: sgx_library_path, required: false)
sgx_trts_dep = cpp.find_library('sgx_trts', dirs: sgx_library_path, required: false)

if sgx_tstdc_dep.found() and sgx_tstdcxx_dep.found() and sgx_trts_dep.found()
    message('SGX found')

    sgx_tlibc_incdir = include_directories(get_option('sgx_sdk_dir') +'/include/tlibc')
    sgx_stdcxx_incdir = include_directories(get_option('sgx_sdk_dir') +'/include/libcxx')
    sgx_incdir = include_directories(get_option('sgx_sdk_dir') + '/include')

    sgx_compile_args = compile_args + ['-DIS_ENCLAVE', '-nostdinc', '-fpie', '-fstack-protector', '-nostdinc++']
     
    doc_sgx = static_library('document_sgx', cpp_files,
        c_args: sgx_compile_args,
        cpp_args: sgx_compile_args,
        link_args:['-Wl,--no-undefined','-nostdlib','-nodefaultlibs','-L'+get_option('sgx_sdk_dir')+'/lib64',
        '-Wl,--whole-archive','-lsgx_trts','-Wl,--no-whole-archive',
        '-Wl,--start-group', '-lsgx_tstdc','-lsgx_tcxx', '-Wl,--end-group',
        ],
        include_directories: [sgx_incdir, sgx_tlibc_incdir, sgx_stdcxx_incdir, inc_dirs], install: true)
else
    message('SGX not found. Not building trusted library.')
endif

subdir('test')

unit_tests_bin = executable('libdocument-test', test_files,
    dependencies : [doc_dep, gtest],
    include_directories: inc_dirs, cpp_args: compile_args)

run_target(
    'unit-tests',
    command: unit_tests_bin,
    depends: [unit_tests_bin])

install_subdir('include/json', install_dir : 'include')

# NOTE: gtest on ubuntu still uses deprecated functions so we can't lint the test files yet
clangtidy = find_program('clang-tidy', required: false)

if clangtidy.found()
    tidy_files = cpp_files

    run_target(
        'lint',
        command: [
            'scripts/clang-tidy.sh',
            clangtidy.path(),
            meson.source_root(),
            meson.build_root(),
            tidy_files
        ],
    depends: [doc])
endif


