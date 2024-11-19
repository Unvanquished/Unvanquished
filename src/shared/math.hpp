#ifndef MATH_HPP
#define MATH_HPP

#include <glm/vec3.hpp>
//because I'm lazy and glm::length2() is going to be used so often anyway
#include <glm/gtx/norm.hpp>

glm::vec3 SafeNormalize( glm::vec3 const& o );

#define xstr(s) str(s)
#define str(s) #s

#define normalize_warn( v )                  \
	do                                         \
	{                                          \
		if ( glm::length2( ( v ) ) == 0.f )      \
		{                                        \
			char const norm_warn_str[] = "length of 0 in " __FILE__ ": %s at " str( __LINE__ ) ", please report the bug"; \
			Log::Warn( norm_warn_str,  __func__ ); \
		}                                        \
	} while(0)

struct trajectory_t;

glm::vec3 BG_EvaluateTrajectory( const trajectory_t *tr, int atTime );
glm::vec3 BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime );

#endif
