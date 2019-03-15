
#ifndef video_to_spikes_hpp
#define video_to_spikes_hpp

#include <vector>
#include <algorithm>
#include <math.h>

#include "spikes.hpp"
#include "frame.hpp"

/* We want to pretend that a normal sequence of video-frames at
    t=0,f_dt,2*f_dt,3*f_dt,... is actually a stream of spikes
    which occur at t=0,s_dt,2*s_dt,3*s_dt where s_dt is smaller
    than f_dt. In practise s_dt is not so obviously quantised,
    but assuming something like f_dt=1/30 and s_dt=1/3000 gets
    us somewhere close.

    Within each spike time-step we are limited to at most
    s_k spikes, so we always send spikes for the pixels with
    the largest change since the last update. This ensures that
    the biggest changes are propagated fastest, as big changes
    are (presumably) going to result in the biggest changes to
    classification.
    
    To split things up between frames we'll use simple linear
    interpolation on a per-pixel basis, so between time t and
    t+f_dt we'll assume that pixel (i,j) varies linearly from
    p[i,j,dt] to p[i,j,dt+f_dt]. Within each time step we send
    up to s_k spikes, and any remaining changes happen in a
    following time step.

    This approach has some interesting behaviour:
    - Extremely large changes (e.g black to white flips) take
      a long time to propagate through, as it takes at
      least (w*h)/s_k spikes fall all pixels to flip.
    - A pixel might manage to exceed the threshold in one frame, 
      then return to the original value. If there were other
      larger changes, then no spike will be produced, even though
      there was a large enough change.
    - A pixel might spike twice within one t_s period, particularly
      if it is rapidly changing.
*/

template<class TSource>
class VideoToSpikes
{
private:
    TSource m_source;

    double m_spike_threshold;
    int m_max_spikes_per_frame;

    uint32_t m_t=0;
    Frame m_state;
public:
    VideoToSpikes(TSource &&src, double spike_threshold, int max_spikes_per_frame)
        : m_source(std::move(src))
        , m_spike_threshold(spike_threshold)
        , m_max_spikes_per_frame(max_spikes_per_frame)
        , m_t(0)
    {}

    bool read(std::vector<spike_info> &spikes){
        Frame input;

        spikes.clear();

        fprintf(stderr, "Reading frame\n");

        if(!m_source.read(input)){
            fprintf(stderr, "Stream finished\n");
            return false;
        }

        if(m_state.w==0){
            fprintf(stderr, "Got first frame, w=%u, h=%u \n", input.w, input.h);
            m_state.fill_grey(input.w, input.h);
        }

        // Produce candidate spikes for each pixel based on change against state
        std::vector<std::pair<double,spike_info> > candidates;
        for(unsigned y=0; y<m_state.h; y++){
            for(unsigned x=0; x<m_state.w; x++){
                double lum_curr=m_state.get_luminance(x,y);
                double lum_new=input.get_luminance(x,y);

                double change=std::abs(lum_curr-lum_new);
                //fprintf(stderr, " %u,%u : %g\n", x,y, change);
                if(change > m_spike_threshold){
                    spike_info spike;
                    spike.w=m_state.w;
                    spike.h=m_state.h;
                    spike.t=m_t;
                    spike.x=x;
                    spike.y=y;
                    spike.r=input.get_r(x,y);
                    spike.g=input.get_g(x,y);
                    spike.b=input.get_b(x,y);
                    candidates.push_back(std::make_pair(change,spike));
                }
            }
        }

        //fprintf(stderr, "|candidates| = %u\n", candidates.size());

        // Sort the spikes by the change amount so that we can manage budget per time step
        std::sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b){ return a.first < b.first;  }); 

        // Trim the array down if it exceeds maximum spike budget
        if(candidates.size() > m_max_spikes_per_frame){
            int toDelete=candidates.size()-m_max_spikes_per_frame;
            candidates.erase(candidates.begin(), candidates.begin()+toDelete);
        }

        //fprintf(stderr, "  trimmed = %u\n", candidates.size());

        // Now apply it back to the state to reflect the spikes output
        for(const auto &vs : candidates){
            const auto &s=vs.second;
            m_state.set_rgb( s.x, s.y,  s.r,s.g,s.b);

            spikes.push_back(s);
        }

        fprintf(stderr, "m_t=%u\n", m_t);
        ++m_t;

        return true;
    }    
};


#endif
