#ifndef WINDY_GEOMETRY_EXTENSIONS_H
#define WINDY_GEOMETRY_EXTENSIONS_H

#include <cmath>
#include <float.h>

#ifndef M_PI
  #define M_PI  3.14159265358979323846264f  // from CRC
#endif

namespace windy {
	class GeometryExtensions {
	public:
		static windy::Rect rectIntersection(const windy::Rect& rect1, const windy::Rect& rect2) {
			auto intersection = 
				windy::Rect(std::max(rect1.origin.x, rect2.origin.x),
							  std::max(rect1.origin.y, rect2.origin.y),
							  0,
							  0);

			intersection.size.x =
				std::min(rect1.origin.x + rect1.size.x, rect2.origin.x + rect2.size.x) - intersection.origin.x;

			intersection.size.y =
				std::min(rect1.origin.y + rect1.size.y, rect2.origin.y + rect2.size.y) - intersection.origin.y;

			return intersection;

		}

		static bool rectIntersectsRect(const windy::Rect& rect1, const windy::Rect& rect2) {
			return !(rect1.getMaxX()  < rect2.getMinX() ||
					 rect2.getMaxX()  < rect1.getMinX() ||
					 rect1.getMaxY()  < rect2.getMinY() ||
					 rect2.getMaxY()  < rect1.getMinY());

		}

		static glm::vec2 moveTowards(const glm::vec2& current, const glm::vec2& target, float maxDistanceDelta = 1.0f/60.0f) {
			auto a = target - current;
			float magnitude = a.length();
			if (magnitude <= maxDistanceDelta || magnitude == 0.0f)
			{
				return target;
			}
			return current + a / magnitude * maxDistanceDelta;

		}

		static float degreesToRadians(float degrees) {
			return static_cast<float>(degrees * M_PI / 180.0);
		}

		static float radiansToDegrees(float radians) {
			return static_cast<float>(radians * (180.0 / M_PI));
		}

	};
}


#endif
