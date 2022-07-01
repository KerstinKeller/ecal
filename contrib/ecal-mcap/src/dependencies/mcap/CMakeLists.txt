project(mcap)

add_library(mcap INTERFACE)

target_sources(mcap
  INTERFACE
    include/mcap/crc32.hpp
    include/mcap/errors.hpp
    include/mcap/internal.hpp
    include/mcap/mcap.hpp
    include/mcap/reader.hpp
    include/mcap/types.hpp
    include/mcap/writer.hpp
)

target_include_directories(mcap
  INTERFACE
    ./include
)

target_link_libraries(mcap
  INTERFACE
    lz4
    libzstd_static
)

target_compile_features(mcap INTERFACE cxx_std_17)