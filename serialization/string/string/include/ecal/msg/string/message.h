/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2025 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

#pragma once

#include <string>

//#include <ecal/msg/message.h>

namespace eCAL
{
  namespace message
  {
    // unfortunately, we need an actual object for this :/
    inline std::string GetTypeName(const std::string& /*message*/)
    {
      return("std::string");
    }

    // unfortunately, we need an actual object for this :/
    inline std::string GetEncoding(const std::string& /*message*/)
    {
      return("base");
    }
    
    // unfortunately, we need an actual object for this :/
    inline std::string GetDescription(const std::string& /*message*/)
    {
      return("");
    }


    inline bool Serialize(const std::string& message, std::string& buffer)
    {
      buffer = message;
      return true;
    }

    inline bool Deserialize(const std::string& buffer, std::string& message)
    {
      message = buffer;
      return true;
    }
  }
}