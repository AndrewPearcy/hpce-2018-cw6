#include "frame.hpp"
#include "pattern.hpp"
#include "merger.hpp"
#include "event_scheduler.hpp"

#include <random>
#include <cmath>

int main(int argc, char *argv[])
{
    fprintf(stderr, "Inserting objects into frame.\n");
    FrameReader source(stdin);
    FrameWriter sink(stdout);

    if(argc<2){
        fprintf(stderr, "Need object event stream as second argument.");
        exit(1);
    }

    FILE *eventsIn=fopen(argv[1], "rt");
    if(!eventsIn){
        fprintf(stderr, "Couldn't open events file '%s'\n", argv[1]);
        exit(1);
    }
    ObjectEventReader eventReader(eventsIn);

    std::mt19937 rng;
    std::uniform_real_distribution<> udist;

    pattern p=pattern::test_pattern();

    std::vector<std::vector<object_event> > events;

    // Read all the events from the event stream
    object_event e;
    while(eventReader.read(e)){
        if(e.t>=events.size()){
            events.resize(e.t+1);
        }
        events.at(e.t).push_back(e);
    }

    // Now insert the events into the stream
    int n=0, nNext=1;
    Frame f(0,0);
    bool first=true;
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }

        //fprintf(stderr, "Count = %d\n", events.at(n).size());
        insert_objects(f, events.at(n), p);

        sink.write(f);
        ++n;
    }

    return 0;
}
