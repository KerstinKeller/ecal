/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2026 AUMOVIO and subsidiaries. All rights reserved.
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

#include <gtest/gtest.h>

#include "registration/ecal_registration_database.h"

namespace
{
  eCAL::Registration::Sample CreatePublisherSample(uint64_t entity_id, int32_t process_id, const std::string& host, const std::string& topic_name)
  {
    eCAL::Registration::Sample sample;
    sample.cmd_type = bct_reg_publisher;
    sample.identifier.entity_id = entity_id;
    sample.identifier.process_id = process_id;
    sample.identifier.host_name = host;
    sample.topic.topic_name = topic_name;
    sample.topic.datatype_information.name = "demo::Type";
    sample.topic.registration_clock = 10;
    sample.topic.topic_size = 128;
    sample.topic.connections_local = 1;
    sample.topic.connections_external = 2;
    sample.topic.message_drops = 3;
    sample.topic.data_id = 7;
    sample.topic.data_clock = 8;
    sample.topic.data_frequency = 900;
    sample.topic.latency_us.latest = 12.0;
    return sample;
  }
}

TEST(registration_database_test, sample_and_direct_mutation_are_equivalent_for_publisher)
{
  eCAL::Registration::CEcalRegistrationDatabase sample_db;
  eCAL::Registration::CEcalRegistrationDatabase direct_db;

  const auto sample = CreatePublisherSample(42, 1001, "host_a", "topic_foo");
  sample_db.ApplySample(sample);

  eCAL::Registration::CEcalRegistrationDatabase::TopicRegistrationDelta reg_delta;
  reg_delta.process_id = sample.identifier.process_id;
  reg_delta.host_name = sample.identifier.host_name;
  reg_delta.topic = sample.topic;

  eCAL::Registration::CEcalRegistrationDatabase::TopicMonitoringDelta mon_delta;
  mon_delta.registration_clock = sample.topic.registration_clock;
  mon_delta.topic_size = sample.topic.topic_size;
  mon_delta.connections_local = sample.topic.connections_local;
  mon_delta.connections_external = sample.topic.connections_external;
  mon_delta.message_drops = sample.topic.message_drops;
  mon_delta.data_id = sample.topic.data_id;
  mon_delta.data_clock = sample.topic.data_clock;
  mon_delta.data_frequency = sample.topic.data_frequency;
  mon_delta.latency_us = sample.topic.latency_us;

  direct_db.AddOrUpdatePublisher(sample.identifier.entity_id, reg_delta);
  direct_db.UpdatePublisherMonitoring(sample.identifier.entity_id, mon_delta);

  const auto sample_snap = sample_db.GetSnapshot();
  const auto direct_snap = direct_db.GetSnapshot();

  EXPECT_EQ(sample_snap.PublisherCount(), direct_snap.PublisherCount());
  EXPECT_TRUE(sample_snap.HasPublisher(sample.identifier.entity_id));
  EXPECT_TRUE(direct_snap.HasPublisher(sample.identifier.entity_id));

  eCAL::Registration::CEcalRegistrationDatabase::TopicRegistrationDelta sample_reg;
  eCAL::Registration::CEcalRegistrationDatabase::TopicRegistrationDelta direct_reg;
  ASSERT_TRUE(sample_snap.GetPublisherRegistration(sample.identifier.entity_id, sample_reg));
  ASSERT_TRUE(direct_snap.GetPublisherRegistration(sample.identifier.entity_id, direct_reg));

  EXPECT_EQ(sample_reg.process_id, direct_reg.process_id);
  EXPECT_EQ(sample_reg.host_name, direct_reg.host_name);
  EXPECT_EQ(sample_reg.topic, direct_reg.topic);

  eCAL::Registration::CEcalRegistrationDatabase::TopicMonitoringDelta sample_mon;
  eCAL::Registration::CEcalRegistrationDatabase::TopicMonitoringDelta direct_mon;
  ASSERT_TRUE(sample_snap.GetPublisherMonitoring(sample.identifier.entity_id, sample_mon));
  ASSERT_TRUE(direct_snap.GetPublisherMonitoring(sample.identifier.entity_id, direct_mon));

  EXPECT_EQ(sample_mon.registration_clock, direct_mon.registration_clock);
  EXPECT_EQ(sample_mon.topic_size, direct_mon.topic_size);
  EXPECT_EQ(sample_mon.connections_local, direct_mon.connections_local);
  EXPECT_EQ(sample_mon.connections_external, direct_mon.connections_external);
  EXPECT_EQ(sample_mon.message_drops, direct_mon.message_drops);
  EXPECT_EQ(sample_mon.data_id, direct_mon.data_id);
  EXPECT_EQ(sample_mon.data_clock, direct_mon.data_clock);
  EXPECT_EQ(sample_mon.data_frequency, direct_mon.data_frequency);
  EXPECT_EQ(sample_mon.latency_us, direct_mon.latency_us);
}

