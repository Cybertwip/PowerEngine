#pragma once

#include <algorithm>
#include <functional>
#include <cmath>

#include "glm/glm.hpp"

namespace windy {
#define MATH_TOLERANCE 2e-37f

typedef glm::ivec3 Vec;

typedef glm::ivec2 Size;

class Rect
{
public:
	Vec origin;
	Size  size;
	
public:
	Rect();
	Rect(float x, float y, float width, float height);
	Rect(const Rect& other);
	void setRect(float x, float y, float width, float height);
	Rect& operator= (const Rect& other);
	float getMinX() const;
	float getMidX() const;
	float getMaxX() const;
	float getMinY() const;
	float getMidY() const;
	float getMaxY() const;
	bool containsPoint(const Vec& point) const;
};

inline Rect::Rect()
{
	Rect(0.0f, 0.0f, 0.0f, 0.0f);
}

inline Rect::Rect(float x, float y, float width, float height)
{
	setRect(x, y, width, height);
}

inline Rect::Rect(const Rect& other)
{
	setRect(other.origin.x, other.origin.y, other.size.x, other.size.y);
}

inline Rect& Rect::operator= (const Rect& other)
{
	setRect(other.origin.x, other.origin.y, other.size.x, other.size.y);
	return *this;
}

inline void Rect::setRect(float x, float y, float width, float height)
{
	origin.x = x;
	origin.y = y;
	
	size.x = width;
	size.y = height;
}

inline bool Rect::containsPoint(const Vec& point) const
{
	bool bRet = false;
	
	if (point.x >= getMinX() && point.x <= getMaxX()
		&& point.y >= getMinY() && point.y <= getMaxY())
	{
		bRet = true;
	}
	
	return bRet;
}

inline float Rect::getMaxX() const
{
	return origin.x + size.x;
}

inline float Rect::getMidX() const
{
	return origin.x + size.x / 2.0f;
}

inline float Rect::getMinX() const
{
	return origin.x;
}

inline float Rect::getMaxY() const
{
	return origin.y + size.y;
}

inline float Rect::getMidY() const
{
	return origin.y + size.y / 2.0f;
}

inline float Rect::getMinY() const
{
	return origin.y;
}


}


