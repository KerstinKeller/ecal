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

#include "registration/ecal_registration_database.h"

#include <utility>

namespace eCAL
{
  namespace Registration
  {
    CEcalRegistrationDatabase::Snapshot::Snapshot()
      : state_(std::make_shared<State>())
    {}

    CEcalRegistrationDatabase::Snapshot::Snapshot(std::shared_ptr<const State> state)
      : state_(std::move(state))
    {}

    CEcalRegistrationDatabase::Revision CEcalRegistrationDatabase::Snapshot::GetRevision() const { return state_->revision; }
    bool CEcalRegistrationDatabase::Snapshot::HasProcess(ProcessKey key) const { return state_->processes.find(key) != state_->processes.end(); }
    bool CEcalRegistrationDatabase::Snapshot::HasPublisher(EntityKey key) const { return state_->publishers.find(key) != state_->publishers.end(); }
    bool CEcalRegistrationDatabase::Snapshot::HasSubscriber(EntityKey key) const { return state_->subscribers.find(key) != state_->subscribers.end(); }
    bool CEcalRegistrationDatabase::Snapshot::HasServer(EntityKey key) const { return state_->servers.find(key) != state_->servers.end(); }
    bool CEcalRegistrationDatabase::Snapshot::HasClient(EntityKey key) const { return state_->clients.find(key) != state_->clients.end(); }
    size_t CEcalRegistrationDatabase::Snapshot::ProcessCount() const { return state_->processes.size(); }
    size_t CEcalRegistrationDatabase::Snapshot::PublisherCount() const { return state_->publishers.size(); }
    size_t CEcalRegistrationDatabase::Snapshot::SubscriberCount() const { return state_->subscribers.size(); }
    size_t CEcalRegistrationDatabase::Snapshot::ServerCount() const { return state_->servers.size(); }
    size_t CEcalRegistrationDatabase::Snapshot::ClientCount() const { return state_->clients.size(); }

    bool CEcalRegistrationDatabase::Snapshot::GetPublisherRegistration(EntityKey key, TopicRegistrationDelta& out) const
    {
      auto it = state_->publishers.find(key);
      if (it == state_->publishers.end()) return false;
      out = it->second;
      return true;
    }

    bool CEcalRegistrationDatabase::Snapshot::GetPublisherMonitoring(EntityKey key, TopicMonitoringDelta& out) const
    {
      auto it = state_->publisher_monitoring.find(key);
      if (it == state_->publisher_monitoring.end()) return false;
      out = it->second;
      return true;
    }

    CEcalRegistrationDatabase::CEcalRegistrationDatabase()
      : current_state_(std::make_shared<State>())
    {}

    void CEcalRegistrationDatabase::EnsureProcessMembership(State& state_, ProcessKey process_key_)
    {
      if (state_.members_by_process.find(process_key_) == state_.members_by_process.end())
        state_.members_by_process.emplace(process_key_, State::ProcessMembers{});
    }

    void CEcalRegistrationDatabase::AddMembership(std::unordered_map<ProcessKey, State::ProcessMembers>& map_, ProcessKey process_key_, EntityType entity_type_, EntityKey key_)
    {
      auto& members = map_[process_key_];
      switch (entity_type_)
      {
      case EntityType::publisher: members.publishers.insert(key_); break;
      case EntityType::subscriber: members.subscribers.insert(key_); break;
      case EntityType::server: members.servers.insert(key_); break;
      case EntityType::client: members.clients.insert(key_); break;
      case EntityType::process: break;
      }
    }

