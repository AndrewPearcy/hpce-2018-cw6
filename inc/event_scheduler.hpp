#ifndef event_scheduler_hpp
#define event_scheduler_hpp

#include "object_event.hpp"

#include <cassert>
#include <random>
#include <cmath>

struct event_schedule
{
    int time_base=0;
    int time_till_rise=0;
    int rise_time=0;
    int time_till_fall=0;
    int fall_time=0;
    float strength=0;

    int scale;

    int begin_pos_x;
    int begin_pos_y;
    int end_pos_x;
    int end_pos_y;

    double prop(int t0, int dt, int t) const
    {
        assert(t0 <= t && t<=t0+dt);
        return (t-t0)/(double)dt;
    }

    double lerp(double a, double b, double p) const
    { return (1-p)*a+p*b; }

    int total_time() const
    {
        return time_till_rise+rise_time+time_till_fall+fall_time;
    }

    int end_time() const
    {
        return time_base+total_time();
    }

    bool step(int t, object_event &e)
    {
        // Create default event doing nothing
        e.t=t;
        e.x=0;
        e.y=0;
        e.scale=1;
        e.strength=0;
        if(t >= end_time()){
            return false;
        }

        e.scale=scale;
        if(t < time_base+time_till_rise){
            // Nothing happened yet.
            //fprintf(stderr, "t0=%d, t=%d, Not yet\n", time_base, t);
        }else{
            double dt=(t-time_base)/(double)total_time();
            e.x=round(lerp(begin_pos_x, end_pos_x, dt)/scale)*scale;
            e.y=round(lerp(begin_pos_y, end_pos_y, dt)/scale)*scale;
            
            e.strength=0;
            if(t < time_base+time_till_rise){
                // do nothing
            }else if(t < time_base+time_till_rise+rise_time){
                e.strength=lerp(0, strength, prop(time_base+time_till_rise, rise_time, t));
            }else if(t < time_base+time_till_rise+rise_time+time_till_fall){
                e.strength=strength;
            }else{
                e.strength=lerp(strength, 0, prop(time_base+time_till_rise+rise_time+time_till_fall, fall_time, t));
            }
            assert(!std::isnan(e.strength));
            assert(0<=e.strength && e.strength<=1);
        }

        return true;
    }
};

event_schedule make_random_event_schedule(std::mt19937 &urng, int w, int h, int t0, int trange, int min_scale, int max_scale)
{
    std::uniform_real_distribution<> udist;

    assert(min_scale<=max_scale);

    event_schedule s;
    s.time_base=t0;
    while(s.total_time() == 0){
        s.time_till_rise=urng()%trange;
        s.rise_time=urng()%trange;
        s.time_till_fall=urng()%trange;
        s.fall_time=urng()%trange;
    }

    s.strength=0.75+udist(urng)*0.25;

    //s.scale=std::min((double)max_scale, min_scale+std::exp2(std::floor(-std::log2(udist(urng)))));
    // Current gen hardware only supports one scale. Too much overhead for multi-resolution
    s.scale=1;

    s.begin_pos_x=urng()%w;
    s.begin_pos_y=urng()%h;

    float max_speed=0.25*s.scale; // Can't move by more than 0.25*scale in one frame in each dimension
    do{
        s.end_pos_x=s.begin_pos_x+max_speed*(udist(urng)*2-1)*(s.rise_time+s.time_till_fall+s.fall_time);
    }while(s.end_pos_x < 0 || w<=s.end_pos_x );
    do{
        s.end_pos_y=s.begin_pos_y+max_speed*(udist(urng)*2-1)*(s.rise_time+s.time_till_fall+s.fall_time);
    }while(s.end_pos_y < 0 || h <= s.end_pos_y );

    //fprintf(stderr, "ttRise=%d, tRise=%d, ttFall=%d, tFall=%d, strength=%f\n", s.time_till_rise, s.rise_time, s.time_till_fall, s.fall_time, s.strength);

    return s;
}

class object_event_stream
{
    int m_w;
    int m_h;
    int m_trange;
    int m_min_scale;
    int m_max_scale;

    std::mt19937 m_urng;
    event_schedule m_curr;

    int m_t0;
public:

    object_event_stream()
    {}

    object_event_stream(int w, int h, int trange, int seed)
        : m_w(w)
        , m_h(h)
        , m_trange(trange)
        , m_min_scale(1)
        , m_max_scale(std::min(w,h)/16)
        , m_urng(seed)
        , m_t0(0)
    {}

    bool read(object_event &ev)
    {
        if( m_curr.end_time() <= m_t0 ){
            m_curr=make_random_event_schedule(m_urng, m_w, m_h, m_t0, m_trange, m_min_scale, m_max_scale);
        }

        //fprintf(stderr, "m_t0=%d\n", m_t0);
        m_curr.step(m_t0, ev);
        ++m_t0;

        return true;
    }
};


#endif
