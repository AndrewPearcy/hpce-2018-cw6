#include "spikes_to_video.hpp"

int main()
{
    BinarySpikeReader spikeSource(stdin);
    SpikesToVideo<BinarySpikeReader> frameSource(std::move(spikeSource));
    
    FrameWriter sink(stdout);

    int n=0, nNext=1;
    Frame f;
    while(frameSource.read(f)){
        if(n==nNext){
            fprintf(stderr, "Spike %u.\n", n);
            nNext=std::min(2*nNext, nNext+1000);
        }
        sink.write(f);
        ++n;
    }

    return 0;
}
