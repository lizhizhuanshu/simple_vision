
#ifndef __VISION_COLOR_H__
#define __VISION_COLOR_H__

#include"vision_util.h"
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

inline auto computeColorShiftSum(const unsigned char* color,Color * c)->int{
    return computeColorShiftSum(color,(unsigned char*)c);
}

inline auto computeColorShiftSum(const unsigned char* color,ColorGamut * c)->int{
    return computeColorShiftSum(color,(unsigned char*)&c->color,(unsigned char*)&c->shift);
}

inline auto computeColorShiftSum(const unsigned char* color,ColorNot * c)->int{
    return computeColorShiftSum(color,(unsigned char*)&c->data);
}


inline auto computeColorShiftSum(const unsigned char* color,ColorGamutNot * c)->int{
    return computeColorGamutNotShiftSum(color,(unsigned char*)&c->color,(unsigned char*)&c->shift);
}


inline auto computeColorShiftSum(const unsigned char* color,TColorBase *c)->int{
    switch (c->type)
    {
    case TColorType::ALONE:
        return computeColorShiftSum(color,(Color*)c->data);
    case TColorType::COLOR_GAMUT:
        return computeColorShiftSum(color,(ColorGamut*)c->data);
    case TColorType::NOT:
        return computeColorShiftSum(color,(ColorNot*)c->data);
    case TColorType::COLOR_GAMUT_NOT:
        return computeColorShiftSum(color,(ColorGamutNot*)c->data);
    default:
        break;
    }
    return MAX_COLOR_SHIFT;
}


inline auto computeColorShiftSum(const unsigned char* color,ColorComposition * c)->int{
    int result = MAX_COLOR_SHIFT;
    while(c){
        int count = computeColorShiftSum(color,&c->color);
        if(count<result) result = count;
        if(result == 0) break;
        c = c->next;
    }
    return result;
}

inline auto compareColor(const unsigned char* color,Color * c,int colorShiftSum)->int{
    return computeColorShiftSum(color,c) <= colorShiftSum;
}

inline auto compareColor(const unsigned char* color,ColorGamut * c,int colorShiftSum)->int{
    return computeColorShiftSum(color,c) <= colorShiftSum;
}

inline auto compareColor(const unsigned char* color,ColorNot * c,int colorShiftSum)->int{
    return computeColorShiftSum(color,c) <= colorShiftSum;
}

inline auto compareColor(const unsigned char* color,ColorGamutNot * c,int colorShiftSum)->int{
    return computeColorShiftSum(color,c) <= colorShiftSum;
}

inline auto compareColor(const unsigned char* color,TColorBase *c,int colorShiftSum)->int{
    switch (c->type)
    {
    case TColorType::ALONE:
        return compareColor(color,(Color*)c->data,colorShiftSum);
    case TColorType::COLOR_GAMUT:
        return compareColor(color,(ColorGamut*)c->data,colorShiftSum);
    case TColorType::NOT:
        return compareColor(color,(ColorNot*)c->data,colorShiftSum);
    case TColorType::COLOR_GAMUT_NOT:
        return compareColor(color,(ColorGamutNot*)c->data,colorShiftSum);
    default:
        break;
    }
    return 0;
}

inline auto compareColor(const unsigned char* color,ColorComposition * c,int colorShiftSum)->int{
    int result = 0;
    int index = 0;
    while(c){
        index++;
        if(compareColor(color,&c->color,colorShiftSum)){
            result = index;
            break;
        }
        c = c->next;
    }
    return result;
}





auto decodeColor(const char*str,int size,int *pos=nullptr)->ColorComposition*;
auto encodeColor(const ColorComposition* color)->std::string;
void freeColorComposition(ColorComposition* color);

}  // namespace vision


#endif // __VISION_COLOR_H__