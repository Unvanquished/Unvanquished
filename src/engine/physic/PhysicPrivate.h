/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the <organization> nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef PHYSIC_PHYSICPRIVATE_H_
#define PHYSIC_PHYSICPRIVATE_H_

#include <btBulletCollisionCommon.h>
#include "Physic.h"
#include "../../common/Log.h"
#include "../qcommon/surfaceflags.h"

namespace Physic {
    extern btCollisionDispatcher* dispatcher;
    extern btDbvtBroadphase* broadphase;
    extern btDefaultCollisionConfiguration* collisionConf;
    extern btCollisionWorld* world;

    inline btVector3 q3ToBullet(const vec3_t vec) {
        if(vec) {
            return btVector3(vec[0], vec[1], vec[2]);
        } else {
            return btVector3(0, 0, 0);
        }
    }

    inline void bulletToQ3(btVector3 from, vec3_t to) {
        to[0] = from[0];
        to[1] = from[1];
        to[2] = from[2];
    }

    inline short convertCollisionMask(int mask) {
        short result = 0;
        if(mask & CONTENTS_SOLID) {
            result |= COLLISION_SOLID;
        }
        if(mask & CONTENTS_BODY) {
            result |= COLLISION_BODY;
        }
        return result;
    }

    struct ClosestConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback {
        int shapePart;
        int triangleIndex;

        ClosestConvexResultCallback(const btVector3& from, const btVector3& to) :
            btCollisionWorld::ClosestConvexResultCallback(from, to),
            shapePart(-1),
            triangleIndex(-1) {}

        virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) {
            float result = btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
            if(convexResult.m_localShapeInfo) {
                shapePart = convexResult.m_localShapeInfo->m_shapePart;
                triangleIndex = convexResult.m_localShapeInfo->m_triangleIndex;
            } else {
                shapePart = -1;
                triangleIndex = -1;
            }
            return result;
        }
    };
}

#endif //PHYSIC_PHYSICPRIVATE_H_

