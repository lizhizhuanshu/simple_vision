#ifndef SVISION_PNG_IMAGE_H
#define SVISION_PNG_IMAGE_H

#include"Bitmap.h"

#include <functional>
#include <string>
#include <vector>


namespace vision{

using ResourceLoader = std::function<bool(const std::string& path, std::string& out)>;

class CommonBitmap :public Bitmap
{
	std::vector<unsigned char> data_;
	const char* error_;
	static ResourceLoader resourceLoader_;
public:
	CommonBitmap();

	// Register a user-supplied loader for relative paths. Return true and fill
	// `out` with image bytes, or return false to fall back to direct file access.
	static void setResourceLoader(ResourceLoader loader);

	bool toBoolResult(unsigned int error);
	bool load(const unsigned char* data, unsigned int size);
	bool load(const char* path);
	void load(Bitmap * source,int x,int y,int width,int height);
	const char* errorText(){
		return error_;
	}
};



} // namespace vision



#endif //SVISION_PNG_IMAGE_H
