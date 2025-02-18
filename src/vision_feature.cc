

#include "vision_feature.h"
#include "vision_color.h"
#include "vision_util.h"
#include <cstdlib>

namespace vision {

  auto inline isNumber(const char c)->bool{
    return c>='0' && c<='9';
  }

  auto toInt(const char* str, int size,int&pos, int& value)->bool{
    value = 0;
    int sign = 1;
    if(str[pos] == '-'){
      if(size-pos<2 || !isNumber(str[pos+1]))return false;
      sign = -1;
      pos++;
    }else if(!isNumber(str[pos])){
      return false;
    }
    char c = 0;
    while (true) {
      if(pos>=size )break;
      c = str[pos];
      if(c == '|' || c == '\0')break;
      if(!isNumber(c))return false;
      value = value*10 + c-'0';
      pos++;
    }
    value *= sign;
    return true;
  }

  auto decodeFeature(const char *str, int size, FeatureCompositionRoot *feature)->bool{
    int x,y;
    int pos = 0;
    feature->count = 0;
    feature->data = nullptr;
    ColorComposition *color = nullptr;
    FeatureComposition root;
    root.next = nullptr;
    FeatureComposition *last = &root;
    int cPos = 0;
    bool result =false;
    while (pos<size) {
      if(!toInt(str, size, pos, x))break;
      if(pos>=size || str[pos]!='|')break;
      pos++;
      if(!toInt(str, size, pos, y))break;
      if(pos>=size || str[pos]!='|')break;
      pos++;
      color = decodeColor(str+pos, size-pos,&cPos);
      if(color == nullptr)break;
      pos += cPos;
      feature->count++;
      auto f = (FeatureComposition*)MY_MALLOC(sizeof(FeatureComposition));
      if(f == nullptr){
        freeColorComposition(color);
        break;
      }
      f->x = x;
      f->y = y;
      f->color = color;
      f->next = nullptr;
      last->next = f;
      last = f;

      if(pos>=size || str[pos]!= ','){
        result = true;
        break;
      }
      pos++;
    }
    feature->data = root.next;
    if(!result){
      freeFeatureComposition(feature);
    }
    return result;
  }

  auto freeFeatureComposition(FeatureCompositionRoot *feature)->void{
    auto f = feature->data;
    while(f != nullptr){
      auto next = f->next;
      freeColorComposition(f->color);
      MY_FREE(f);
      f = next;
    }
  }

  auto encodeFeature(FeatureCompositionRoot *feature)->std::string{
    std::string result;
    auto f = feature->data;
    while(f != nullptr){
      result += std::to_string(f->x);
      result += '|';
      result += std::to_string(f->y);
      result += '|';
      result += encodeColor(f->color);
      f = f->next;
      if(f != nullptr){
        result += ',';
      }
    }
    return result;
  }


  auto isFeature(Bitmap *bitmap, FeatureCompositionRoot *feature, int shiftSum)->bool{
    auto f = feature->data;
    int nowShift = 0;
    while(f != nullptr){
      nowShift += computeColorShiftSum(computeCoordColor(bitmap, f->x, f->y),f->color);
      if(nowShift>shiftSum){
        return false;
      }
      f = f->next;
    }
    return true;
  }

  auto isFeature(Bitmap *bitmap, int x, int y, FeatureCompositionRoot *feature, int shiftSum)->bool{
    auto f = feature->data;
    int nowShift = 0;
    int nowX = 0;
    int nowY = 0;
    while(f != nullptr){
      nowX = x+f->x;
      nowY = y+f->y;
      if(isInBitmapScope(bitmap, nowX, nowY))
        nowShift += computeColorShiftSum(computeCoordColor(bitmap, nowX,nowY),f->color);
      else
        nowShift += MAX_COLOR_SHIFT;
      if(nowShift>shiftSum){
        return false;
      }
      f = f->next;
    }
    return true;
  }
}