#ifndef SIMPLE_PHYSICS_DEMO_H
#define SIMPLE_PHYSICS_DEMO_H

#include <QWidget>

#include "mjob.hpp"
#include "test/vector.h"

class SimplePhysicsDemo : public QWidget
{
    Q_OBJECT
public:
    SimplePhysicsDemo(std::size_t ball_count);

    void init_balls();
    void check_collisions(std::size_t off, std::size_t count);
    void update_position(std::size_t off, std::size_t count);

    void timerEvent(QTimerEvent* qte);
    void paintEvent(QPaintEvent* qpe);

private:
    bool m_running = true;
    JobSystem m_job_system;
    std::vector<v2> m_ball_p;
    std::vector<v2> m_ball_v;
    std::vector<int> m_ball_s;
    std::vector<QColor> m_ball_c;
    double m_simulation_time;
};


#endif // SIMPLE_PHYSICS_DEMO_H
