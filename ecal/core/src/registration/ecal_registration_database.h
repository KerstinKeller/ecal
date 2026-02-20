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

#pragma once

#include "serialization/ecal_struct_sample_registration.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace eCAL
{
  namespace Registration
  {
    class CEcalRegistrationDatabase
    {
    private:
      struct State;

    public:
      using Revision = uint64_t;
      using EntityKey = uint64_t;
      using ProcessKey = int32_t;

      enum class EntityType { process, publisher, subscriber, server, client };
      enum class EventType { new_entity, updated_entity, deleted_entity };

      struct EntityEvent
      {
        EventType event_type{ EventType::updated_entity };
        EntityType entity_type{ EntityType::process };
        EntityKey entity_key{ 0 };
      };

      struct ApplyResult
      {
        Revision new_revision{ 0 };
        std::vector<EntityEvent> events;
      };

      struct ProcessRegistrationDelta { Registration::Process process; std::string host_name; };
      struct ProcessMonitoringDelta { Registration::ProcessState state; Registration::eTimeSyncState time_sync_state{ Registration::eTimeSyncState::tsync_none }; };

      struct TopicRegistrationDelta { ProcessKey process_id{ 0 }; std::string host_name; Registration::Topic topic; };
      struct TopicMonitoringDelta
      {
        int32_t registration_clock{ 0 };
        int32_t topic_size{ 0 };
        int32_t connections_local{ 0 };
        int32_t connections_external{ 0 };
        int32_t message_drops{ 0 };
        int64_t data_id{ 0 };
        int64_t data_clock{ 0 };
        int32_t data_frequency{ 0 };
        Registration::Statistics latency_us;
      };

      struct ServiceRegistrationDelta { ProcessKey process_id{ 0 }; std::string host_name; Service::Service service; };
      struct ServiceMonitoringDelta { int32_t registration_clock{ 0 }; };

      struct ClientRegistrationDelta { ProcessKey process_id{ 0 }; std::string host_name; Service::Client client; };
      struct ClientMonitoringDelta { int32_t registration_clock{ 0 }; };

      class Snapshot
      {
      public:
        Snapshot();
        Revision GetRevision() const;
        bool HasProcess(ProcessKey key) const;
        bool HasPublisher(EntityKey key) const;
        bool HasSubscriber(EntityKey key) const;
        bool HasServer(EntityKey key) const;
        bool HasClient(EntityKey key) const;
        size_t ProcessCount() const;
        size_t PublisherCount() const;
        size_t SubscriberCount() const;
        size_t ServerCount() const;
        size_t ClientCount() const;
        bool GetPublisherRegistration(EntityKey key, TopicRegistrationDelta& out) const;
        bool GetPublisherMonitoring(EntityKey key, TopicMonitoringDelta& out) const;

      private:
        friend class CEcalRegistrationDatabase;
        explicit Snapshot(std::shared_ptr<const State> state);
        std::shared_ptr<const State> state_;
      };

      CEcalRegistrationDatabase();

      ApplyResult ApplySample(const Registration::Sample& sample_);
      ApplyResult ApplySamples(const Registration::SampleList& samples_);

      ApplyResult AddOrUpdateProcess(ProcessKey key_, const ProcessRegistrationDelta& delta_);
      ApplyResult UpdateProcessMonitoring(ProcessKey key_, const ProcessMonitoringDelta& delta_);
      ApplyResult RemoveProcess(ProcessKey key_);

      ApplyResult AddOrUpdatePublisher(EntityKey key_, const TopicRegistrationDelta& delta_);
      ApplyResult UpdatePublisherMonitoring(EntityKey key_, const TopicMonitoringDelta& delta_);
      ApplyResult RemovePublisher(EntityKey key_);

      ApplyResult AddOrUpdateSubscriber(EntityKey key_, const TopicRegistrationDelta& delta_);
      ApplyResult UpdateSubscriberMonitoring(EntityKey key_, const TopicMonitoringDelta& delta_);
      ApplyResult RemoveSubscriber(EntityKey key_);

      ApplyResult AddOrUpdateServer(EntityKey key_, const ServiceRegistrationDelta& delta_);
      ApplyResult UpdateServerMonitoring(EntityKey key_, const ServiceMonitoringDelta& delta_);
      ApplyResult RemoveServer(EntityKey key_);

      ApplyResult AddOrUpdateClient(EntityKey key_, const ClientRegistrationDelta& delta_);
      ApplyResult UpdateClientMonitoring(EntityKey key_, const ClientMonitoringDelta& delta_);
      ApplyResult RemoveClient(EntityKey key_);

      Snapshot GetSnapshot() const;
      Revision CurrentRevision() const;
      Revision PreviousRevision() const;

    private:
      struct State
      {
        Revision revision{ 0 };

        std::unordered_map<ProcessKey, ProcessRegistrationDelta> processes;
        std::unordered_map<ProcessKey, ProcessMonitoringDelta> process_monitoring;

        std::unordered_map<EntityKey, TopicRegistrationDelta> publishers;
        std::unordered_map<EntityKey, TopicMonitoringDelta> publisher_monitoring;

        std::unordered_map<EntityKey, TopicRegistrationDelta> subscribers;
        std::unordered_map<EntityKey, TopicMonitoringDelta> subscriber_monitoring;

        std::unordered_map<EntityKey, ServiceRegistrationDelta> servers;
        std::unordered_map<EntityKey, ServiceMonitoringDelta> server_monitoring;

        std::unordered_map<EntityKey, ClientRegistrationDelta> clients;
        std::unordered_map<EntityKey, ClientMonitoringDelta> client_monitoring;

        struct ProcessMembers
        {
          std::unordered_set<EntityKey> publishers;
          std::unordered_set<EntityKey> subscribers;
          std::unordered_set<EntityKey> servers;
          std::unordered_set<EntityKey> clients;
        };

        std::unordered_map<ProcessKey, ProcessMembers> members_by_process;
      };

      using MutatorT = std::function<void(State&, std::vector<EntityEvent>&, bool&)>;
      ApplyResult ApplyMutation(const MutatorT& mutator_);

      static void EnsureProcessMembership(State& state_, ProcessKey process_key_);
      static void AddMembership(std::unordered_map<ProcessKey, State::ProcessMembers>& map_, ProcessKey process_key_, EntityType entity_type_, EntityKey key_);
      static void RemoveMembership(std::unordered_map<ProcessKey, State::ProcessMembers>& map_, ProcessKey process_key_, EntityType entity_type_, EntityKey key_);

      mutable std::mutex mutex_;
      std::shared_ptr<const State> current_state_;
      Revision previous_revision_{ 0 };
    };
  }
}
