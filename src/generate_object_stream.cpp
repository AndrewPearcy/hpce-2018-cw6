#include "frame.hpp"
#include "pattern.hpp"
#include "event_scheduler.hpp"

#include <random>
#include <cmath>

int main()
{
    FrameReader source(stdin);

    std::mt19937 rng;
    std::uniform_real_distribution<> udist;

    std::vector<object_event_stream> streams;

    int n=0, nNext=1;
    Frame f(0,0);
    bool first=true;
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }

        if(first){
            first=false;
            for(unsigned i=0; i<8; i++){
                streams.push_back(object_event_stream(f.w, f.h, /*event time scale*/ 10, i));
            }
        }

        std::vector<object_event> evs(8);
        
        for(unsigned i=0; i<8; i++){
            streams[i].read(evs[i]);
            //evs[i].strength=1;
            //fprintf(stderr, "t=%d\n", evs[i].t);
            evs[i].write(stdout);
        }
        
        ++n;
    }

    return 0;
}
