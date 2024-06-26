# ========================= eCAL LICENSE =================================
#
# Copyright (C) 2016 - 2023 Continental Corporation
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

project(service_sample_server)

find_package(Threads REQUIRED)

set(sources
  src/main.cpp
)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ecal_service)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER core/service)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${sources}
)
