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

#include "reader_attribute_builder.h"
#include "ecal/process.h"
#include "ecal/config.h"

namespace eCAL
{
  namespace
  {
    eCAL::Types::SynchronizationMutexType ResolveCompiledDefaultSynchronizationMutexType()
    {
#if defined(ECAL_OS_LINUX) && defined(ECAL_USE_CLOCKLOCK_MUTEX) && defined(ECAL_HAS_CLOCKLOCK_MUTEX)
      return eCAL::Types::SynchronizationMutexType::robust_mutex_v1;
#else
      return eCAL::Types::SynchronizationMutexType::mutex_v1;
#endif
    }

    eCAL::Types::SynchronizationMutexType ResolveSynchronizationMutexType(const eCAL::Configuration& config_)
    {
      if (config_.transport_layer.shm.synchronization_mutex_type == eCAL::TransportLayer::SHM::SynchronizationMutexType::mutex_v1)
        return eCAL::Types::SynchronizationMutexType::mutex_v1;
      if (config_.transport_layer.shm.synchronization_mutex_type == eCAL::TransportLayer::SHM::SynchronizationMutexType::robust_mutex_v1)
        return eCAL::Types::SynchronizationMutexType::robust_mutex_v1;

      return ResolveCompiledDefaultSynchronizationMutexType();
    }
  }

  eCALReader::SAttributes BuildReaderAttributes(const std::string& topic_name_, const eCAL::Configuration& config_)
  {
    const auto& subscriber_config      = config_.subscriber;
    const auto& transport_layer_config = config_.transport_layer;
    const auto& registration_config    = config_.registration;

    eCALReader::SAttributes attributes;

    attributes.network_enabled            = config_.communication_mode == eCAL::eCommunicationMode::network;
    attributes.loopback                   = registration_config.loopback;
    attributes.drop_out_of_order_messages = subscriber_config.drop_out_of_order_messages;
    attributes.registration_timeout_ms    = registration_config.registration_timeout;
    attributes.topic_name                 = topic_name_;
    attributes.host_name                  = Process::GetHostName();
    attributes.shm_transport_domain       = Process::GetShmTransportDomain();
    attributes.process_id                 = Process::GetProcessID();
    attributes.process_name               = Process::GetProcessName();
    attributes.unit_name                  = Process::GetUnitName();

    attributes.udp.enable        = subscriber_config.layer.udp.enable;
    attributes.udp.broadcast     = config_.communication_mode == eCAL::eCommunicationMode::local;
    attributes.udp.port          = transport_layer_config.udp.port;
    attributes.udp.receivebuffer = transport_layer_config.udp.receive_buffer;
    
    switch (config_.communication_mode)
    {
      case eCAL::eCommunicationMode::network:
        attributes.udp.group = transport_layer_config.udp.network.group;
        break;
      case eCAL::eCommunicationMode::local:
        attributes.udp.group = transport_layer_config.udp.local.group;
        break;
      default:
        break;
    }

    attributes.tcp.enable                    = subscriber_config.layer.tcp.enable;
    attributes.tcp.thread_pool_size          = transport_layer_config.tcp.number_executor_reader;
    attributes.tcp.max_reconnection_attempts = transport_layer_config.tcp.max_reconnections;
    
    attributes.shm.enable = subscriber_config.layer.shm.enable;
    attributes.shm.synchronization_mutex_type = ResolveSynchronizationMutexType(config_);
    
    return attributes;
  }
}
