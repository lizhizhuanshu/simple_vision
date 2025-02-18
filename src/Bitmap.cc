
#include "Bitmap.h"
#include "vision_util.h"
namespace vision {

//TODO 在模板图像宽度和高度都小于目标bitmap的情况下，可以优化
auto isImage(Bitmap *bitmap, int x, int y, Bitmap *templateImage, int shiftSum)->bool{
  if(x<0 || y<0) return false;
  if(x+templateImage->width_>bitmap->width_ || y+templateImage->height_>bitmap->height_) return false;
  int nowShift = 0;
  for(int i=0;i<templateImage->height_;i++){
    for(int j=0;j<templateImage->width_;j++){
      nowShift += computeColorShiftSum(computeCoordColor(bitmap,x+j,y+i),computeCoordColor(templateImage,j,i));
      if(nowShift>shiftSum){
        return false;
      }
    }
  }
  return true;
}



} //namespace vision