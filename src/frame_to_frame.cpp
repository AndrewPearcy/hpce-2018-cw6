#include "frame.hpp"

int main()
{
    fprintf(stderr, "Copying frames from stdin to stdout.\n");
    FrameReader source(stdin);
    FrameWriter sink(stdout);

    int n=0, nNext=1;
    Frame f(0,0);
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }
        sink.write(f);
        ++n;
    }

    return 0;
}
