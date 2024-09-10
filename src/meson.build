src = [
  'dir.cc',
  'hash.cc',
  'main.cc',
  'stat.cc',
  'util.cc',
  ]

# https://mesonbuild.com/Dependencies.html#openssl
dep = [dependency('openssl')]

if get_option('debug')
  add_global_arguments('-DDEBUG', language : 'cpp')
endif

if get_option('squash2')
  add_global_arguments('-DCONFIG_SQUASH2', language : 'cpp')
  src += 'squash2.cc'
else
  add_global_arguments('-DCONFIG_SQUASH1', language : 'cpp')
  src += 'squash1.cc'
endif

# `dnf install cppunit cppunit-devel` on Fedora
if get_option('cppunit')
  add_global_arguments('-DCONFIG_CPPUNIT', language : 'cpp')
  dep += dependency('cppunit')
  src += 'cppunit.cc'
endif

executable('dirhash-cpp', src, dependencies : dep, install : true)
