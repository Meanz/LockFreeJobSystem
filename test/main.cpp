#include <QCoreApplication>
#include <QDebug>

#include <unistd.h> //For usleep

#include "timing.h"
#include "mjob.hpp"
#include "vector.h"
#include <functional>

int fib(int n)
{
  int a = 0, b = 1, c, i;
  if( n == 0)
    return a;
  for (i = 2; i <= n; i++)
  {
     c = a + b;
     a = b;
     b = c;
  }
  return b;
}

void empty_job(const void* p)
{
  //  fib(100000);
}

/* rdtsc */
extern __inline unsigned long long
    __attribute__((__gnu_inline__, __always_inline__, __artificial__))
    rdtsc (void)
{
    return __builtin_ia32_rdtsc ();
}

void fib_test()
{
    JobSystem job_system;

    usleep(20000); //Sleep 10 ms for thread to come up and running

    //Benchmark one single run
    const int num_jobs = 4096;
    const int loops = 1;

    Stopwatch stopwatch;
    stopwatch.Start();
    for(std::size_t n=0; n < num_jobs; n++)
    {
        empty_job(nullptr);
    }
    stopwatch.Stop();
    double single_thread_1_run = (stopwatch.ElapsedNanoseconds() / double(num_jobs)) / 1e6;
    qDebug() << "Actual job takes " << single_thread_1_run << " milliseconds ";

    //Benchmark job system runs
    for(std::size_t k=0; k < 1; k++)
    {

    stopwatch.Start();
    std::size_t r1 = rdtsc();
    for(std::size_t n=0; n < loops; n++)
    {
        Job* root = job_system.create_job(empty_job);
        for(std::size_t i=0; i < num_jobs - 1; i++)
        {
            Job* child = job_system.create_job_as_child(root, empty_job);
            job_system.enqueue(child);
        }
        job_system.enqueue(root);
        job_system.wait(root);
    }
    std::size_t r2 = rdtsc();
    stopwatch.Stop();
    //4096 jobs over this span
    double ms_per_job = stopwatch.ElapsedMilliseconds() / loops / num_jobs;
    std::size_t cycles_per_job = (r2 - r1) / loops / num_jobs;

    qInfo() << "Time: " << stopwatch.ElapsedMilliseconds() << "ms | per run:" <<
        ms_per_job << "ms | calculated overhead: " << (ms_per_job - single_thread_1_run);
    qInfo() << "Total Cycles: " << (r2 - r1) << " | per job: " << cycles_per_job;


    }
}

#include <QApplication>
#include "simple_physics_demo.h"

int simple_physics_demo(int argc, char* argv[])
{
    QApplication app(argc, argv);

    SimplePhysicsDemo* spd = new SimplePhysicsDemo(50);
    spd->showNormal();

    return app.exec();
}

int main(int argc, char *argv[])
{
    //SimplePhysicsDemo spd;
    //spd.run();

    //simple_physics_demo(argc, argv);

    fib_test();
    //std::function<void()> fn = []() {};
    //qDebug() << sizeof(fn);


    return 0;
}
