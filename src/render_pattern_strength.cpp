#include "frame.hpp"
#include "classifier.hpp"

int main()
{
    FrameReader source(stdin);
    FrameWriter sink(stdout);

    pattern p=pattern::test_pattern();

    int n=0, nNext=1;
    Frame f(0,0);
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }

        Frame r;
        pattern_strength_render(f, r, p);

        sink.write(r);
        ++n;
    }

    return 0;
}
