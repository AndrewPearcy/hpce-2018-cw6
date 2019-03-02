#ifndef temporal_interpolate_hpp
#define temporal_interpolate_hpp

#include <vector>
#include <cstdint>
#include <cassert>
#include <stdexcept>

#include "frame.hpp"

template<class TSrc>
struct TemporalInterpolator
{
private:
    // Where the frames are coming from
    TSrc m_src; // We own whatever it is

    int m_oversample;

    Frame m_f0;
    Frame m_f1;
    Frame m_f2;
    Frame m_f3;
    int m_off;
public:
    TemporalInterpolator(TSrc &&src, int oversample)
        : m_src(std::move(src))
        , m_oversample(oversample)
    {
        assert(m_oversample >= 1);
        if(!m_src.read(m_f0)){
            throw std::runtime_error("Can't temporally interpolate file with 0 frames.");
        }
        if(!m_src.read(m_f1)){
            throw std::runtime_error("Can't temporally interpolate file with 1 frame.");
        }
        if(!m_src.read(m_f2)){
            throw std::runtime_error("Can't temporally interpolate file with 2 frames.");
        }
        if(!m_src.read(m_f3)){
            throw std::runtime_error("Can't temporally interpolate file with 3 frames.");
        }
        m_off=0;
    }

    static double cubic(double v0, double v1, double v2, double v3, double alpha)
    {
        double alpha2=alpha*alpha;
        double alpha3=alpha*alpha*alpha;
        double r=( v0*(-alpha3+2*alpha2-alpha) + v1*(3*alpha3-5*alpha2+2) + v2*(-3*alpha3+4*alpha2+alpha) + v3*(alpha3-alpha2)) * 0.5;
        r=std::max(0.0, std::min(255.0, r));
        return r;
    }

    bool read(Frame &mid)
    {
        if(m_off==m_oversample){
            // Advance forwards
            m_f0=m_f1;
            m_f1=m_f2;
            m_f2=m_f3;
            if(!m_src.read(m_f3)){
                return false; // End of stream
            }
            m_off=0;
        }

        // We have two frames, so now interpolate between them
        double alpha=m_off/(double)m_oversample;

        // Our per-pixel interpolation, which captures alpha
        auto f = [=](uint8_t *dst, const uint8_t *f0, const uint8_t *f1, const uint8_t *f2, const uint8_t *f3){
            for(int i=0; i<3; i++){
                dst[i]=cubic(f0[i], f1[i], f2[i], f3[i], alpha);
            }
        };

        // interpolate the entire frame
        mid=m_f0.quad_op(m_f1, m_f2, m_f3, f);

        ++m_off;

        return true;
    }

};


#endif
