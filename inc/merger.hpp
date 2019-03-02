#ifndef merger_hpp
#define merger_hpp

#include <vector>
#include <algorithm>
#include <cstdint>
#include <sstream>

#include "frame.hpp"
#include "pattern.hpp"
#include "object_event.hpp"



void insert_object_at(
    Frame &f,
    int ox, int oy,
    int scale,
    float strength,
    const pattern &p
){
    assert(scale>0);
    assert(scale<f.w && scale<f.h);

    for(int px=0; px<p.pw; px++){
        for(int py=0; py<p.ph; py++){
            auto m=p.m(px,py);
            auto val=p.c(px,py);
            assert(0<=val && val<=1);
            val*=255;
            //fprintf(stderr, "Adding at pxy=(%d,%d), m=%f, val=%g\n", px, py, m, val);
            for(int x=ox+px*scale; x<ox+(px+1)*scale; x++){
                for(int y=oy+py*scale; y<oy+(py+1)*scale; y++){
                    if(m==0 || (x<0) || (f.w<=x) || (y<0) || (f.h<=y)){
                        continue;
                    }
                    
                    
                    float r=f.get_r(x,y), g=f.get_g(x,y), b=f.get_b(x,y);
                    //fprintf(stderr, "   (%f,%f,%f)\n", r,g,b);
                    float p=strength*strength*m;
                    r=p*val + (1-p)*r;
                    g=p*val + (1-p)*g;
                    b=p*val + (1-p)*b;
                    //fprintf(stderr, "   (%f,%f,%f)\n", r,g,b);
                    f.set_rgb(x,y, r, g, b);
                }
            }
        }
    }
}

void insert_objects(
    Frame &f,
    const std::vector<object_event> &event,
    const pattern &p
){
    for(const auto &e : event){
        insert_object_at(f, e.x, e.y, e.scale, e.strength, p);
    }
}

#endif
