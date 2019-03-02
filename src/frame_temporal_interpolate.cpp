#include "temporal_interpolate.hpp"

int main(int argc, char *argv[])
{
    if(argc!=2){
        fprintf(stderr, "usage : frame_temporal_interpolate oversample\n");
        fprintf(stderr, "   reads mjpeg from stdin, writes mjpeg to stdout\n");
        fprintf(stderr, "\n");
        exit(1);
    }

    int oversample=atoi(argv[1]);
    fprintf(stderr, "Oversampling by %d times\n", oversample);

    FrameReader source(stdin);
    TemporalInterpolator<FrameReader> interpolator(std::move(source), oversample); // After this point interpolator owns source

    FrameWriter sink(stdout);

    int n=0, nNext=1;
    Frame f(0,0);
    while(interpolator.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }
        sink.write(f);
        ++n;
    }

    return 0;
}
