

#include "CommonBitmap.h"
#include<lodepng.h>

namespace vision {

CommonBitmap::CommonBitmap()
	: Bitmap(),error_(nullptr)
{
	origin_ = nullptr;
	pixelStride_ = 4;
}


bool CommonBitmap::toBoolResult(unsigned int error)
{
	if(error)
	{
		this->error_ = lodepng_error_text(error);;
		return false;
	}
	rowShift_ = pixelStride_ * width_;
	origin_ = data_.data();
	return true;
}
bool CommonBitmap::load(const unsigned char* data, unsigned int size)
{
	lodepng::State state;
	data_.clear();
	auto error = lodepng::decode(this->data_,this->width_,this->height_,state,data,size);
	return toBoolResult(error);
}

bool CommonBitmap::load(const char* path)
{
	data_.clear();
	auto error = lodepng::decode(this->data_,this->width_,this->height_,path);
	return toBoolResult(error);
}

void CommonBitmap::load(Bitmap * source, int x, int y, int width, int height)
{
	data_.clear();
	width_ = width;
	height_ = height;
	pixelStride_ = source->pixelStride_;
	rowShift_ = pixelStride_ * width_;
	data_.resize(rowShift_ * height_);
	origin_ = data_.data();
	for(int i=0;i<height;i++)
	{
		memcpy(data_.data() + i * rowShift_,source->origin_ + (y + i) * source->rowShift_ + x * pixelStride_,rowShift_);
	}

}
} // namespace vision



