#ifndef JOB_SYSTEM_H
#define JOB_SYSTEM_H

#include <vector>
#include <atomic>
#include <memory>
#include <thread>

using JobFunction = std::add_pointer<void(const void*)>::type;
struct Job
{
    JobFunction pfn;
    Job* parent;
    std::atomic<uint32_t> unfinished_jobs;
    char padding[4];
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

            if(t != b)
            {
                //There is still more than one item left in the queue
                return job;
            }


            //This is the last item in the queue
            if(__sync_val_compare_and_swap(&m_top, t, t + 1) != t)
            {
                //failed race against steal operation
                job = nullptr;
            }

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
            if(__sync_val_compare_and_swap(&m_top, t, t + 1) != t)
            {
                return nullptr;
            }

            //The exchg was successful return the new job
            return job;
        }
        return nullptr;
    }

    bool is_empty()
    {
        return m_bottom <= m_top;
    }

    std::size_t size()
    {
        return m_bottom - m_top;
    }

private:
    Job* m_jobs[Size];
    uint32_t m_bottom = 0;
    uint32_t m_top = 0;
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

class Worker;
class JobSystem
{
public:
    /**
     * @brief
     * JobSystem Create a new job system, this should preferably be a singleton
     * as the job system creates one thread per "hardware" thread
     */
    JobSystem(std::size_t num_workers = std::thread::hardware_concurrency());

    ~JobSystem();

    /**
     * @brief create_job
     * @param function
     * @return
     */
    Job* create_job(JobFunction function);

    /**
     * @brief create_job_as_child
     * @param parent
     * @param function
     * @return
     */
    Job* create_job_as_child(Job* parent, JobFunction function);

    /**
     * @brief run Enqueue the given job
     * @param job
     */
    void enqueue(Job* job);

    /**
     * @brief has_job_completed Check whether the given job and all it's children has finished execution
     * @param job
     * @return
     */
    bool has_job_completed(const Job* job) const;

    /**
     * @brief wait Wait for the given job and all it's children to finish execution
     * @param job
     */
    void wait(const Job* job);

    /**
     * @brief get_queue
     * @return
     */
    WorkStealingQueue<>* get_queue(std::size_t idx);

    template<typename T>
    void parlell_for(std::vector<T> set, JobFunction jf)
    {
    }

private:
    std::vector<std::unique_ptr<Worker>> m_workers;
    JobAllocator m_job_allocator;
};

#endif // JOB_SYSTEM_H
