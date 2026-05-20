#include "ecal_serialize_common.h"
#include "ecal_serialize_tracing.h"

#include <ecal/core/pb/tracing.pbftags.h>
#include <protozero/pbf_writer.hpp>
#include <protozero/buffer_vector.hpp>
#include <protozero/pbf_reader.hpp>
#include <protozero/ecal_helper.h>

#include <type_traits>

namespace
{
  template <typename Writer>
  void SerializeTopicMetadata(Writer& writer, const eCAL::tracing::STopicMetadata& metadata)
  {
    writer.add_string(+eCAL::pb::TraceTopicMetadata::optional_string_tracing_version, metadata.tracing_version);
    writer.add_uint64(+eCAL::pb::TraceTopicMetadata::optional_uint64_entity_id, metadata.entity_id);
    writer.add_int32(+eCAL::pb::TraceTopicMetadata::optional_int32_process_id, metadata.process_id);
    writer.add_string(+eCAL::pb::TraceTopicMetadata::optional_string_host_name, metadata.host_name);
    writer.add_string(+eCAL::pb::TraceTopicMetadata::optional_string_topic_name, metadata.topic_name);
    writer.add_string(+eCAL::pb::TraceTopicMetadata::optional_string_encoding, metadata.encoding);
    writer.add_string(+eCAL::pb::TraceTopicMetadata::optional_string_type_name, metadata.type_name);
    writer.add_enum(+eCAL::pb::TraceTopicMetadata::optional_TraceTopicDirection_direction, static_cast<int32_t>(metadata.direction));
  }

  void DeserializeTopicMetadata(::protozero::pbf_reader& reader, eCAL::tracing::STopicMetadata& metadata)
  {
    while (reader.next())
    {
      switch (reader.tag())
      {
      case +eCAL::pb::TraceTopicMetadata::optional_string_tracing_version: AssignString(reader, metadata.tracing_version); break;
      case +eCAL::pb::TraceTopicMetadata::optional_uint64_entity_id: metadata.entity_id = reader.get_uint64(); break;
      case +eCAL::pb::TraceTopicMetadata::optional_int32_process_id: metadata.process_id = reader.get_int32(); break;
      case +eCAL::pb::TraceTopicMetadata::optional_string_host_name: AssignString(reader, metadata.host_name); break;
      case +eCAL::pb::TraceTopicMetadata::optional_string_topic_name: AssignString(reader, metadata.topic_name); break;
      case +eCAL::pb::TraceTopicMetadata::optional_string_encoding: AssignString(reader, metadata.encoding); break;
      case +eCAL::pb::TraceTopicMetadata::optional_string_type_name: AssignString(reader, metadata.type_name); break;
      case +eCAL::pb::TraceTopicMetadata::optional_TraceTopicDirection_direction: metadata.direction = static_cast<eCAL::tracing::topic_direction>(reader.get_enum()); break;
      default: reader.skip(); break;
      }
    }
  }

  template <typename Writer>
  void SerializePublisherSpan(Writer& writer, const eCAL::tracing::SPublisherSpanData& span)
  {
    writer.add_enum(+eCAL::pb::PublisherTraceSpan::optional_TraceOperationType_op_type, static_cast<int32_t>(span.op_type));
    writer.add_uint64(+eCAL::pb::PublisherTraceSpan::optional_uint64_entity_id, span.entity_id);
    writer.add_uint64(+eCAL::pb::PublisherTraceSpan::optional_uint64_process_id, span.process_id);
    writer.add_uint64(+eCAL::pb::PublisherTraceSpan::optional_uint64_payload_size, static_cast<uint64_t>(span.payload_size));
    writer.add_int64(+eCAL::pb::PublisherTraceSpan::optional_sint64_clock, span.clock);
    writer.add_uint64(+eCAL::pb::PublisherTraceSpan::optional_uint64_layer, span.layer);
    writer.add_int64(+eCAL::pb::PublisherTraceSpan::optional_sint64_start_ns, span.start_ns);
    writer.add_int64(+eCAL::pb::PublisherTraceSpan::optional_sint64_end_ns, span.end_ns);
  }

  void DeserializePublisherSpan(::protozero::pbf_reader& reader, eCAL::tracing::SPublisherSpanData& span)
  {
    while (reader.next())
    {
      switch (reader.tag())
      {
      case +eCAL::pb::PublisherTraceSpan::optional_TraceOperationType_op_type: span.op_type = static_cast<eCAL::tracing::operation_type>(reader.get_enum()); break;
      case +eCAL::pb::PublisherTraceSpan::optional_uint64_entity_id: span.entity_id = reader.get_uint64(); break;
      case +eCAL::pb::PublisherTraceSpan::optional_uint64_process_id: span.process_id = reader.get_uint64(); break;
      case +eCAL::pb::PublisherTraceSpan::optional_uint64_payload_size: span.payload_size = static_cast<size_t>(reader.get_uint64()); break;
      case +eCAL::pb::PublisherTraceSpan::optional_sint64_clock: span.clock = reader.get_sint64(); break;
      case +eCAL::pb::PublisherTraceSpan::optional_uint64_layer: span.layer = reader.get_uint64(); break;
      case +eCAL::pb::PublisherTraceSpan::optional_sint64_start_ns: span.start_ns = reader.get_sint64(); break;
      case +eCAL::pb::PublisherTraceSpan::optional_sint64_end_ns: span.end_ns = reader.get_sint64(); break;
      default: reader.skip(); break;
      }
    }
  }

