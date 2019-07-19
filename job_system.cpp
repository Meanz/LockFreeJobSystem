#include "job_system.h"
#include "job_worker.h"

#include <cassert>

JobSystem::JobSystem(std::size_t num_workers)
{
    //Initialize workers
    assert(num_workers != 0);

    m_workers.resize(num_workers);
    m_queues.resize(num_workers);

    //Create queues
    for(std::size_t i=0; i < num_workers; i++)
    {
        m_queues[i] = new WorkStealingQueue<>();
    }

    //Create workers
    m_workers[0] = std::make_unique<Worker>(0, num_workers, *this, m_queues.data());
    for(std::size_t i=1; i < num_workers; i++)
    {
        m_workers[i] = std::make_unique<Worker>(i, num_workers, *this, m_queues.data());
        m_workers[i]->set_active(true);
    }
    //Start workers
    for(std::size_t i=1; i < num_workers; i++)
    {
        m_workers[i]->set_thread(std::thread(&Worker::thread_function, m_workers[i].get()));
    }
}

JobSystem::~JobSystem()
{
    for(std::size_t i=1; i < m_workers.size(); i++)
    {
        m_workers[i]->set_active(false);
        m_workers[i]->get_thread().join();
    }
    m_workers.clear();
    //Destroy all queues
    for(std::size_t i=0; i < m_queues.size(); i++)
    {
        delete m_queues[i];
    }
    m_queues.clear();
}

Job* JobSystem::create_job(JobFunction function)
{
    Job* job = m_job_allocator.allocate();
    job->pfn = function;
    job->parent = nullptr;
    job->unfinished_jobs = 1;
    return job;
}

Job* JobSystem::create_job_as_child(Job* parent, JobFunction function)
{
    //Atomic increment
    parent->unfinished_jobs++;

    Job* job = m_job_allocator.allocate();
    job->pfn = function;
    job->parent = parent;
    job->unfinished_jobs = 1;
    return job;
}

void JobSystem::enqueue(Job* job)
{
    m_workers[0]->run(job);
}

bool JobSystem::has_job_completed(const Job* job) const
{
    if(job->unfinished_jobs.load() == 0)
    {
        return true;
    }
    return false;
}

void JobSystem::wait(const Job* job)
{
    while(!has_job_completed(job))
    {
        m_workers[0]->fetch_and_execute();
    }
}
