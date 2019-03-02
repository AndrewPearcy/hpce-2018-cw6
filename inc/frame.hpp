#ifndef frame_hpp
#define frame_hpp

#include "jpeg_helpers.hpp"

#include <vector>
#include <cstdint>
#include <cassert>
#include <memory>

// Helper class for images
struct Frame
{
    Frame(int _w=0, int _h=0)
        : w(_w)
        , h(_h)
        , stride( ((3*w+3)/4)*4 ) // Make sure scanlines are padded to word length
        , pixels(h*stride, 0) // Zero initialised frame
    {}

    int w, h, stride;
    std::vector<uint8_t> pixels;

    void fill_grey(int _w, int _h)
    {
        w=_w;
        h=_h;
        stride=((w*3+3)/4)*4;
        pixels.assign( h*stride, 0x80 );
    }

    const uint8_t *get_rgb(int x, int y) const
    { return &pixels[y*stride+3*x]; }

    uint8_t *get_rgb(int x, int y)
    { return &pixels[y*stride+3*x]; }

    uint8_t get_r(int x, int y) const
    { return get_rgb(x,y)[0]; }

    uint8_t get_g(int x, int y) const
    { return get_rgb(x,y)[1]; }

    uint8_t get_b(int x, int y) const
    { return get_rgb(x,y)[2]; }

    void set_rgb(int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        get_rgb(x,y)[0]=r;
        get_rgb(x,y)[1]=g;
        get_rgb(x,y)[2]=b;
    }

    void set_rgb_safe(int x, int y, uint8_t r, uint8_t g, uint8_t b)
    {
        if(x<0 || w<=x || y<0 || h<=y){
            return;
        }
        get_rgb(x,y)[0]=r;
        get_rgb(x,y)[1]=g;
        get_rgb(x,y)[2]=b;
    }

    double get_luminance(int x, int y) const
    { return get_r(x,y)*0.299+get_g(x,y)*0.587+get_b(x,y)*0.114; }

    template<class TF>
    void binary_op_inplace(const Frame &b, TF f)
    {
        assert(w==b.w && h==b.h);
        
        for(int y=0; y<h; y++){
            for(int x=0; x<w; x++){
                f( get_rgb(x,y), get_rgb(x,y), b.get_rgb(x,y) );
            }
        }
    }

    template<class TF>
    void quad_op_inplace(const Frame &b, const Frame &c, const Frame &d, TF f)
    {
        for(int y=0; y<h; y++){
            for(int x=0; x<w; x++){
                f( get_rgb(x,y), get_rgb(x,y), b.get_rgb(x,y), c.get_rgb(x,y), d.get_rgb(x,y) );
            }
        }
    }

    template<class TF>
    Frame binary_op(const Frame &b, TF f) const
    {
        Frame res(*this); // Make a copy of this frame
        res.binary_op_inplace(b, f);
        return res;
    }

    template<class TF>
    Frame quad_op(const Frame &b, const Frame &c, const Frame &d, TF f) const
    {
        Frame res(*this); // Make a copy of this frame
        res.quad_op_inplace(b, c, d, f);
        return res;
    }
};


struct FrameReader
{
private:
    std::unique_ptr<jpeg_helpers::read_JPEG_file> reader;

public:
    FrameReader(FILE *src)
        : reader(new jpeg_helpers::read_JPEG_file(src) )
    {}

    FrameReader(FrameReader &&src)
    {
        std::swap(reader, src.reader);
    }

    FrameReader(const FrameReader &) = delete;
    FrameReader &operator=(const FrameReader &) = delete;

    bool read(Frame &res)
    {
        res.w=0;
        res.h=0;
        res.stride=0;
        res.pixels.resize(0);

        if(reader->read(res.w, res.h, res.pixels))
        {
            return false;
        }

        res.stride=((3*res.w+3)/4)*4;

        return true;
    }
};

struct FrameWriter
{
private:
    FILE *dst;
    const int quality=99;
public:

    FrameWriter(const FrameWriter &) = delete;
    FrameWriter &operator=(const FrameWriter &) = delete;

    FrameWriter(FILE *_dst)
        : dst(_dst)
    {}

    FrameWriter(FrameWriter &&orig)
        : dst(0)
    {
        std::swap(dst, orig.dst);
    }

    void write(const Frame &src)
    {
        jpeg_helpers::write_JPEG_file (src.w, src.h, src.pixels, dst, quality);
    }
};


#endif
