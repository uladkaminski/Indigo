/****************************************************************************
 * Copyright (C) from 2009 to Present EPAM Systems.
 *
 * This file is part of Indigo toolkit.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#ifndef __os_sync_wrapper_h__
#define __os_sync_wrapper_h__

#include <mutex>
#include <condition_variable>

#include "base_c/defs.h"
#include "base_cpp/exception.h"

namespace indigo
{

    //
    // Semaphore
    //
    class DLLEXPORT OsSemaphore
    {
    public:
        OsSemaphore(int initial_count, int max_count);
        ~OsSemaphore();

        void Wait();
        void Post();

    private:
        std::mutex _mutex;
        std::condition_variable _cond;
        int _count;
        const int _max_count;
    };

    //
    // Message system
    //

    class OsMessageSystem
    {
    public:
        OsMessageSystem();

        void SendMsg(int message, void* param = 0);
        void RecvMsg(int* message, void** result = 0);

    private:
        OsSemaphore _sendSem;
        OsSemaphore _finishRecvSem;

        std::mutex _sendLock;
        std::mutex _recvLock;

        volatile int _localMessage;
        void* volatile _localParam;
    };

    //
    // Thread-safe static local variables initialization object
    //

    DLLEXPORT std::mutex& osStaticObjConstructionLock();

    // Local static variables with constructors should have std::mutex
    // guard to avoid thread conflicts.
    // This object should be declared as ONLY static object because
    // _was_created variable should be zero by default.
    template <typename T> class ThreadSafeStaticObj
    {
    public:
        ~ThreadSafeStaticObj()
        {
            if (_was_created)
            {
                _obj->~T();
                _obj = 0;
                _was_created = false;
            }
        }

        T* ptr()
        {
            return _ptr();
        }
        T& ref()
        {
            return *_ptr();
        }
        T* operator->()
        {
            return ptr();
        }

    private:
        void _ensureInitialized()
        {
            if (!_was_created)
            {
                std::lock_guard<std::mutex> locker(osStaticObjConstructionLock());

                if (!_was_created)
                {
                    _obj = new ((void*)_obj_data) T;
                    _was_created = true;
                }
            }
        }

        T* _ptr()
        {
            _ensureInitialized();
            return _obj;
        }

        T* _obj;
        char _obj_data[sizeof(T)];
        volatile bool _was_created; // Zero for static objects
    };

} // namespace indigo

#endif // __os_sync_wrapper_h__
