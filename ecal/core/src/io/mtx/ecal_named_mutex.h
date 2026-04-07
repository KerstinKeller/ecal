/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2022 Continental Corporation
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

/**
 * @brief  eCAL named mutex
**/

#pragma once

#include <ecal/os.h>

#include <string>
#include <memory>
#include <cstdint>

namespace eCAL
{
  class CNamedMutexImplBase;

  class CNamedMutex
  {
  public:
    enum class eType
    {
#ifdef ECAL_OS_WINDOWS
      winapi_mutex,
#endif
#ifdef ECAL_OS_LINUX
      pthread_mutex,
  #if defined(ECAL_HAS_ROBUST_MUTEX) || defined(ECAL_HAS_CLOCKLOCK_MUTEX)
      pthread_robust_mutex,
  #endif
#endif
    };

    explicit CNamedMutex(const std::string& name_, bool recoverable_ = false);
    CNamedMutex(const std::string& name_, eType mutex_type_, bool recoverable_ = false);
    CNamedMutex();
    ~CNamedMutex();

    CNamedMutex(const CNamedMutex&) = delete;
    CNamedMutex& operator=(const CNamedMutex&) = delete;
    CNamedMutex(CNamedMutex&& named_mutex) noexcept;
    CNamedMutex& operator=(CNamedMutex&& named_mutex)  noexcept;

    bool Create(const std::string& name_, bool recoverable_ = false);
    bool Create(const std::string& name_, eType mutex_type_, bool recoverable_ = false);
    void Destroy();

    bool IsCreated() const;
    bool IsRecoverable() const;
    bool WasRecovered() const;
    bool HasOwnership() const;

    void DropOwnership();

    bool Lock(int64_t timeout_);
    void Unlock();

  private:
    std::unique_ptr<CNamedMutexImplBase> m_impl;
  };
}
