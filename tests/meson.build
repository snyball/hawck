catch2dep = dependency('catch2', required : false)

if catch2dep.found()
  tests_src = [
    'CSV-tests.cpp',
    'FSWatcher-tests.cpp',
    'tests-main.cpp',
    'XDG-tests.cpp',
    'Popen-tests.cpp',
    'Version-tests.cpp',
    '../src/Popen.cpp',
    '../src/FSWatcher.cpp',
    '../src/XDG.cpp',
    '../src/CSV.cpp',
    '../src/Permissions.cpp',
    '../src/Version.cpp',
  ]
  
  executable('hawck-tests',
             tests_src,
             include_directories : inc,
             dependencies : [pthreaddep, catch2dep],
             install : false,
             #c_pch : 'pch/tests_pch.h',
             #cpp_pch : 'pch/tests_pch.hpp',
            )
else
  warning('Unable to compile tests, did not find catch2')
endif