  template <typename Writer>
  void SerializeSubscriberSpan(Writer& writer, const eCAL::tracing::SSubscriberSpanData& span)
  {
    writer.add_enum(+eCAL::pb::SubscriberTraceSpan::optional_TraceOperationType_op_type, static_cast<int32_t>(span.op_type));
    writer.add_uint64(+eCAL::pb::SubscriberTraceSpan::optional_uint64_entity_id, span.entity_id);
    writer.add_uint64(+eCAL::pb::SubscriberTraceSpan::optional_uint64_topic_id, span.topic_id);
    writer.add_uint64(+eCAL::pb::SubscriberTraceSpan::optional_uint64_process_id, span.process_id);
    writer.add_uint64(+eCAL::pb::SubscriberTraceSpan::optional_uint64_payload_size, static_cast<uint64_t>(span.payload_size));
    writer.add_int64(+eCAL::pb::SubscriberTraceSpan::optional_sint64_clock, span.clock);
    writer.add_uint64(+eCAL::pb::SubscriberTraceSpan::optional_uint64_layer, span.layer);
    writer.add_int64(+eCAL::pb::SubscriberTraceSpan::optional_sint64_start_ns, span.start_ns);
    writer.add_int64(+eCAL::pb::SubscriberTraceSpan::optional_sint64_end_ns, span.end_ns);
  }

  void DeserializeSubscriberSpan(::protozero::pbf_reader& reader, eCAL::tracing::SSubscriberSpanData& span)
  {
    while (reader.next())
    {
      switch (reader.tag())
      {
      case +eCAL::pb::SubscriberTraceSpan::optional_TraceOperationType_op_type: span.op_type = static_cast<eCAL::tracing::operation_type>(reader.get_enum()); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_uint64_entity_id: span.entity_id = reader.get_uint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_uint64_topic_id: span.topic_id = reader.get_uint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_uint64_process_id: span.process_id = reader.get_uint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_uint64_payload_size: span.payload_size = static_cast<size_t>(reader.get_uint64()); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_sint64_clock: span.clock = reader.get_sint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_uint64_layer: span.layer = reader.get_uint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_sint64_start_ns: span.start_ns = reader.get_sint64(); break;
      case +eCAL::pb::SubscriberTraceSpan::optional_sint64_end_ns: span.end_ns = reader.get_sint64(); break;
      default: reader.skip(); break;
      }
    }
  }

  template <typename Writer>
  void SerializeTraceRecord(Writer& writer, const eCAL::protozero::TraceRecordVariant& record)
  {
    std::visit([&writer](const auto& value)
    {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same<T, eCAL::tracing::STopicMetadata>::value)
      {
        Writer nested_writer{ writer, +eCAL::pb::TraceRecord::optional_TraceTopicMetadata_metadata };
        SerializeTopicMetadata(nested_writer, value);
      }
      else if constexpr (std::is_same<T, eCAL::tracing::SPublisherSpanData>::value)
      {
        Writer nested_writer{ writer, +eCAL::pb::TraceRecord::optional_PublisherTraceSpan_publisher };
        SerializePublisherSpan(nested_writer, value);
      }
      else
      {
        Writer nested_writer{ writer, +eCAL::pb::TraceRecord::optional_SubscriberTraceSpan_subscriber };
        SerializeSubscriberSpan(nested_writer, value);
      }
    }, record);
  }

  void DeserializeTraceRecord(::protozero::pbf_reader& reader, eCAL::protozero::TraceRecordVariant& record)
  {
    while (reader.next())
    {
      switch (reader.tag())
      {
      case +eCAL::pb::TraceRecord::optional_TraceTopicMetadata_metadata:
      {
        eCAL::tracing::STopicMetadata metadata{};
        AssignMessage(reader, metadata, DeserializeTopicMetadata);
        record = std::move(metadata);
        return;
      }
      case +eCAL::pb::TraceRecord::optional_PublisherTraceSpan_publisher:
      {
        eCAL::tracing::SPublisherSpanData span{};
        AssignMessage(reader, span, DeserializePublisherSpan);
        record = std::move(span);
        return;
      }
      case +eCAL::pb::TraceRecord::optional_SubscriberTraceSpan_subscriber:
      {
        eCAL::tracing::SSubscriberSpanData span{};
        AssignMessage(reader, span, DeserializeSubscriberSpan);
        record = std::move(span);
        return;
      }
      default:
        reader.skip();
        break;
      }
    }
  }

  template <typename Writer>
  void SerializeTraceRecordList(Writer& writer, const std::vector<eCAL::protozero::TraceRecordVariant>& records)
  {
    for (const auto& record : records)
    {
      Writer nested_writer{ writer, +eCAL::pb::TraceRecordList::repeated_TraceRecord_records };
      SerializeTraceRecord(nested_writer, record);
    }
  }

  void DeserializeTraceRecordList(::protozero::pbf_reader& reader, std::vector<eCAL::protozero::TraceRecordVariant>& records)
  {
    while (reader.next())
    {
      switch (reader.tag())
      {
      case +eCAL::pb::TraceRecordList::repeated_TraceRecord_records:
      {
        eCAL::protozero::TraceRecordVariant record = eCAL::tracing::STopicMetadata{};
        AssignMessage(reader, record, DeserializeTraceRecord);
        records.emplace_back(std::move(record));
        break;
      }
      default:
        reader.skip();
        break;
      }
    }
  }
}

