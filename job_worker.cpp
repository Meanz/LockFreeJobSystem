#include "job_worker.h"
#include <QDebug>

Worker::Worker(uint8_t worker_idx, uint8_t num_workers, JobSystem& job_system,
               WorkStealingQueue<>** queues) :
    m_job_system(job_system),
    m_worker_idx(worker_idx),
    m_num_workers(num_workers),
    m_queues(queues)
{
    m_rng = std::mt19937(1331);
}

Worker::~Worker()
{
    qDebug() << "Worker with idx: " << m_worker_idx << " destroyed. it completed " << m_jobs_completed << " jobs";
}

void Worker::set_active(bool active)
{
    m_active = active;
}

void Worker::run(Job* job)
{
    get_queue()->push(job);
}

bool Worker::is_empty_job(Job* job)
{
    return job == nullptr;
}

void Worker::thread_function()
{
    while(m_active)
    {
        fetch_and_execute();
    }
}

void Worker::fetch_and_execute()
{
    Job* job = get_job();
    if(job)
    {
        execute_job(job);
    }
}

WorkStealingQueue<>* Worker::get_queue()
{
    return m_queues[m_worker_idx];
}

std::thread& Worker::get_thread()
{
    return m_thread;
}

void Worker::set_thread(std::thread&& thread)
{
    m_thread = std::move(thread);
}

void Worker::execute_job(Job* job)
{
    job->padding[0] = static_cast<char>('0' + m_worker_idx);
    job->pfn(job->padding);
    finish(job);
    m_jobs_completed++;
}

Job* Worker::get_job()
{
    WorkStealingQueue<>* queue = get_queue();
    Job* job = queue->pop();
    if(is_empty_job(job))
    {
        unsigned int rnd = (m_steal_from_queue++) % m_num_workers;
        if(rnd == m_worker_idx)
        {
            std::this_thread::yield();
            return nullptr;
        }
        WorkStealingQueue<>* steal_queue = m_queues[rnd];
        Job* stolen_job = steal_queue->steal();
        if(is_empty_job(stolen_job))
        {
            //We couldn't steal a job from the other queue either
            std::this_thread::yield();
            return nullptr;
        }
        return stolen_job;
    }
    return job;
}

void Worker::finish(Job* job)
{
    const uint32_t unfinished_jobs = --job->unfinished_jobs;
    if(unfinished_jobs == 0 && job->parent)
    {
        finish(job->parent);
    }
}

