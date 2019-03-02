#include "object_event.hpp"
#include <random>
#include <cmath>

int main(int argc, char *argv[])
{
    int oversample=100;
    if(argc>1){
        oversample=atoi(argv[1]);
    }

    ObjectEventReader eventReader(stdin);

    std::vector<std::vector<object_event> > events;

    // Read all the events from the event stream
    object_event e;
    while(eventReader.read(e)){
        if(e.t>=events.size()){
            events.resize(e.t+1);
        }
        events.at(e.t).push_back(e);
    }

    // Now dump them back out oversampled
    for(int i=0; i<events.size(); i++){
        for(int t=0; t<oversample; t++){
            const auto &ev = events[i];
            for(auto e : ev){
                e.t=(i+1)*oversample-1+t;
                e.write(stdout);
            }
        }
    }

    return 0;
}
