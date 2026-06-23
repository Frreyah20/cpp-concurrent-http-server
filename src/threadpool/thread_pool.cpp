#include "thread_pool.h"

#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

extern std::queue<int> task_queue;
extern std::mutex queue_mutex;
extern std::condition_variable queue_cv;

extern std::atomic<bool> running;

void handleClient(int client_fd);

void workerThread()
{
    while(running)
    {
        int client_fd;

        std::unique_lock<std::mutex> lock(queue_mutex);

        queue_cv.wait(lock, []{
            return !task_queue.empty() || !running;
        });

        if(!running && task_queue.empty())
        {
            return;
        }

        client_fd = task_queue.front();
        task_queue.pop();

        lock.unlock();

        handleClient(client_fd);
    }
}