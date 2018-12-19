#pragma once
#include "texture.h"
class SphericalBackground
{
public:
	SphericalBackground() : texture(texture) {};
	SphericalBackground(const char filename[]);
	Color4f GetTexel(const float x, const float y, const float z);
	~SphericalBackground();
private:
	Texture *texture;
};