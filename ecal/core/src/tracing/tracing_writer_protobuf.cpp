/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2025 Continental Corporation
 * Copyright 2025 AUMOVIO and subsidiaries. All rights reserved.
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

#include "tracing_writer_protobuf.h"

#include "serialization/ecal_serialize_tracing.h"

#include <ecal/process.h>
#include <ecal/util.h>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace eCAL
{
  namespace tracing
  {
    static std::string GetCurrentTimestamp()
    {
      std::time_t now = std::time(nullptr);
      std::tm* tm_info = std::localtime(&now);
      std::ostringstream oss;
      oss << std::put_time(tm_info, "%Y%m%d_%H%M%S");
      return oss.str();
    }

    static void AppendVarint32(std::string& out, uint32_t value)
    {
      while (value >= 0x80)
      {
        out.push_back(static_cast<char>((value & 0x7F) | 0x80));
        value >>= 7;
      }
      out.push_back(static_cast<char>(value));
    }

    CTracingWriterProtobuf::CTracingWriterProtobuf()
      : timestamp_(GetCurrentTimestamp())
      , trace_dir_(eCAL::Util::GeteCALTraceDir())
    {}

    std::string CTracingWriterProtobuf::GetTraceFilePath() const
    {
      return trace_dir_ + "/ecal_trace_" + std::to_string(eCAL::Process::GetProcessID()) + "_" + timestamp_ + ".pb";
    }

    bool CTracingWriterProtobuf::WriteRecord(const eCAL::protozero::TraceRecordVariant& record)
    {
      std::string payload;
      if (!eCAL::protozero::SerializeToBuffer(record, payload))
        return false;

      std::string framed;
      framed.reserve(5 + payload.size());
      AppendVarint32(framed, static_cast<uint32_t>(payload.size()));
      framed.append(payload);

      std::ofstream output_file(GetTraceFilePath(), std::ios::app | std::ios::binary);
      if (!output_file.is_open())
      {
        std::cerr << "Warning: Could not open protobuf trace file: " << GetTraceFilePath() << std::endl;
        return false;
      }
      output_file.write(framed.data(), static_cast<std::streamsize>(framed.size()));
      return output_file.good();
    }

    void CTracingWriterProtobuf::WriteSpansToFile(const std::vector<SpanDataVariant>& batch)
    {
      std::lock_guard<std::mutex> lock(write_mutex_);
      for (const auto& span : batch)
      {
        eCAL::protozero::TraceRecordVariant record;
        std::visit([&record](const auto& s) { record = s; }, span);
        if (!WriteRecord(record))
        {
          std::cerr << "Warning: Failed to serialize protobuf span record." << std::endl;
          return;
        }
      }
    }

    void CTracingWriterProtobuf::WriteMetadataToFile(const STopicMetadata& metadata)
    {
      std::lock_guard<std::mutex> lock(write_mutex_);
      if (!WriteRecord(metadata))
      {
        std::cerr << "Warning: Failed to serialize protobuf metadata record." << std::endl;
      }
    }
  }
}
