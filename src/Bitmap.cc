

#include "Bitmap.h"
#include "vision_util.h"
namespace vision {

//TODO 在模板图像宽度和高度都小于目标bitmap的情况下，可以优化
// The screen and the template may carry different pixel layouts (BGRA
// screen vs RGBA template decoded from PNG is the classic Android mix),
// so both layouts are template parameters here.
template <PixelChannels PC, PixelChannels TC>
auto isImage(Bitmap *bitmap, int x, int y, Bitmap *templateImage, int shiftSum)->bool{
  if(x<0 || y<0) return false;
  if(x+templateImage->width_>bitmap->width_ || y+templateImage->height_>bitmap->height_) return false;
  int nowShift = 0;
  for(int i=0;i<templateImage->height_;i++){
    for(int j=0;j<templateImage->width_;j++){
      nowShift += computeColorShiftSum<PC,TC>(computeCoordColor(bitmap,x+j,y+i),computeCoordColor(templateImage,j,i));
      if(nowShift>shiftSum){
        return false;
      }
    }
  }
  return true;
}

// Explicit instantiations for the four screen/template layout combinations.
template auto isImage<RGBA_LAYOUT,RGBA_LAYOUT>(Bitmap*,int,int,Bitmap*,int)->bool;
template auto isImage<RGBA_LAYOUT,BGRA_LAYOUT>(Bitmap*,int,int,Bitmap*,int)->bool;
template auto isImage<BGRA_LAYOUT,RGBA_LAYOUT>(Bitmap*,int,int,Bitmap*,int)->bool;
template auto isImage<BGRA_LAYOUT,BGRA_LAYOUT>(Bitmap*,int,int,Bitmap*,int)->bool;

// Runtime dispatch on the two formats. Declared in Bitmap.h.
bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum){
  switch(bitmap->format_){
    case PIXEL_BGRA:
      switch(templateImage->format_){
        case PIXEL_BGRA: return isImage<BGRA_LAYOUT,BGRA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
        default:         return isImage<BGRA_LAYOUT,RGBA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
      }
    default:
      switch(templateImage->format_){
        case PIXEL_BGRA: return isImage<RGBA_LAYOUT,BGRA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
        default:         return isImage<RGBA_LAYOUT,RGBA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
      }
  }
}

} //namespace vision