TEST(registration_database_test, remove_process_cascades_all_registered_members)
{
  eCAL::Registration::CEcalRegistrationDatabase db;

  eCAL::Registration::CEcalRegistrationDatabase::ProcessRegistrationDelta process_delta;
  process_delta.host_name = "host_a";
  process_delta.process.process_name = "proc_a";
  db.AddOrUpdateProcess(1001, process_delta);

  eCAL::Registration::CEcalRegistrationDatabase::TopicRegistrationDelta topic_delta;
  topic_delta.process_id = 1001;
  topic_delta.host_name = "host_a";
  topic_delta.topic.topic_name = "topic_foo";
  db.AddOrUpdatePublisher(10, topic_delta);
  db.AddOrUpdateSubscriber(11, topic_delta);

  eCAL::Registration::CEcalRegistrationDatabase::ServiceRegistrationDelta service_delta;
  service_delta.process_id = 1001;
  service_delta.host_name = "host_a";
  service_delta.service.service_name = "service_foo";
  db.AddOrUpdateServer(12, service_delta);

  eCAL::Registration::CEcalRegistrationDatabase::ClientRegistrationDelta client_delta;
  client_delta.process_id = 1001;
  client_delta.host_name = "host_a";
  client_delta.client.service_name = "service_foo";
  db.AddOrUpdateClient(13, client_delta);

  auto before = db.GetSnapshot();
  ASSERT_EQ(before.ProcessCount(), 1u);
  ASSERT_EQ(before.PublisherCount(), 1u);
  ASSERT_EQ(before.SubscriberCount(), 1u);
  ASSERT_EQ(before.ServerCount(), 1u);
  ASSERT_EQ(before.ClientCount(), 1u);

  db.RemoveProcess(1001);

  auto after = db.GetSnapshot();
  EXPECT_EQ(after.ProcessCount(), 0u);
  EXPECT_EQ(after.PublisherCount(), 0u);
  EXPECT_EQ(after.SubscriberCount(), 0u);
  EXPECT_EQ(after.ServerCount(), 0u);
  EXPECT_EQ(after.ClientCount(), 0u);
}

TEST(registration_database_test, tracks_current_and_previous_revision)
{
  eCAL::Registration::CEcalRegistrationDatabase db;

  EXPECT_EQ(db.CurrentRevision(), 0u);
  EXPECT_EQ(db.PreviousRevision(), 0u);

  eCAL::Registration::CEcalRegistrationDatabase::TopicRegistrationDelta reg_delta;
  reg_delta.process_id = 1;
  reg_delta.host_name = "host";
  reg_delta.topic.topic_name = "topic";
  db.AddOrUpdatePublisher(5, reg_delta);
  EXPECT_EQ(db.CurrentRevision(), 1u);
  EXPECT_EQ(db.PreviousRevision(), 0u);

  eCAL::Registration::CEcalRegistrationDatabase::TopicMonitoringDelta mon_delta;
  mon_delta.registration_clock = 5;
  db.UpdatePublisherMonitoring(5, mon_delta);
  EXPECT_EQ(db.CurrentRevision(), 2u);
  EXPECT_EQ(db.PreviousRevision(), 1u);

  db.RemovePublisher(5);
  EXPECT_EQ(db.CurrentRevision(), 3u);
  EXPECT_EQ(db.PreviousRevision(), 2u);

  db.RemovePublisher(5);
  EXPECT_EQ(db.CurrentRevision(), 3u);
  EXPECT_EQ(db.PreviousRevision(), 2u);
}
