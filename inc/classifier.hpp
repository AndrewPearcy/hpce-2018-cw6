#ifndef classifier_hpp
#define classifier_hpp

#include <vector>
#include <algorithm>
#include <cstdint>
#include <math.h>

#include "object_event.hpp"
#include "pattern.hpp"

std::vector<float> make_field(const Frame &f)
{
    std::vector<float> field(f.w*f.h);
    for(unsigned y=0; y<f.h; y++){
        for(unsigned x=0; x<f.w; x++){
            field[y*f.w+x]=(int(f.get_r(x,y))+f.get_g(x,y)+f.get_b(x,y))/(3.0*256);
        }
    }
    return field;
}

float pattern_strength_at_point(
    // Overall image field
    int fw, int fh,
    const float *field,
    // mask to look for
    int pw, int ph,
    const float *pmask,
    const float *ptemplate,
    // Position to try
    int ox, int oy
){
    float acc=0;
    float base=0;

    for(int y=0; y<ph; y++){
        for(int x=0;x<pw;x++){
            float f=field[(oy+y)*fw+(ox+x)];
    
            float mask=pmask[y*pw+x];
            if(mask!=0){
                float d=ptemplate[y*pw+x]-f;
                assert(-1<=d && d<=+1);
                float similarity=d*d;
                acc += mask * similarity;
                base += mask;
            }
        }
    }

    return 1-sqrt(acc / base);
}

void pattern_strength_render(
    Frame &src,
    Frame &dst,
    const pattern &p
){
    auto field=make_field(src);

    dst.fill_grey(src.w, src.h);

    for(int y=0; y<src.h-p.ph; y++){
        for(int x=0; x<src.w-p.pw; x++){
            float s=pattern_strength_at_point(
                src.w, src.h,
                &field[0],
                p.pw, p.ph,
                &p.mask[0],
                 &p.content[0],
                x,y
            );
            assert(0<=s && s<=1);
            dst.set_rgb(x,y, s*255, s*255, s*255);
        }
    }
}

void pattern_search_in_level(
    // Overall image field
    int fw, int fh,
    const float *field,
    // Pattern to search for
    int pw, int ph,
    const float *pmask,
    const float *ptemplate,
    int t,// Current time
    int scale, float threshold,
    // Output events array
    std::vector<object_event> &events
){
    for(int oy=0; oy<=fh-ph; oy++){
        for(int ox=0; ox<=fw-pw; ox++){
            float score=pattern_strength_at_point(
                fw, fh,
                field,
                pw, ph,
                pmask,
                ptemplate,
                ox, oy
            );

            //fprintf(stderr, "  s(%d,%d) = %f\n", ox, oy, score);

            if(score > threshold){
                object_event event;
                event.t=t;
                event.x=ox*scale;
                event.y=oy*scale;
                event.scale=scale;
                event.strength=score;
                events.push_back(event);
            }
        }
    }

    /*
    Disable. Here for next gen hardware, but not relevant yet.

    // Try the next coarser level
    if((fw > 2*pw+1) && (fh > 2*ph+1)){
        // Coursen through averaging
        int fws=fw/2, fhs=fh/2;
        std::vector<float> subfield(fws*fhs);
        for(int y=0; y<fhs; y++){
            for(int x=0; x<fws; x++){
                subfield[y*fws+x] = (field[2*y*fw+2*x] + field[2*y*fw+2*x+1] + field[(2*y+1)*fw+2*x] + field[(2*y+1)*fw+2*x+1])/4;
            }
        }

        // Search in the resampled level
        pattern_search_in_level(fws, fhs, &subfield[0], pw, ph, pmask, ptemplate, t, scale, threshold, events);
    }
    */
}

void pattern_search(
    const Frame &f,
    const pattern &p,
    int t,
    int scale, float threshold,
    std::vector<object_event> &events
){
    auto field=make_field(f);

    pattern_search_in_level(
        f.w, f.h, &field[0],
        p.pw, p.ph, &p.mask[0], &p.content[0],
        t,
        scale, threshold,
        events
    );

    // Order from highest score to lowest
    std::sort(events.begin(), events.end(), [](const auto &a, const auto &b) -> bool{
        if(a.strength > b.strength) return true;
        if(a.strength < b.strength) return false;
        // Create a total order
        if(a.x<b.x) return true;
        if(a.x>b.x) return false;
        return a.y<b.y;
    });

    std::vector<object_event> selected;

    // Build up the final list of 8
    while(selected.size()<8 && events.size()>0){
        auto e=events.front();
        events.erase(events.begin());
        selected.push_back(e);

        // Strip out any events which are:
        // - At the same scale;
        // - AND within 2 scale units of distance
        auto it=events.begin();
        while(it!=events.end()){
            if(it->scale==e.scale && std::abs(it->x-e.x)<=2 && std::abs(it->y-e.y)<=2){
                it=events.erase(it);
            }else{
                ++it;
            }
        }
    }

    events=selected;
};

#endif
