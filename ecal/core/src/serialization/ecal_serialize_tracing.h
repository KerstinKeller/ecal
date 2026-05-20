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

#include <tracing/tracing.h>

#include <string>
#include <variant>
#include <vector>

namespace eCAL
{
  inline namespace protozero
  {
    using TraceRecordVariant = std::variant<tracing::STopicMetadata, tracing::SPublisherSpanData, tracing::SSubscriberSpanData>;

    bool SerializeToBuffer(const tracing::STopicMetadata& source_sample_, std::vector<char>& target_buffer_);
    bool SerializeToBuffer(const tracing::STopicMetadata& source_sample_, std::string& target_buffer_);
    bool DeserializeFromBuffer(const char* data_, size_t size_, tracing::STopicMetadata& target_sample_);

    bool SerializeToBuffer(const tracing::SPublisherSpanData& source_sample_, std::vector<char>& target_buffer_);
    bool SerializeToBuffer(const tracing::SPublisherSpanData& source_sample_, std::string& target_buffer_);
    bool DeserializeFromBuffer(const char* data_, size_t size_, tracing::SPublisherSpanData& target_sample_);

    bool SerializeToBuffer(const tracing::SSubscriberSpanData& source_sample_, std::vector<char>& target_buffer_);
    bool SerializeToBuffer(const tracing::SSubscriberSpanData& source_sample_, std::string& target_buffer_);
    bool DeserializeFromBuffer(const char* data_, size_t size_, tracing::SSubscriberSpanData& target_sample_);

    bool SerializeToBuffer(const TraceRecordVariant& source_sample_, std::vector<char>& target_buffer_);
    bool SerializeToBuffer(const TraceRecordVariant& source_sample_, std::string& target_buffer_);
    bool DeserializeFromBuffer(const char* data_, size_t size_, TraceRecordVariant& target_sample_);

    bool SerializeToBuffer(const std::vector<TraceRecordVariant>& source_sample_, std::vector<char>& target_buffer_);
    bool SerializeToBuffer(const std::vector<TraceRecordVariant>& source_sample_, std::string& target_buffer_);
    bool DeserializeFromBuffer(const char* data_, size_t size_, std::vector<TraceRecordVariant>& target_sample_);
  }
}