namespace eCAL { namespace protozero {
bool SerializeToBuffer(const tracing::STopicMetadata& s, std::vector<char>& b){ b.clear(); ::protozero::basic_pbf_writer<std::vector<char>> w{b}; SerializeTopicMetadata(w,s); return true; }
bool SerializeToBuffer(const tracing::STopicMetadata& s, std::string& b){ b.clear(); ::protozero::pbf_writer w{b}; SerializeTopicMetadata(w,s); return true; }
bool DeserializeFromBuffer(const char* d, size_t z, tracing::STopicMetadata& t){ try{ t={}; ::protozero::pbf_reader r{d,z}; DeserializeTopicMetadata(r,t); return true;} catch(const std::exception& e){ LogDeserializationException(e,"eCAL::tracing::STopicMetadata"); return false; } }

bool SerializeToBuffer(const tracing::SPublisherSpanData& s, std::vector<char>& b){ b.clear(); ::protozero::basic_pbf_writer<std::vector<char>> w{b}; SerializePublisherSpan(w,s); return true; }
bool SerializeToBuffer(const tracing::SPublisherSpanData& s, std::string& b){ b.clear(); ::protozero::pbf_writer w{b}; SerializePublisherSpan(w,s); return true; }
bool DeserializeFromBuffer(const char* d, size_t z, tracing::SPublisherSpanData& t){ try{ t={}; ::protozero::pbf_reader r{d,z}; DeserializePublisherSpan(r,t); return true;} catch(const std::exception& e){ LogDeserializationException(e,"eCAL::tracing::SPublisherSpanData"); return false; } }

bool SerializeToBuffer(const tracing::SSubscriberSpanData& s, std::vector<char>& b){ b.clear(); ::protozero::basic_pbf_writer<std::vector<char>> w{b}; SerializeSubscriberSpan(w,s); return true; }
bool SerializeToBuffer(const tracing::SSubscriberSpanData& s, std::string& b){ b.clear(); ::protozero::pbf_writer w{b}; SerializeSubscriberSpan(w,s); return true; }
bool DeserializeFromBuffer(const char* d, size_t z, tracing::SSubscriberSpanData& t){ try{ t={}; ::protozero::pbf_reader r{d,z}; DeserializeSubscriberSpan(r,t); return true;} catch(const std::exception& e){ LogDeserializationException(e,"eCAL::tracing::SSubscriberSpanData"); return false; } }

bool SerializeToBuffer(const TraceRecordVariant& s, std::vector<char>& b){ b.clear(); ::protozero::basic_pbf_writer<std::vector<char>> w{b}; SerializeTraceRecord(w,s); return true; }
bool SerializeToBuffer(const TraceRecordVariant& s, std::string& b){ b.clear(); ::protozero::pbf_writer w{b}; SerializeTraceRecord(w,s); return true; }
bool DeserializeFromBuffer(const char* d, size_t z, TraceRecordVariant& t){ try{ ::protozero::pbf_reader r{d,z}; DeserializeTraceRecord(r,t); return true;} catch(const std::exception& e){ LogDeserializationException(e,"eCAL::protozero::TraceRecordVariant"); return false; } }

bool SerializeToBuffer(const std::vector<TraceRecordVariant>& s, std::vector<char>& b){ b.clear(); ::protozero::basic_pbf_writer<std::vector<char>> w{b}; SerializeTraceRecordList(w,s); return true; }
bool SerializeToBuffer(const std::vector<TraceRecordVariant>& s, std::string& b){ b.clear(); ::protozero::pbf_writer w{b}; SerializeTraceRecordList(w,s); return true; }
bool DeserializeFromBuffer(const char* d, size_t z, std::vector<TraceRecordVariant>& t){ try{ t.clear(); ::protozero::pbf_reader r{d,z}; DeserializeTraceRecordList(r,t); return true;} catch(const std::exception& e){ LogDeserializationException(e,"std::vector<eCAL::protozero::TraceRecordVariant>"); return false; } }
}}
