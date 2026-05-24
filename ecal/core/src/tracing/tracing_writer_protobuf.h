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

#pragma once

#include "tracing_writer.h"

#include <mutex>
#include <string>

namespace eCAL
{
  namespace tracing
  {
    class CTracingWriterProtobuf : public TracingWriter
    {
    public:
      CTracingWriterProtobuf();
      ~CTracingWriterProtobuf() override = default;

      void WriteSpansToFile(const std::vector<SpanDataVariant>& batch) override;
      void WriteMetadataToFile(const STopicMetadata& metadata) override;

      std::string GetTraceFilePath() const;

    private:
      bool WriteRecord(const eCAL::protozero::TraceRecordVariant& record);

      mutable std::mutex write_mutex_;
      std::string        timestamp_;
      std::string        trace_dir_;
    };
  }
}
