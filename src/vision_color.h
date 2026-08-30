
#ifndef __VISION_COLOR_H__
#define __VISION_COLOR_H__

#include"vision_util.h"
#include <cstdlib>
#include <cstdint>
#include<memory.h>
#include <string>


namespace vision{


using ColorValueType = uint32_t;
enum class TColorType:int{
    ALONE = 1,
    COLOR_GAMUT = 2,
    NOT = 3,
    COLOR_GAMUT_NOT = 4,
};

struct TColorBase{
    TColorType type;
    unsigned char data[0];
};



struct Color{
    ColorValueType data;
    Color():data(0){}
    Color(ColorValueType data):data(data){}
    operator int()
    {
        return data;
    }
};


struct Pixel
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char o;
};

struct ColorGamut{
    ColorValueType color;
    ColorValueType shift;
};

struct ColorNot{
    ColorValueType data;
};

struct ColorGamutNot{
    ColorValueType color;
    ColorValueType shift;
};

struct ColorComposition{
    ColorComposition *next;
    TColorBase color;
};

constexpr int MAX_COLOR_SHIFT = 255*3;
constexpr int DECODE_COLOR_SHIFT = (sizeof(Color)-3)*8;

// Pixel-vs-color comparisons are templates on the pixel buffer's channel
// layout (RGBA_LAYOUT / BGRA_LAYOUT from vision_util.h), specialized at
// compile time so the hot loops carry no runtime lookup. Decoded Color
// objects store the logical 0xRRGGBB value.
template <PixelChannels PC>
inline int pixelChannel(const unsigned char* px, int channel)
{
    // channel: 0=R 1=G 2=B
    return channel == 0 ? px[PC.r] : (channel == 1 ? px[PC.g] : px[PC.b]);
}

template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,Color * c)->int{
    return abs(pixelChannel<PC>(color,0) - ((c->data >> 16) & 0xFF))
         + abs(pixelChannel<PC>(color,1) - ((c->data >> 8) & 0xFF))
         + abs(pixelChannel<PC>(color,2) - (c->data & 0xFF));
}

template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,ColorGamut * c)->int{
    // per-channel: how far outside the gamut box this pixel is
    int result = 0;
    int t;
    t = abs(pixelChannel<PC>(color,0) - (int)((c->color >> 16) & 0xFF)) - (int)((c->shift >> 16) & 0xFF);
    if (t > 0) result += t;
    t = abs(pixelChannel<PC>(color,1) - (int)((c->color >> 8) & 0xFF)) - (int)((c->shift >> 8) & 0xFF);
    if (t > 0) result += t;
    t = abs(pixelChannel<PC>(color,2) - (int)(c->color & 0xFF)) - (int)(c->shift & 0xFF);
    if (t > 0) result += t;
    return result;
}

template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,ColorNot * c)->int{
    return computeColorShiftSum<PC>(color,(Color*)c);
}


template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,ColorGamutNot * c)->int{
    // "!a-b": how deeply the pixel sits *inside* the gamut box; matching
    // (compareColor <= slack) then means "not in the box", which is the
    // intended semantics of the gamut-not form.
    int result = 0;
    int t;
    t = (int)((c->shift >> 16) & 0xFF) - abs(pixelChannel<PC>(color,0) - (int)((c->color >> 16) & 0xFF));
    if (t > 0) result += t;
    t = (int)((c->shift >> 8) & 0xFF) - abs(pixelChannel<PC>(color,1) - (int)((c->color >> 8) & 0xFF));
    if (t > 0) result += t;
    t = (int)(c->shift & 0xFF) - abs(pixelChannel<PC>(color,2) - (int)(c->color & 0xFF));
    if (t > 0) result += t;
    return result;
}


template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,TColorBase *c)->int{
    switch (c->type)
    {
    case TColorType::ALONE:
        return computeColorShiftSum<PC>(color,(Color*)c->data);
    case TColorType::COLOR_GAMUT:
        return computeColorShiftSum<PC>(color,(ColorGamut*)c->data);
    case TColorType::NOT:
        return computeColorShiftSum<PC>(color,(ColorNot*)c->data);
    case TColorType::COLOR_GAMUT_NOT:
        return computeColorShiftSum<PC>(color,(ColorGamutNot*)c->data);
    default:
        break;
    }
    return MAX_COLOR_SHIFT;
}


template <PixelChannels PC>
inline auto computeColorShiftSum(const unsigned char* color,ColorComposition * c)->int{
    int result = MAX_COLOR_SHIFT;
    while(c){
        int count = computeColorShiftSum<PC>(color,&c->color);
        if(count<result) result = count;
        if(result == 0) break;
        c = c->next;
    }
    return result;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,Color * c,int colorShiftSum)->int{
    return computeColorShiftSum<PC>(color,c) <= colorShiftSum;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,ColorGamut * c,int colorShiftSum)->int{
    return computeColorShiftSum<PC>(color,c) <= colorShiftSum;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,ColorNot * c,int colorShiftSum)->int{
    // "!rrggbb": the pixel matches when it is sufficiently *different* from
    // the color. `colorShiftSum` is the similarity slack (0 = exact), so the
    // pixel must differ by strictly more than that slack.
    return computeColorShiftSum<PC>(color,c) > colorShiftSum;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,ColorGamutNot * c,int colorShiftSum)->int{
    return computeColorShiftSum<PC>(color,c) <= colorShiftSum;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,TColorBase *c,int colorShiftSum)->int{
    switch (c->type)
    {
    case TColorType::ALONE:
        return compareColor<PC>(color,(Color*)c->data,colorShiftSum);
    case TColorType::COLOR_GAMUT:
        return compareColor<PC>(color,(ColorGamut*)c->data,colorShiftSum);
    case TColorType::NOT:
        return compareColor<PC>(color,(ColorNot*)c->data,colorShiftSum);
    case TColorType::COLOR_GAMUT_NOT:
        return compareColor<PC>(color,(ColorGamutNot*)c->data,colorShiftSum);
    default:
        break;
    }
    return 0;
}

template <PixelChannels PC>
inline auto compareColor(const unsigned char* color,ColorComposition * c,int colorShiftSum)->int{
    int result = 0;
    int index = 0;
    while(c){
        index++;
        if(compareColor<PC>(color,&c->color,colorShiftSum)){
            result = index;
            break;
        }
        c = c->next;
    }
    return result;
}





auto decodeColor(const char*str,int size,int *pos=nullptr)->ColorComposition*;
auto decodeColor(const char* str,int size,Color*color)->bool;
auto decodeColor(const char* str,int size,ColorNot *color)->bool;
auto decodeColor(const char* str,int size,ColorGamut*color)->bool;
auto decodeColor(const char* str,int size,ColorGamutNot*color)->bool;
auto encodeColor(const ColorComposition* color)->std::string;
void freeColorComposition(ColorComposition* color);

}  // namespace vision


#endif // __VISION_COLOR_H__