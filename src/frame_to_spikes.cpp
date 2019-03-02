#include "video_to_spikes.hpp"

int main()
{
    // Changes smaller than 16 are supressed
    double spikeThreshold=16.0;

    // Current camera produces at most 10 MSpikes/sec, so at 3000 frames/sec
    // we get 3333 spikes/frame
    int maxSpikesPerFrame=3333;

    FrameReader frameSource(stdin);
    VideoToSpikes<FrameReader> spikeSource(std::move(frameSource), spikeThreshold, maxSpikesPerFrame);
    
    BinarySpikeWriter sink(stdout);

    int n=0, nNext=1;
    std::vector<spike_info> spikes;
    while(spikeSource.read(spikes)){
        for(const auto &s : spikes){
            if(n==nNext){
                fprintf(stderr, "Spike %u, t=%u.\n", n, s.t);
                nNext=std::min(2*nNext, nNext+1000000);

            }
            ++n;
            sink.write(s);
        }
        spikes.clear();
        
    }

    return 0;
}
