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

#include <serialization/ecal_serialize_tracing.h>
#include <tracing/tracing.h>

#include <gtest/gtest.h>

TEST(TestTracingSerialization, TopicMetadataRoundTrip)
{
  eCAL::tracing::STopicMetadata source{};
  source.tracing_version = "1.0.0";
  source.entity_id       = 1234;
  source.process_id      = 5678;
  source.host_name       = "host_a";
  source.topic_name      = "topic_a";
  source.encoding        = "protobuf";
  source.type_name       = "pkg.Msg";
  source.direction       = eCAL::tracing::topic_direction::subscriber;

  std::string buffer;
  ASSERT_TRUE(eCAL::protozero::SerializeToBuffer(source, buffer));

  eCAL::tracing::STopicMetadata target{};
  ASSERT_TRUE(eCAL::protozero::DeserializeFromBuffer(buffer.data(), buffer.size(), target));

  EXPECT_EQ(target.tracing_version, source.tracing_version);
  EXPECT_EQ(target.entity_id, source.entity_id);
  EXPECT_EQ(target.process_id, source.process_id);
  EXPECT_EQ(target.host_name, source.host_name);
  EXPECT_EQ(target.topic_name, source.topic_name);
  EXPECT_EQ(target.encoding, source.encoding);
  EXPECT_EQ(target.type_name, source.type_name);
  EXPECT_EQ(target.direction, source.direction);
}

TEST(TestTracingSerialization, PublisherSpanRoundTrip)
{
  eCAL::tracing::SPublisherSpanData source{};
  source.op_type      = eCAL::tracing::operation_type::send;
  source.entity_id    = 42;
  source.process_id   = 12;
  source.payload_size = 512;
  source.clock        = -4;
  source.layer        = eCAL::tracing::tl_trace_shm_udp;
  source.start_ns     = 1000;
  source.end_ns       = 2000;

  std::vector<char> buffer;
  ASSERT_TRUE(eCAL::protozero::SerializeToBuffer(source, buffer));

  eCAL::tracing::SPublisherSpanData target{};
  ASSERT_TRUE(eCAL::protozero::DeserializeFromBuffer(buffer.data(), buffer.size(), target));

  EXPECT_EQ(target.op_type, source.op_type);
  EXPECT_EQ(target.entity_id, source.entity_id);
  EXPECT_EQ(target.process_id, source.process_id);
  EXPECT_EQ(target.payload_size, source.payload_size);
  EXPECT_EQ(target.clock, source.clock);
  EXPECT_EQ(target.layer, source.layer);
  EXPECT_EQ(target.start_ns, source.start_ns);
  EXPECT_EQ(target.end_ns, source.end_ns);
}

TEST(TestTracingSerialization, SubscriberSpanRoundTrip)
{
  eCAL::tracing::SSubscriberSpanData source{};
  source.op_type      = eCAL::tracing::operation_type::receive;
  source.entity_id    = 84;
  source.topic_id     = 21;
  source.process_id   = 13;
  source.payload_size = 1024;
  source.clock        = 8;
  source.layer        = eCAL::tracing::tl_trace_tcp;
  source.start_ns     = 3000;
  source.end_ns       = 4000;

  std::string buffer;
  ASSERT_TRUE(eCAL::protozero::SerializeToBuffer(source, buffer));

  eCAL::tracing::SSubscriberSpanData target{};
  ASSERT_TRUE(eCAL::protozero::DeserializeFromBuffer(buffer.data(), buffer.size(), target));

  EXPECT_EQ(target.op_type, source.op_type);
  EXPECT_EQ(target.entity_id, source.entity_id);
  EXPECT_EQ(target.topic_id, source.topic_id);
  EXPECT_EQ(target.process_id, source.process_id);
  EXPECT_EQ(target.payload_size, source.payload_size);
  EXPECT_EQ(target.clock, source.clock);
  EXPECT_EQ(target.layer, source.layer);
  EXPECT_EQ(target.start_ns, source.start_ns);
  EXPECT_EQ(target.end_ns, source.end_ns);
}

TEST(TestTracingSerialization, RecordVariantAndListRoundTrip)
{
  std::vector<eCAL::protozero::TraceRecordVariant> source;

  eCAL::tracing::STopicMetadata metadata{};
  metadata.tracing_version = "1.0.0";
  metadata.entity_id       = 1;
  metadata.process_id      = 2;
  metadata.host_name       = "h";
  metadata.topic_name      = "t";
  metadata.encoding        = "protobuf";
  metadata.type_name       = "Type";
  metadata.direction       = eCAL::tracing::topic_direction::publisher;

  eCAL::tracing::SPublisherSpanData pub{};
  pub.op_type      = eCAL::tracing::operation_type::send;
  pub.entity_id    = 10;
  pub.process_id   = 11;
  pub.payload_size = 12;
  pub.clock        = 13;
  pub.layer        = eCAL::tracing::tl_trace_shm;
  pub.start_ns     = 14;
  pub.end_ns       = 15;

  eCAL::tracing::SSubscriberSpanData sub{};
  sub.op_type      = eCAL::tracing::operation_type::receive;
  sub.entity_id    = 20;
  sub.topic_id     = 21;
  sub.process_id   = 22;
  sub.payload_size = 23;
  sub.clock        = 24;
  sub.layer        = eCAL::tracing::tl_trace_udp;
  sub.start_ns     = 25;
  sub.end_ns       = 26;

  source.emplace_back(metadata);
  source.emplace_back(pub);
  source.emplace_back(sub);

  std::string buffer;
  ASSERT_TRUE(eCAL::protozero::SerializeToBuffer(source, buffer));

  std::vector<eCAL::protozero::TraceRecordVariant> target;
  ASSERT_TRUE(eCAL::protozero::DeserializeFromBuffer(buffer.data(), buffer.size(), target));

  ASSERT_EQ(target.size(), source.size());
  EXPECT_TRUE(std::holds_alternative<eCAL::tracing::STopicMetadata>(target[0]));
  EXPECT_TRUE(std::holds_alternative<eCAL::tracing::SPublisherSpanData>(target[1]));
  EXPECT_TRUE(std::holds_alternative<eCAL::tracing::SSubscriberSpanData>(target[2]));
}
