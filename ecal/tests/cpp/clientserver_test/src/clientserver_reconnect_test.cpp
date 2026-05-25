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

#include <ecal/ecal.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace
{
  constexpr auto kRegistrationRefresh = std::chrono::milliseconds(1000);

  bool WaitForInstanceCountAtLeast(const eCAL::CServiceClient& client, size_t minimum_count, std::chrono::milliseconds timeout)
  {
    const auto end_time = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < end_time)
    {
      if (client.GetClientInstances().size() >= minimum_count)
      {
        return true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
  }

  std::pair<int, int> CountExecutedAndConnectionClosed(const eCAL::ServiceResponseVecT& responses)
  {
    int executed_count = 0;
    int connection_closed_count = 0;

    for (const auto& response : responses)
    {
      if (response.call_state == eCAL::eCallState::executed)
      {
        ++executed_count;
      }

      if ((response.call_state == eCAL::eCallState::failed) && (response.error_msg.find("Connection closed") != std::string::npos))
      {
        ++connection_closed_count;
      }
    }

    return { executed_count, connection_closed_count };
  }
}

TEST(core_cpp_clientserver, ReconnectAfterServerRestart_NoStaleConnectionClosedResponses)
{
  eCAL::Initialize("clientserver reconnect regression test");

  const std::string service_name = "service_reconnect_regression";
  const eCAL::SServiceMethodInformation method_info{ "mymethod", {}, {} };

  auto make_server = [&method_info, &service_name](const std::string& server_name)
  {
    auto server = std::make_shared<eCAL::CServiceServer>(service_name);
    server->SetMethodCallback(method_info
      , [server_name](const eCAL::SServiceMethodInformation& /*method_info_*/, const std::string& request_, std::string& response_) -> int
        {
          response_ = server_name + " response on " + request_;
          return 1;
        });
    return server;
  };

  eCAL::CServiceClient client(service_name, eCAL::ServiceMethodInformationSetT());

  std::vector<std::shared_ptr<eCAL::CServiceServer>> servers;
  servers.emplace_back(make_server("server_1_round_1"));
  servers.emplace_back(make_server("server_2_round_1"));

  ASSERT_TRUE(WaitForInstanceCountAtLeast(client, 2, 4 * kRegistrationRefresh));

  eCAL::ServiceResponseVecT initial_responses;
  ASSERT_TRUE(client.CallWithResponse("mymethod", "hello", initial_responses, 2000));
  auto initial_counts = CountExecutedAndConnectionClosed(initial_responses);
  EXPECT_EQ(initial_counts.first, 2);
  EXPECT_EQ(initial_counts.second, 0);

  servers.clear();

  // Wait for unregister path and failed state propagation.
  eCAL::Process::SleepMS(static_cast<int>(3 * kRegistrationRefresh.count()));

  servers.emplace_back(make_server("server_1_round_2"));
  servers.emplace_back(make_server("server_2_round_2"));

  ASSERT_TRUE(WaitForInstanceCountAtLeast(client, 2, 4 * kRegistrationRefresh));

  eCAL::ServiceResponseVecT responses_after_restart;
  client.CallWithResponse("mymethod", "hello", responses_after_restart, 2000);

  const auto counts_after_restart = CountExecutedAndConnectionClosed(responses_after_restart);

  // Regression assertion:
  // After reconnecting to restarted servers, there should be no stale
  // "Connection closed" responses from old sessions.
  EXPECT_EQ(counts_after_restart.second, 0);
  EXPECT_GE(counts_after_restart.first, 2);

  eCAL::Finalize();
}
