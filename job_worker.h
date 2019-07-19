#ifndef JOB_WORKER_H
#define JOB_WORKER_H

#include "job_system.h"
#include <atomic>
#include <random>
#include <thread>

struct Job;
class JobSystem;


class Worker
{
public:
    Worker(uint8_t worker_idx, uint8_t num_workers, JobSystem& job_system, WorkStealingQueue<>** queues);
    ~Worker();

    void set_active(bool active);

    //NOTE: Not threadsafe, must be called from worker threads thread
    void run(Job* job);

    bool is_empty_job(Job* job);

    void thread_function();

    void fetch_and_execute();

    WorkStealingQueue<>* get_queue();

    std::thread& get_thread();
    void set_thread(std::thread&& thread);
private:

    void execute_job(Job* job);

    Job* get_job();

    void finish(Job* job);

    JobSystem& m_job_system;
    std::mt19937 m_rng;
    bool m_active = false;
    uint8_t m_worker_idx;
    uint8_t m_num_workers;
    WorkStealingQueue<>** m_queues;
    std::thread m_thread;
    uint32_t m_jobs_completed = 0;
    uint32_t m_steal_from_queue = 0;
};


#endif // JOB_WORKER_H
