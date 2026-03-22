// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 epsilonbsp

#ifndef SM_BZIP2_EXTENSION_H_
#define SM_BZIP2_EXTENSION_H_

#include "smsdk_ext.h"
#include <vector>

struct Bzip2_Task;

class Bzip2 : public SDKExtension {
public:
    virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
    virtual void SDK_OnUnload();
    virtual void SDK_OnAllLoaded();

    // Called from the worker thread to queue a finished task.
    void QueueCompletedTask(Bzip2_Task *task);

    // Called each game frame to fire pending callbacks on the main thread.
    void ProcessCompletedTasks();

private:
    IMutex*                  m_mutex = nullptr;
    std::vector<Bzip2_Task*> m_completed_tasks;
};

#endif // SM_BZIP2_EXTENSION_H_
