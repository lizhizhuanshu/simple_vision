
#include "vision_color.h"
#include <cstdio>
#include <iostream>

constexpr int ALONE_COLOR_STR_SIZE = 6;
constexpr int COLOR_NOT_STR_SIZE = ALONE_COLOR_STR_SIZE+1;
constexpr int COLOR_GAMUT_STR_SIZE = ALONE_COLOR_STR_SIZE*2+1;
constexpr int COLOR_GAMUT_NOT_STR_SIZE = ALONE_COLOR_STR_SIZE*2+2;

namespace vision { 


auto rawDecodeColor(const char* str,ColorValueType&data)->bool{
  data = 0;
  for(int i = 0;i< ALONE_COLOR_STR_SIZE;i++){
    data <<= 4;
    if(str[i]>='0' && str[i]<='9'){
      data += str[i]-'0';
    }else if(str[i]>='a' && str[i]<='f'){
      data += str[i]-'a'+10;
    }else if(str[i]>='A' && str[i]<='F'){
      data += str[i]-'A'+10;
    }else{
      return false;
    }
  }
  return true;
}
  
auto decodeColor(const char* str,int size,Color*color)->bool{
  if(size < ALONE_COLOR_STR_SIZE) return false;
  return rawDecodeColor(str,color->data);
}

auto decodeColor(const char* str,int size,ColorNot *color)->bool{
  if(size < COLOR_NOT_STR_SIZE && str[0] !='!') return false;
  return rawDecodeColor(str+1,color->data);
}

auto decodeColor(const char* str,int size,ColorGamut*color)->bool{
  if(size < COLOR_GAMUT_STR_SIZE && str[6]!='-') return false;
  if(!rawDecodeColor(str,color->color)) return false;
  return rawDecodeColor(str+ALONE_COLOR_STR_SIZE+1,color->shift);
}

auto decodeColor(const char* str,int size,ColorGamutNot*color)->bool{
  if(size < COLOR_GAMUT_NOT_STR_SIZE && str[0] !='!' &&  str[7]!='-') return false;
  if(!rawDecodeColor(str+1,color->color)) return false;
  return rawDecodeColor(str+ALONE_COLOR_STR_SIZE+2,color->shift);
}

void freeColorComposition(ColorComposition *color){
  while(color != nullptr){
    auto next = color->next;
    MY_FREE(color);
    color = next;
  }
}


auto decodeColor(const char* str,int size,int*pos)->ColorComposition*{
  int index = 0;
  int remain;
  bool result = true;
  ColorComposition root;
  root.next = nullptr;
  ColorComposition* now = &root;
  while (index <size &&  (remain =size-index)>=ALONE_COLOR_STR_SIZE) {
    if(str[index] == '!'){
      if(remain < COLOR_NOT_STR_SIZE) {
        result = false;
        break;
      }
      if(remain>= COLOR_GAMUT_NOT_STR_SIZE && str[index+7] == '-'){
        ColorComposition * data = (ColorComposition*)MY_MALLOC(sizeof(ColorGamutNot)+sizeof(ColorComposition));
        if(data == nullptr) {
          result = false;
          break;
        }
        data->color.type = TColorType::COLOR_GAMUT_NOT;
        data->next = nullptr;
        now->next = data;
        now = data;
        if(!decodeColor(str+index,remain,(ColorGamutNot*)data->color.data)){
          result = false;
          break;
        }
        index += COLOR_GAMUT_NOT_STR_SIZE;
      }else{
        ColorComposition * data = (ColorComposition*)MY_MALLOC(sizeof(ColorNot)+sizeof(ColorComposition));
        if(data == nullptr) {
          result = false;
          break;
        }
        data->color.type = TColorType::NOT;
        data->next = nullptr;
        now->next = data;
        now = data;
        if(!decodeColor(str+index,remain,(ColorNot*)data->color.data)){
          result = false;
          break;
        }
        index += COLOR_NOT_STR_SIZE;
      }
    }else if(str[index+6] == '-'){
      if(remain < COLOR_GAMUT_STR_SIZE){
        result = false;
        break;
      }
      ColorComposition * data = (ColorComposition*)MY_MALLOC(sizeof(ColorGamut)+sizeof(ColorComposition));
      if(data == nullptr) {
        result = false;
        break;
      }
      data->color.type = TColorType::COLOR_GAMUT;
      data->next = nullptr;
      now->next = data;
      now = data;
      if(!decodeColor(str+index,remain,(ColorGamut*)data->color.data)){
        result = false;
        break;
      }
      index += COLOR_GAMUT_STR_SIZE;
    }else{
      if(remain < ALONE_COLOR_STR_SIZE){
        result = false;
        break;
      }
      ColorComposition * data = (ColorComposition*)MY_MALLOC(sizeof(Color)+sizeof(ColorComposition));
      if(data == nullptr) {
        result = false;
        break;
      }
      data->color.type = TColorType::ALONE;
      data->next = nullptr;
      now->next = data;
      now = data;
      if(!decodeColor(str+index,remain,(Color*)data->color.data)){
        result = false;
        break;
      }
      index += ALONE_COLOR_STR_SIZE;
    }
    if(index<size){
      if(str[index] != '|'){
        break;
      }
      index++;
    }
  }
  if (!result) {
    freeColorComposition(root.next);
    return nullptr;
  }
  if(pos != nullptr){
    *pos = index;
  }
  return root.next;
}

static auto encodeColor(Color*color,std::string&out){
  auto data = color->data;
	char str[8];
  sprintf(str ,"%06x",data);
  out.append(str,6);
}

static auto encodeColor(ColorGamut*color,std::string&out){
  auto data = color->color;
  auto shift = color->shift;
  char str[15];
  sprintf(str,"%06x-%06x",data,shift);
  out.append(str,13);
}

auto encodeColor(const ColorGamutNot*color,std::string&out){
  auto data = color->color;
  auto shift = color->shift;
  char str[16];
  sprintf(str,"!%06x-%06x",data,shift);
  out.append(str,14);
}

auto encodeColor(const ColorNot*color,std::string&out){
  auto data = color->data;
  char str[9];
  sprintf(str,"!%06x",data);
  out.append(str,8);
}

auto encodeColor(const ColorComposition *color)->std::string{
  std::string result;
  while(color != nullptr){
    switch (color->color.type)
    {
    case TColorType::ALONE:
      encodeColor((Color*)color->color.data,result);
      break;
    case TColorType::COLOR_GAMUT:
      encodeColor((ColorGamut*)color->color.data,result);
      break;
    case TColorType::NOT:
      encodeColor((ColorNot*)color->color.data,result);
      break;
    case TColorType::COLOR_GAMUT_NOT:
      encodeColor((ColorGamutNot*)color->color.data,result);
      break;
    default:
      break;
    }
    color = color->next;
    if(color != nullptr){
      result.push_back('|');
    }
  }
  return result;


}

}

