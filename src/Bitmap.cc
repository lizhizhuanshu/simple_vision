


#include "Bitmap.h"
#include "vision_util.h"

namespace vision {

// Runtime entry: dispatch the screen/template layout pair once per call.
bool isImage(Bitmap*bitmap,int x,int y,Bitmap* templateImage,int shiftSum){
  switch(bitmap->format_){
    case PIXEL_BGRA:
      switch(templateImage->format_){
        case PIXEL_BGRA: return isImagePair<BGRA_LAYOUT,BGRA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
        default:         return isImagePair<BGRA_LAYOUT,RGBA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
      }
    default:
      switch(templateImage->format_){
        case PIXEL_BGRA: return isImagePair<RGBA_LAYOUT,BGRA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
        default:         return isImagePair<RGBA_LAYOUT,RGBA_LAYOUT>(bitmap,x,y,templateImage,shiftSum);
      }
  }
}

} //namespace vision
