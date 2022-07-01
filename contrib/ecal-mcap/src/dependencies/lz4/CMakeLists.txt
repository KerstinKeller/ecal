project(lz4)

add_library(lz4)

target_sources(lz4
  PRIVATE
    lz4.c
    lz4.h
)

target_include_directories(lz4
  PUBLIC
    .
)

