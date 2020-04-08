sources = files([
  'paket.cpp'
])

src = include_directories('.')

lib = library(meson.project_name(), sources, include_directories : includes, install: true, dependencies: module_deps)
