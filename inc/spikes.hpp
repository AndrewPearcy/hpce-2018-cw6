#ifndef spikes_hpp
#define spikes_hpp

#include <cstdint>
#include <stdexcept>

struct spike_info
{
    uint32_t t;
    uint16_t w;
    uint16_t h;
    uint16_t x;
    uint16_t y;
    uint8_t r, g, b;
    uint8_t _pad_;
};

static_assert(sizeof(spike_info)==16, "Expected spike_info size to be 16 for portability.");

class BinarySpikeWriter
{
private:
    FILE *m_dst = 0;
public:
    BinarySpikeWriter &operator=(const BinarySpikeWriter &) = delete;
    BinarySpikeWriter(const BinarySpikeWriter &) = delete;

    BinarySpikeWriter(FILE *dst)
        : m_dst(dst)
    {}

    BinarySpikeWriter(BinarySpikeWriter &&x)
    {
        std::swap(m_dst, x.m_dst);
    }

    ~BinarySpikeWriter()
    {
        if(m_dst){
            fclose(m_dst);
        }
    }

    void write(const spike_info &spike)
    {
        if(1 != fwrite(&spike, sizeof(spike), 1, m_dst)){
            throw std::runtime_error("Couldn't write spike.");
        }
    }
};

class BinarySpikeReader
{
private:
    FILE *m_src = 0;
public:
    BinarySpikeReader &operator=(const BinarySpikeReader &) = delete;
    BinarySpikeReader(const BinarySpikeReader &) = delete;

    BinarySpikeReader(FILE *src)
        : m_src(src)
    {}

    BinarySpikeReader(BinarySpikeReader &&x)
    {
        std::swap(m_src, x.m_src);
    }

    bool read(spike_info &spike)
    {
        return 1 == fread(&spike, sizeof(spike), 1, m_src);
    }
};

#endif
