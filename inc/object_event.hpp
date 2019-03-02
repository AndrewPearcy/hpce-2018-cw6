#ifndef object_event_hpp
#define object_event_hpp

#include <cstdint>

#include "pattern.hpp"

struct object_event
{
    uint32_t t;
    int32_t x, y;
    int32_t scale;
    float strength;

    bool read(FILE *src)
    {
        int a,b,c,d;
        float e;
        if(5!=fscanf(src, "%d,%d,%d,%d,%f", &a,&b,&c,&d,&e)){
            return false;
        }
        t=a;
        x=b;
        y=c;
        scale=d;
        strength=e;
        return true;
    }

    void write(FILE *dst) const
    {
        fprintf(dst, "%d,%d,%d,%d,%f\n",t,x,y,scale,strength);
    }

    /* Draw a red bounding box around the pattern for visualisation purposes. */
    void draw(Frame &f, const pattern &p) const
    {
        for(int ox=x; ox<x+p.pw*scale; ox++){
            f.set_rgb_safe(ox,y-1, 255,0,0);
            f.set_rgb_safe(ox,y-2, 255,0,0);
            f.set_rgb_safe(ox,y-3, 255,0,0);
            f.set_rgb_safe(ox,y+p.ph*scale, 255,0,0);
            f.set_rgb_safe(ox,y+p.ph*scale+1, 255,0,0);
            f.set_rgb_safe(ox,y+p.ph*scale+2, 255,0,0);
        }
        for(int oy=y; oy<y+p.ph*scale; oy++){
            f.set_rgb_safe(x-1, oy, 255*strength,0,0);
            f.set_rgb_safe(x-2, oy, 255*strength,0,0);
            f.set_rgb_safe(x-3, oy, 255*strength,0,0);
            f.set_rgb_safe(x+p.pw*scale, oy, 255*strength,0,0);
            f.set_rgb_safe(x+p.pw*scale+1, oy, 255*strength,0,0);
            f.set_rgb_safe(x+p.pw*scale+2, oy, 255*strength,0,0);
        }
    }
};

class ObjectEventReader
{
private:
    FILE *m_src;
public:
    ObjectEventReader()
        : m_src(0)
    {}

    ObjectEventReader(FILE *src)
        : m_src(src)
    {}

    ObjectEventReader(ObjectEventReader &&o)
        : m_src(0)
    {
        std::swap(m_src, o.m_src);
    }

    bool read(object_event &e)
    {
        return e.read(m_src);
    }
};


#endif