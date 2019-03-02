#include "frame.hpp"
#include "classifier.hpp"

int main()
{
    fprintf(stderr, "Classifying frames from stdin, writing to stdout.\n");
    FrameReader source(stdin);

    pattern p=pattern::test_pattern();

    float threshold=0.4;

    int n=0, nNext=1;
    Frame f(0,0);
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }

        std::vector<object_event> events;
        pattern_search(f, p, n, 1, threshold, events);

        for(const auto &e : events){
            e.write(stdout);
        }
        fflush(stdout);
        ++n;
    }

    return 0;
}
