catch2dep = dependency('catch2', required : false)

## Try to download catch2
if not catch2dep.found()
  message('Downloading Catch2 header file ...')
  p = run_command(
    '../bin/dl-dep.sh',
    'https://github.com/catchorg/Catch2/releases/download/v2.3.0/catch.hpp',
    'catch2'
    )
  if p.returncode() != 0
    warning('Unable to download catch2')
  else
    catch2_inc_dir = p.stdout().strip()
    catch2_inc = include_directories(['../src/include-ext'])
    catch2dep = declare_dependency(
      include_directories : catch2_inc,
    )
  endif
endif

if catch2dep.found()
  tests_src = [
    'CSV-tests.cpp',
    'FSWatcher-tests.cpp',
    'tests-main.cpp',
    'XDG-tests.cpp',
    '../src/FSWatcher.cpp',
    '../src/XDG.cpp',
    '../src/CSV.cpp',
    '../src/Permissions.cpp',
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
