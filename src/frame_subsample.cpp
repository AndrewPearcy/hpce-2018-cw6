#include "frame.hpp"


Frame merge_frames(const std::vector<Frame> &frames)
{
    int w=frames.front().w, h=frames.front().h;
    int stride=frames.front().stride;
    std::vector<float> acc(h*w*3);  // Accumulate in floating-point

    for(const auto &f : frames){
        for(int y=0; y<h; y++){
            for(int x=0; x<w; x++){
                acc[y*w*3+x*3+0] += f.get_r(x,y);
                acc[y*w*3+x*3+1] += f.get_g(x,y);
                acc[y*w*3+x*3+2] += f.get_b(x,y);

            }
        }
    }

    int n=frames.size();

    Frame res(w,h);
    for(int y=0; y<h; y++){
        for(int x=0; x<w; x++){
            res.set_rgb(x,y, acc[y*w*3+x*3]/n, acc[y*w*3+x*3+1]/n, acc[y*w*3+x*3+2]/n); 
        }
    }
    return res;
}


int main(int argc, char *argv[])
{
    fprintf(stderr, "Subsampling frames from stdin to stdout.\n");
    FrameReader source(stdin);
    FrameWriter sink(stdout);

    int oversample=100;
    if(argc>1){
        oversample=atoi(argv[1]);
    }
    if(oversample<1){
        fprintf(stderr, "Can't oversample by less than 1.\n");
        exit(1);
    }

    // The shutter is open for this much of the frame
    float eShutterSpeed=0.3;

    int shutterOpen=std::max(1, (int)(eShutterSpeed*oversample));
    int shutterClosed=oversample-shutterOpen;

    int n=0, nNext=1;
    Frame f(0,0);
    std::vector<Frame> acc;
    while(source.read(f)){
        if(n==nNext){
            fprintf(stderr, "Frame %u.\n", n);
            nNext=2*nNext;
        }
        int offset=n%oversample;
        if(shutterClosed <= offset){
            acc.push_back(f);
            if(offset+1 == oversample){
                f=merge_frames(acc);
                sink.write(f);
                acc.clear();
            }
        }
        
        ++n;
    }

    return 0;
}