    void CEcalRegistrationDatabase::RemoveMembership(std::unordered_map<ProcessKey, State::ProcessMembers>& map_, ProcessKey process_key_, EntityType entity_type_, EntityKey key_)
    {
      auto it = map_.find(process_key_);
      if (it == map_.end()) return;

      switch (entity_type_)
      {
      case EntityType::publisher: it->second.publishers.erase(key_); break;
      case EntityType::subscriber: it->second.subscribers.erase(key_); break;
      case EntityType::server: it->second.servers.erase(key_); break;
      case EntityType::client: it->second.clients.erase(key_); break;
      case EntityType::process: break;
      }

      if (it->second.publishers.empty() && it->second.subscribers.empty() && it->second.servers.empty() && it->second.clients.empty())
        map_.erase(it);
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::ApplyMutation(const MutatorT& mutator_)
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      auto next_state = std::make_shared<State>(*current_state_);

      std::vector<EntityEvent> events;
      bool changed = false;
      mutator_(*next_state, events, changed);

      if (!changed)
        return { current_state_->revision, std::move(events) };

      previous_revision_ = current_state_->revision;
      next_state->revision = current_state_->revision + 1;
      current_state_ = next_state;
      return { current_state_->revision, std::move(events) };
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::AddOrUpdateProcess(ProcessKey key_, const ProcessRegistrationDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto it = state_.processes.find(key_);
        if (it == state_.processes.end())
        {
          state_.processes.emplace(key_, delta_);
          EnsureProcessMembership(state_, key_);
          events_.push_back({ EventType::new_entity, EntityType::process, static_cast<EntityKey>(key_) });
          changed_ = true;
        }
        else if (!(it->second.process == delta_.process && it->second.host_name == delta_.host_name))
        {
          it->second = delta_;
          events_.push_back({ EventType::updated_entity, EntityType::process, static_cast<EntityKey>(key_) });
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::UpdateProcessMonitoring(ProcessKey key_, const ProcessMonitoringDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>&, bool& changed_)
      {
        auto it = state_.process_monitoring.find(key_);
        if (it == state_.process_monitoring.end() || !(it->second.state == delta_.state && it->second.time_sync_state == delta_.time_sync_state))
        {
          state_.process_monitoring[key_] = delta_;
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::RemoveProcess(ProcessKey key_)
    {
      return ApplyMutation([key_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto members_it = state_.members_by_process.find(key_);
        if (members_it != state_.members_by_process.end())
        {
          const auto pubs = members_it->second.publishers;
          const auto subs = members_it->second.subscribers;
          const auto srvs = members_it->second.servers;
          const auto clis = members_it->second.clients;

          for (auto id : pubs)
          {
            auto it = state_.publishers.find(id);
            if (it != state_.publishers.end())
            {
              RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::publisher, id);
              state_.publishers.erase(it);
              state_.publisher_monitoring.erase(id);
              events_.push_back({ EventType::deleted_entity, EntityType::publisher, id });
              changed_ = true;
            }
          }

          for (auto id : subs)
          {
            auto it = state_.subscribers.find(id);
            if (it != state_.subscribers.end())
            {
              RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::subscriber, id);
              state_.subscribers.erase(it);
              state_.subscriber_monitoring.erase(id);
              events_.push_back({ EventType::deleted_entity, EntityType::subscriber, id });
              changed_ = true;
            }
          }

          for (auto id : srvs)
          {
            auto it = state_.servers.find(id);
            if (it != state_.servers.end())
            {
              RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::server, id);
              state_.servers.erase(it);
              state_.server_monitoring.erase(id);
              events_.push_back({ EventType::deleted_entity, EntityType::server, id });
              changed_ = true;
            }
          }

          for (auto id : clis)
          {
            auto it = state_.clients.find(id);
            if (it != state_.clients.end())
            {
              RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::client, id);
              state_.clients.erase(it);
              state_.client_monitoring.erase(id);
              events_.push_back({ EventType::deleted_entity, EntityType::client, id });
              changed_ = true;
            }
          }
        }

        if (state_.processes.erase(key_) > 0)
        {
          events_.push_back({ EventType::deleted_entity, EntityType::process, static_cast<EntityKey>(key_) });
          changed_ = true;
        }

        state_.process_monitoring.erase(key_);
        state_.members_by_process.erase(key_);
      });
    }

#define ECAL_REGDB_ADD_UPDATE_REMOVE_TOPIC(NAME, TYPE_ENUM, REG_MAP, MON_MAP) \
    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::AddOrUpdate##NAME(EntityKey key_, const TopicRegistrationDelta& delta_) \
    { return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>& events_, bool& changed_) { \
      auto it = state_.REG_MAP.find(key_); \
      if (it == state_.REG_MAP.end()) { state_.REG_MAP.emplace(key_, delta_); AddMembership(state_.members_by_process, delta_.process_id, EntityType::TYPE_ENUM, key_); events_.push_back({ EventType::new_entity, EntityType::TYPE_ENUM, key_ }); changed_ = true; } \
      else if (!(it->second.topic == delta_.topic && it->second.process_id == delta_.process_id && it->second.host_name == delta_.host_name)) { \
        if (it->second.process_id != delta_.process_id) { RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::TYPE_ENUM, key_); AddMembership(state_.members_by_process, delta_.process_id, EntityType::TYPE_ENUM, key_); } \
        it->second = delta_; events_.push_back({ EventType::updated_entity, EntityType::TYPE_ENUM, key_ }); changed_ = true; } \
    }); } \
    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::Update##NAME##Monitoring(EntityKey key_, const TopicMonitoringDelta& delta_) \
    { return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>&, bool& changed_) { \
      auto it = state_.MON_MAP.find(key_); \
      if (it == state_.MON_MAP.end() || !(it->second.registration_clock == delta_.registration_clock && it->second.topic_size == delta_.topic_size && it->second.connections_local == delta_.connections_local && it->second.connections_external == delta_.connections_external && it->second.message_drops == delta_.message_drops && it->second.data_id == delta_.data_id && it->second.data_clock == delta_.data_clock && it->second.data_frequency == delta_.data_frequency && it->second.latency_us == delta_.latency_us)) { state_.MON_MAP[key_] = delta_; changed_ = true; } \
    }); } \
    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::Remove##NAME(EntityKey key_) \
    { return ApplyMutation([key_](State& state_, std::vector<EntityEvent>& events_, bool& changed_) { \
      auto it = state_.REG_MAP.find(key_); if (it != state_.REG_MAP.end()) { RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::TYPE_ENUM, key_); state_.REG_MAP.erase(it); state_.MON_MAP.erase(key_); events_.push_back({ EventType::deleted_entity, EntityType::TYPE_ENUM, key_ }); changed_ = true; } \
    }); }

    ECAL_REGDB_ADD_UPDATE_REMOVE_TOPIC(Publisher, publisher, publishers, publisher_monitoring)
    ECAL_REGDB_ADD_UPDATE_REMOVE_TOPIC(Subscriber, subscriber, subscribers, subscriber_monitoring)

