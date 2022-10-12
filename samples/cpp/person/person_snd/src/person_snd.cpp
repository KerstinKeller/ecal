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
  std::thread run_publisher(const std::string& name, std::chrono::milliseconds sleep_time, const std::atomic<bool>& pub_stop)
  {
    return std::thread([&pub_stop, sleep_time, name]() {
      eCAL::string::CPublisher<std::string> publisher(name);
      while (!pub_stop)
      {
        publisher.Send("ABCD");
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
      }
      });
  }
}

int main()
{
  // initialize eCAL API
  eCAL::Initialize(0, nullptr, "Provoke Datalosses");

  std::atomic<bool> stop(false);
  std::thread pub_foo(run_publisher("foo", std::chrono::milliseconds(10), stop));
  std::thread pub_bar(run_publisher("bar", std::chrono::milliseconds(10), stop));
  std::thread pub_baz(run_publisher("baz", std::chrono::milliseconds(10), stop));
  std::thread pub_bli(run_publisher("bli", std::chrono::milliseconds(10), stop));
  std::thread pub_blub(run_publisher("blub", std::chrono::milliseconds(10), stop));

  // let them work together
  while (eCAL::Ok())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // stop publishing thread
  stop = true;
  pub_foo.join();
  pub_bar.join();
  pub_baz.join();
  pub_bli.join();
  pub_blub.join();

  // finalize eCAL API
  // without destroying any pub / sub
  eCAL::Finalize();
}