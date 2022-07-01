# ========================= eCAL LICENSE =================================
#
# Copyright (C) 2016 - 2019 Continental Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ========================= eCAL LICENSE =================================

project(hdf5 LANGUAGES C CXX)

add_subdirectory(src/dependencies/lz4)
add_subdirectory(src/dependencies/mcap)
set(ZSTD_BUILD_STATIC ON)
add_subdirectory(src/dependencies/zstd/build/cmake)


set(ecalhdf5_src
    src/eh5_meas.cpp
    src/escape.h
    src/escape.cpp
)

set(ecalhdf5_header_base
    include/ecalhdf5/eh5_defs.h
    include/ecalhdf5/eh5_meas.h
    include/ecalhdf5/eh5_types.h   
)

ecal_add_library(${PROJECT_NAME} ${ecalhdf5_src} ${ecalhdf5_header_base})
add_library(eCAL::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PRIVATE .
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} 
  PUBLIC 
    eCAL::message
  PRIVATE  
    eCAL::utils
    mcap
)

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}"
    FILES
        ${ecalhdf5_src}
        ${ecalhdf5_header_base}
)

ecal_install_library(${PROJECT_NAME})
install(DIRECTORY
    "include/" DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT sdk
    FILES_MATCHING PATTERN "*.h")

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER contrib)
