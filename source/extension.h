// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 EpsilonBSP

#ifndef SM_BZIP2_EXTENSION_H_
#define SM_BZIP2_EXTENSION_H_

#include "smsdk_ext.h"
#include <vector>

struct BZ2Task;

class Bzip2 : public SDKExtension {
public:
    virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();

    // Called from the worker thread to queue a finished task.
    void QueueCompletedTask(BZ2Task *task);

    // Called each game frame to fire pending callbacks on the main thread.
    void ProcessCompletedTasks();

private:
    IMutex                  *m_Mutex = nullptr;
    std::vector<BZ2Task *>   m_CompletedTasks;
};

#endif // SM_BZIP2_EXTENSION_H_
