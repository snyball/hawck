tests_src = [
  'CSV-tests.cpp',
  'FSWatcher-tests.cpp',
  'tests-main.cpp',
  '../src/FSWatcher.cpp',
  '../src/CSV.cpp'
]

executable('hawck-tests',
           tests_src,
           include_directories : inc,
           dependencies : [pthreaddep, catch2dep],
           install : true
          )
