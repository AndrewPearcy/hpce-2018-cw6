#ifndef pattern_hpp
#define pattern_hpp

#include <vector>
#include <algorithm>
#include <cstdint>
#include <sstream>

#include "frame.hpp"

//#include "object_event.hpp"


/*
    Mask is a value between 0 and 1 indicating whether the pixel contributes, and if so how much
    Content is a value beteen 0 and 1 indicating expected value.
*/
struct pattern
{
    int pw, ph;
    std::vector<float> mask;
    std::vector<float> content;

    pattern()
        : pw(0)
        , ph(0)
    {}

    pattern(const Frame &m)
    {
        pw=m.w;
        ph=m.h;
        mask.resize(pw*ph);
        content.resize(pw*ph);
        for(int x=0; x<pw; x++){
            for(int y=0; y<ph; y++){
                mask[y*pw+x]=m.get_r(x,y);
                content[y*pw+x]=m.get_g(x,y);
            }
        }
    }

    float m(int x, int y) const
    {
        if(x<0 || x>=pw || y<0 || y>=ph) return 0;
        return mask[y*pw+x];
    }

    float c(int x, int y) const
    {
        if(x<0 || x>=pw || y<0 || y>=ph) return 0;
        return content[y*pw+x];
    }

    static pattern test_pattern()
    {
        const int W=9, H=9;
        const float mask[W*H]={
            0,1,1,1,1,1,1,1,0,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,1,
            0,1,1,1,1,1,1,1,0
        };
        const float content[W*H]={
            0,1,1,1,1,1,1,1,0,
            1,1,1,1,1,1,1,1,1,
            1,1,0,0,0,0,0,1,1,
            1,1,0,0,0,0,0,1,1,
            1,1,0,0,0,0,0,1,1,
            1,1,0,0,0,0,0,1,1,
            1,1,0,0,0,0,0,1,1,
            1,1,1,1,1,1,1,1,1,
            0,1,1,1,1,1,1,1,0
        };

        pattern res;
        res.pw=W;
        res.ph=H;
        res.mask.assign(mask,mask+W*H);
        res.content.assign(content,content+W*H);
        return res;
    }
};

#endif
