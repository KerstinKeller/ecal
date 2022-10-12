/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
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
#include <ecal/msg/string/publisher.h>
#include <ecal/msg/string/subscriber.h>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>


namespace {
  // subscriber callback function
  void OnReceive(std::atomic<bool>& alive)
  {
    alive = true;
  }

  void OtherReceive(std::atomic<bool>& alive)
  {
    alive = true;
  }

  std::thread switch_subscribers(std::atomic<bool>& alive, const std::atomic<bool>& stop)
  {
    return std::thread([&]() {
      std::vector<std::string> names{ "foo", "bar"};
      std::shared_ptr<eCAL::string::CSubscriber<std::string>> subscriber;
      while (!stop)
      {
        for (const auto& name : names)
        {
          subscriber.reset();
          subscriber = std::make_shared< eCAL::string::CSubscriber<std::string>>(name);
          auto callback = [&alive](const char*, const std::string&, long long, long long, long long)
          {
            OnReceive(alive);
          };
          subscriber->AddReceiveCallback(callback);
          std::this_thread::sleep_for(std::chrono::seconds(2));
          if (stop) return;
        }
      }
      });

  }

  std::thread fix_subscriber(std::atomic<bool>& alive, const std::atomic<bool>& stop)
  {
    return std::thread([&]() {
      std::shared_ptr<eCAL::string::CSubscriber<std::string>> subscriber;
      subscriber = std::make_shared< eCAL::string::CSubscriber<std::string>>("foo");
      auto callback = [&alive](const char*, const std::string&, long long, long long, long long)
      {
        OtherReceive(alive);
      };
      subscriber->AddReceiveCallback(callback);

      while (!stop)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      });

  }


  std::thread check_callback_executed(std::atomic<bool>& alive, const std::atomic<bool>& stop)
  {
    return std::thread([&]() {
      while (!stop)
      {
        if (!alive)
        {
          std::cout << "Publishers not alive!!!" << std::endl;
        }
        alive = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      });
  }



}
int main()
{
  // initialize eCAL API
  eCAL::Initialize(0, nullptr, "Provoke Datalosses Subscriber");


  std::atomic<bool> stop(false);
  std::atomic<bool> callbacks_alive(true);
  std::atomic<bool> other_callback_alive(true);
  std::thread subscriber(switch_subscribers(callbacks_alive, stop));
  std::thread fix_subscriber(fix_subscriber(other_callback_alive, stop));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::thread supervisor(check_callback_executed(callbacks_alive, stop));
  std::thread supervisor_2(check_callback_executed(other_callback_alive, stop));

  // let them work together
  while (eCAL::Ok())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  stop = true;
  subscriber.join();
  fix_subscriber.join();
  supervisor.join();
  supervisor_2.join();

  // finalize eCAL API
  // without destroying any pub / sub
  eCAL::Finalize();

}