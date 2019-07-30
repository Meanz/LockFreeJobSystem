#ifndef MJOB_HPP
#define MJOB_HPP

//For std::atomic
#include <atomic>
//For std::vector
#include <vector>
//For std::unique_ptr
#include <memory>
//For std::thread and std::thread::hardware_concurrency
#include <thread>
//For assert
#include <assert.h>

//See https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/

using JobFunction = std::add_pointer<void(const void*)>::type;
struct Job
{
    JobFunction pfn;
    Job* parent;
    std::atomic<uint32_t> unfinished_jobs;
    char padding[48];
    std::atomic<uint32_t> continuation_count;
    Job* continuations[15];
};

template<std::size_t Size = 4096u,
          std::size_t Mask = Size - 1u>
class WorkStealingQueue
{
public:
    void push(Job* job)
    {
        //Push only changes bottom
        long b = m_bottom;
        m_jobs[b& Mask] = job;
        //Ensure the job is written before b+1 is published on x86/64, a compiler barrier is enough
        asm volatile("": : :"memory");
        m_bottom = b + 1;
    }

    Job* pop()
    {
        //Pop only changes bottom
        long b = m_bottom - 1;
        //Hardware barrier to  make sure bottom is written before top is read
        //The x86 memory model explicitly allows that
        //"Loads may be reordered with older stores to different locations"
        //Which is exactly the case here
        //__sync_synchronize();
        //It turns out that Exchange is faster than the HW barrier
        //However exchange essentially works as a HW barrier too
        __sync_lock_test_and_set(&m_bottom, b);
        long t = m_top;
        if(t <= b)
        {
            Job* job = m_jobs[b & Mask];
            //There is still more than one item left in the queue
            if(t != b) return job;
            //This is the last item in the queue
            //failed race against steal operation
            if(__sync_val_compare_and_swap(&m_top, t, t + 1) != t) job = nullptr;
            m_bottom = t + 1;
            return job;
        }
        else
        {
            m_bottom = t;
            return nullptr;
        }
    }

    Job* steal()
    {
        //Steal only changes top
        long t = m_top;
        //Ensure that top is always read before bottom
        //loads will not be reordered with other loads on x86
        asm volatile("": : :"memory");
        long b = m_bottom;
        if(t < b)
        {
            Job* job = m_jobs[t & Mask];
            //Check if any modifications was done to T
            //If there was a concurrent modification, fail the steal
            if(__sync_val_compare_and_swap(&m_top, t, t + 1) != t) return nullptr;
            //The exchg was successful return the new job
            return job;
        }
        return nullptr;
    }

    bool is_empty() const { return m_bottom <= m_top; }
    std::size_t size() const { return m_bottom - m_top; }

private:
    Job* m_jobs[Size];
    uint32_t m_bottom = 0;
    uint32_t m_top = 0;
};


class JobWorker
{
public:
    JobWorker(uint8_t worker_idx, uint8_t num_workers, WorkStealingQueue<>** queues) :
        m_worker_idx(worker_idx),
        m_num_workers(num_workers),
        m_queues(queues)
    {}

    ~JobWorker() {}

    void set_active(bool active) { m_active = active; }
    bool is_empty_job(Job* job) { return job == nullptr; }

    //NOTE: Not threadsafe, must be called from worker threads thread
    void run(Job* job) { get_queue()->push(job); }

    /**
     * @brief get_queue
     * @return
     */
    WorkStealingQueue<>* get_queue() { return m_queues[m_worker_idx]; }

    /**
     * @brief get_thread Get the thread associated with this worker
     * @return
     */
    std::thread& get_thread() { return m_thread; }

    /**
     * @brief set_thread Set the thread assocciated with this worker
     * @param thread
     */
    void set_thread(std::thread&& thread) { m_thread = std::move(thread); }

    /**
     * @brief thread_function Main loop, fetch and execute will yield if there is no job available
     */
    void thread_function() { while(m_active) fetch_and_execute(); }


    /**
     * @brief fetch_and_execute Attempt to fetch and execute a job
     */
    void fetch_and_execute() { if(Job* job = get_job()) execute_job(job); }
private:

    /**
     * @brief execute_job
     * @param job
     */
    void execute_job(Job* job)
    {
        job->padding[0] = static_cast<char>('0' + m_worker_idx);
        job->pfn(job->padding);
        finish(job);
        //For statistics
        m_jobs_completed++;
    }

    /**
     * @brief get_job
     * @return
     */
    Job* get_job()
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

    /**
     * @brief finish
     * @param job
     */
    void finish(Job* job)
    {
        const uint32_t unfinished_jobs = --job->unfinished_jobs;
        if(unfinished_jobs == 0 && job->parent)
        {
            finish(job->parent);
        }
    }

    bool m_active = false;
    uint8_t m_worker_idx;
    uint8_t m_num_workers;
    WorkStealingQueue<>** m_queues;

    std::thread m_thread;
    uint32_t m_jobs_completed = 0;
    uint32_t m_steal_from_queue = 0;
};

class JobAllocator
{
public:
    Job* allocate()
    {
        uint32_t index = m_allocated_jobs++;
        return &m_jobs[(index - 1u) & (4096 - 1u)];
    }
private:
    std::atomic<uint32_t> m_allocated_jobs;
    Job m_jobs[4096];
};

class JobSystem
{
public:

    /**
     * @brief
     * JobSystem Create a new job system, this should preferably be a singleton
     * as the job system creates one thread per "hardware" thread
     */
    JobSystem(std::size_t num_workers = std::thread::hardware_concurrency())
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
        m_workers[0] = std::make_unique<JobWorker>(0, num_workers, m_queues.data());
        for(std::size_t i=1; i < num_workers; i++)
        {
            m_workers[i] = std::make_unique<JobWorker>(i, num_workers, m_queues.data());
            m_workers[i]->set_active(true);
        }
        //Start workers
        for(std::size_t i=1; i < num_workers; i++)
        {
            m_workers[i]->set_thread(std::thread(&JobWorker::thread_function, m_workers[i].get()));
        }
    }

    ~JobSystem()
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

    /**
     * @brief create_job
     * @param function
     * @return
     */
    Job* create_job(JobFunction function)
    {
        Job* job = m_job_allocator.allocate();
        job->pfn = function;
        job->parent = nullptr;
        job->unfinished_jobs = 1;
        return job;
    }

    /**
     * @brief create_job_as_child
     * @param parent
     * @param function
     * @return
     */
    Job* create_job_as_child(Job* parent, JobFunction function)
    {
        //Atomic increment
        parent->unfinished_jobs++;

        Job* job = m_job_allocator.allocate();
        job->pfn = function;
        job->parent = parent;
        job->unfinished_jobs = 1;
        return job;
    }

    /**
     * @brief enqueue Enqueue the given job
     * @param job
     */
    void enqueue(Job* job)
    {
        m_workers[0]->run(job);
    }

    /**
     * @brief has_job_completed Check whether the given job and all it's children has finished execution
     * @param job
     * @return
     */
    bool has_job_completed(const Job* job) const { return job->unfinished_jobs == 0; }

    /**
     * @brief wait Wait for the given job and all it's children to finish execution
     * @param job
     */
    void wait(const Job* job)
    {
        while(!has_job_completed(job))
        {
            m_workers[0]->fetch_and_execute();
        }
    }

private:
    std::vector<std::unique_ptr<JobWorker>> m_workers;
    std::vector<WorkStealingQueue<>*> m_queues;
    JobAllocator m_job_allocator;
};


#endif // MJOB_HPP
