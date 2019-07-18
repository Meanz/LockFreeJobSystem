#include "simple_physics_demo.h"

#include "test/timing.h"

#include <QPaintEvent>
#include <QPainter>

SimplePhysicsDemo::SimplePhysicsDemo(std::size_t ball_count)
{
    m_ball_p.resize(ball_count);
    m_ball_v.resize(ball_count);
    m_ball_s.resize(ball_count);
    m_ball_c.resize(ball_count);
    init_balls();
    this->startTimer(16, Qt::PreciseTimer);
}

void SimplePhysicsDemo::init_balls()
{
    for(std::size_t i=0; i < m_ball_p.size(); i++)
    {
        int s = 5 + std::rand() % 20;
        m_ball_s[i] = s;
        m_ball_p[i] = v2(std::rand() % (this->width() - s), std::rand() % (this->height() - s));
        m_ball_v[i] = v2(1 + std::rand() % 5, 1 + std::rand() % 5);
        m_ball_c[i] = Qt::green;
    }
}


bool ball_vs_ball(v2 b1, int s1,
                  v2 b2, int s2)
{
    float d = (b2 - b1).length();
    return d < s1 + s2;
}

void SimplePhysicsDemo::check_collisions(std::size_t off, std::size_t count)
{
    //Create collision matrix

    for(std::size_t i=off; i < off+count; i++)
    {
        v2 bp1 = m_ball_p[i];
        v2& bv1 = m_ball_v[i];
        int bs1 = m_ball_s[i];

        for(std::size_t j=0; j < m_ball_p.size(); j++)
        {
            if(i == j) continue;
            v2 bp2 = m_ball_p[j];
            v2& bv2 = m_ball_p[j];
            int bs2 = m_ball_s[j];

            if(ball_vs_ball(bp1, bs1, bp2, bs2))
            {
                //Collsion detection!!!
                v2 bdp = bp2 - bp1;
                bv1 = -bv1;
                bv2 = -bv2;
                m_ball_c[i] = Qt::red;
                m_ball_c[j] = Qt::red;
            }
        }
    }
}

void SimplePhysicsDemo::update_position(std::size_t off, std::size_t count)
{
    for(std::size_t i=off; i < off+count; i++)
    {
        //Check for collision against wall
        v2& p = m_ball_p[i];
        v2& v = m_ball_v[i];
        int s = m_ball_s[i];
        v2 new_p = p + v;
        //Left wall
        v2 n;
        bool reflect = false;
        if(new_p.x <= 0.0f)
        {
            //Reflect right
            n = v2(-1.0f, 1.0f);
            reflect = true;
        }
        else if(new_p.x >= this->width() - s)
        {
            //Reflect left
            n = v2(-1.0f, 1.0f);
            reflect = true;
        }
        else if(new_p.y <= 0.0f)
        {
            //Reflect down
            n = v2(1.0f, -1.0f);
            reflect = true;
        }
        else if(new_p.y >= this->height() - s)
        {
            //Reflect up
            n = v2(1.0f, -1.0f);
            reflect = true;
        }
        if(reflect)
        {
            v.x *= n.x;
            v.y *= n.y;
        }
        p = new_p;
    }
}

void SimplePhysicsDemo::timerEvent(QTimerEvent* qte)
{
    /*Job* root =
    for(std::size_t i=0; i < m_ball_p.size() / 256; i++)
    {

    }*/
    Stopwatch sw;
    sw.Start();
    check_collisions(0, m_ball_p.size());
    update_position(0, m_ball_p.size());
    sw.Stop();

    m_simulation_time = sw.ElapsedMilliseconds();
    this->update();
}

void SimplePhysicsDemo::paintEvent(QPaintEvent* qpe)
{
    QPainter painter;
    painter.begin(this);

    painter.fillRect(this->rect(), Qt::black);

    for(std::size_t i=0; i < m_ball_p.size(); i++)
    {
        v2 p = m_ball_p[i];
        int s = m_ball_s[i];
        painter.setPen(m_ball_c[i]);
        painter.drawEllipse(int(p.x), int(p.y), s, s);
    }

    painter.setPen(Qt::white);
    painter.drawText(15, 15, QString("Simulation time: %1ms").arg(m_simulation_time));

    painter.end();
}
