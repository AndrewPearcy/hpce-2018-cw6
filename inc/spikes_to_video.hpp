
#ifndef spikes_to_video_hpp
#define spikes_to_video_hpp

#include <vector>
#include <algorithm>

#include "spikes.hpp"
#include "frame.hpp"


template<class TSource>
class SpikesToVideo
{
private:
    TSource m_source;

    int m_tState = 0;

    Frame m_state;

    spike_info m_spike;
    bool m_spikeValid=false;
    int m_spikeIndex=0;

    bool m_finished=false;
public:
    SpikesToVideo(TSource &&src)
        : m_source(std::move(src))
    {}

    bool read(Frame &out){
        if(m_finished){
            return false;
        }

        /* We need to keep reading spikes until either:
            - We get a spike for a different time-step; or
            - We reach end of stream
        */

       while(1){
            //fprintf(stderr, "tState=%d, tSpike=%d (valid=%d, index=%d, x=%u, y=%u)\n", m_tState, m_spike.t, m_spikeValid, m_spikeIndex, m_spike.x, m_spike.y);

            if(m_spikeValid && m_tState < (int)m_spike.t){
                //fprintf(stderr, "  Later spike\n");
                // The pending spike is after the current frame, so output it
                out = m_state;
                m_tState++;
                return true;
            }

            if(!m_spikeValid){
                //fprintf(stderr, "  Reading spike\n");
                // Try to read the next spike
                if(!m_source.read(m_spike)){
                    m_finished=true;
                    return false;
                }
                m_spikeValid=true;
                ++m_spikeIndex;
            }

            assert(m_spikeValid);
            if(m_spike.t == m_tState){
                //fprintf(stderr, "  Consume spike\n");
                if(m_state.w == 0){
                    m_state.fill_grey(m_spike.w, m_spike.h);
                }
                m_state.set_rgb( m_spike.x, m_spike.y,  m_spike.r,m_spike.g,m_spike.b );
                m_spikeValid=false;
            }else{
                assert(m_tState < m_spike.t);
                //fprintf(stderr, "  Spike too new\n");
                // We need to output the current state first before consuming this spike
            }
       }

        return true;
    }    
};


#endif