#undef ECAL_REGDB_ADD_UPDATE_REMOVE_TOPIC

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::AddOrUpdateServer(EntityKey key_, const ServiceRegistrationDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto it = state_.servers.find(key_);
        if (it == state_.servers.end())
        {
          state_.servers.emplace(key_, delta_);
          AddMembership(state_.members_by_process, delta_.process_id, EntityType::server, key_);
          events_.push_back({ EventType::new_entity, EntityType::server, key_ });
          changed_ = true;
        }
        else if (!(it->second.service == delta_.service && it->second.process_id == delta_.process_id && it->second.host_name == delta_.host_name))
        {
          if (it->second.process_id != delta_.process_id)
          {
            RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::server, key_);
            AddMembership(state_.members_by_process, delta_.process_id, EntityType::server, key_);
          }
          it->second = delta_;
          events_.push_back({ EventType::updated_entity, EntityType::server, key_ });
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::UpdateServerMonitoring(EntityKey key_, const ServiceMonitoringDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>&, bool& changed_)
      {
        auto it = state_.server_monitoring.find(key_);
        if (it == state_.server_monitoring.end() || it->second.registration_clock != delta_.registration_clock)
        {
          state_.server_monitoring[key_] = delta_;
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::RemoveServer(EntityKey key_)
    {
      return ApplyMutation([key_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto it = state_.servers.find(key_);
        if (it != state_.servers.end())
        {
          RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::server, key_);
          state_.servers.erase(it);
          state_.server_monitoring.erase(key_);
          events_.push_back({ EventType::deleted_entity, EntityType::server, key_ });
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::AddOrUpdateClient(EntityKey key_, const ClientRegistrationDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto it = state_.clients.find(key_);
        if (it == state_.clients.end())
        {
          state_.clients.emplace(key_, delta_);
          AddMembership(state_.members_by_process, delta_.process_id, EntityType::client, key_);
          events_.push_back({ EventType::new_entity, EntityType::client, key_ });
          changed_ = true;
        }
        else if (!(it->second.client == delta_.client && it->second.process_id == delta_.process_id && it->second.host_name == delta_.host_name))
        {
          if (it->second.process_id != delta_.process_id)
          {
            RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::client, key_);
            AddMembership(state_.members_by_process, delta_.process_id, EntityType::client, key_);
          }
          it->second = delta_;
          events_.push_back({ EventType::updated_entity, EntityType::client, key_ });
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::UpdateClientMonitoring(EntityKey key_, const ClientMonitoringDelta& delta_)
    {
      return ApplyMutation([key_, &delta_](State& state_, std::vector<EntityEvent>&, bool& changed_)
      {
        auto it = state_.client_monitoring.find(key_);
        if (it == state_.client_monitoring.end() || it->second.registration_clock != delta_.registration_clock)
        {
          state_.client_monitoring[key_] = delta_;
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::RemoveClient(EntityKey key_)
    {
      return ApplyMutation([key_](State& state_, std::vector<EntityEvent>& events_, bool& changed_)
      {
        auto it = state_.clients.find(key_);
        if (it != state_.clients.end())
        {
          RemoveMembership(state_.members_by_process, it->second.process_id, EntityType::client, key_);
          state_.clients.erase(it);
          state_.client_monitoring.erase(key_);
          events_.push_back({ EventType::deleted_entity, EntityType::client, key_ });
          changed_ = true;
        }
      });
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::ApplySample(const Registration::Sample& sample_)
    {
      const EntityKey entity_key = sample_.identifier.entity_id;
      const ProcessKey process_key = sample_.identifier.process_id;

      switch (sample_.cmd_type)
      {
      case bct_reg_process:
        return AddOrUpdateProcess(process_key, ProcessRegistrationDelta{ sample_.process, sample_.identifier.host_name });
      case bct_unreg_process:
        return RemoveProcess(process_key);
      case bct_reg_publisher:
      {
        auto result = AddOrUpdatePublisher(entity_key, TopicRegistrationDelta{ process_key, sample_.identifier.host_name, sample_.topic });
        auto mon_result = UpdatePublisherMonitoring(entity_key, TopicMonitoringDelta{ sample_.topic.registration_clock, sample_.topic.topic_size, sample_.topic.connections_local, sample_.topic.connections_external, sample_.topic.message_drops, sample_.topic.data_id, sample_.topic.data_clock, sample_.topic.data_frequency, sample_.topic.latency_us });
        result.new_revision = mon_result.new_revision;
        return result;
      }
      case bct_unreg_publisher:
        return RemovePublisher(entity_key);
      case bct_reg_subscriber:
      {
        auto result = AddOrUpdateSubscriber(entity_key, TopicRegistrationDelta{ process_key, sample_.identifier.host_name, sample_.topic });
        auto mon_result = UpdateSubscriberMonitoring(entity_key, TopicMonitoringDelta{ sample_.topic.registration_clock, sample_.topic.topic_size, sample_.topic.connections_local, sample_.topic.connections_external, sample_.topic.message_drops, sample_.topic.data_id, sample_.topic.data_clock, sample_.topic.data_frequency, sample_.topic.latency_us });
        result.new_revision = mon_result.new_revision;
        return result;
      }
      case bct_unreg_subscriber:
        return RemoveSubscriber(entity_key);
      case bct_reg_service:
      {
        auto result = AddOrUpdateServer(entity_key, ServiceRegistrationDelta{ process_key, sample_.identifier.host_name, sample_.service });
        auto mon_result = UpdateServerMonitoring(entity_key, ServiceMonitoringDelta{ sample_.service.registration_clock });
        result.new_revision = mon_result.new_revision;
        return result;
      }
      case bct_unreg_service:
        return RemoveServer(entity_key);
      case bct_reg_client:
      {
        auto result = AddOrUpdateClient(entity_key, ClientRegistrationDelta{ process_key, sample_.identifier.host_name, sample_.client });
        auto mon_result = UpdateClientMonitoring(entity_key, ClientMonitoringDelta{ sample_.client.registration_clock });
        result.new_revision = mon_result.new_revision;
        return result;
      }
      case bct_unreg_client:
        return RemoveClient(entity_key);
      case bct_none:
      case bct_set_sample:
      default:
        return { CurrentRevision(), {} };
      }
    }

    CEcalRegistrationDatabase::ApplyResult CEcalRegistrationDatabase::ApplySamples(const Registration::SampleList& samples_)
    {
      ApplyResult result{ CurrentRevision(), {} };
      for (const auto& sample : samples_)
      {
        auto single = ApplySample(sample);
        result.new_revision = single.new_revision;
        result.events.insert(result.events.end(), single.events.begin(), single.events.end());
      }
      return result;
    }

    CEcalRegistrationDatabase::Snapshot CEcalRegistrationDatabase::GetSnapshot() const
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      return Snapshot(current_state_);
    }

    CEcalRegistrationDatabase::Revision CEcalRegistrationDatabase::CurrentRevision() const
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      return current_state_->revision;
    }

    CEcalRegistrationDatabase::Revision CEcalRegistrationDatabase::PreviousRevision() const
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      return previous_revision_;
    }
  }
}
