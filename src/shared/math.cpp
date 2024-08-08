#include "shared/math.hpp"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

glm::vec3 SafeNormalize( glm::vec3 const& o )
{
	return glm::length2( o ) != 0.f ? glm::normalize( o ) : o;
}
