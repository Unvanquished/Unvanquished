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

#include "PhysicPrivate.h"
#include "BspLoader.h"
#include <LinearMath/btGeometryUtil.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

namespace Physic {

    void Init() {
        collisionConf = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConf);
        broadphase = new btDbvtBroadphase();
        world = new btCollisionWorld(dispatcher, broadphase, collisionConf);
    }

    void Shutdown() {
        delete world;
        delete broadphase;
        delete dispatcher;
        delete collisionConf;
    }

    void Load(void* bspBuffer) {
        //TODO: don't load the map twice
        //TODO: fix memory leaks

        BspLoader loader;
        loader.loadBSPFile(bspBuffer);
        btTriangleMesh* trimesh = new btTriangleMesh();

        for(int i = 0; i < loader.m_numleafs; ++i) {
            BSPLeaf& leaf = loader.m_dleafs[i];
            for(int b = 0; b < leaf.numLeafBrushes; ++b) {
                btAlignedObjectArray<btVector3> planeEquations;
                int brushid = loader.m_dleafbrushes[leaf.firstLeafBrush+b];
                BSPBrush& brush = loader.m_dbrushes[brushid];
                if(brush.shaderNum != -1) {
                    if(loader.m_dshaders[brush.shaderNum].contentFlags & BSPCONTENTS_SOLID) {
                        brush.shaderNum = -1;
                        if(brush.numSides) {
                            for(int p = 0; p < brush.numSides; ++p) {
                                int sideid = brush.firstSide+p;
                                BSPBrushSide& brushside = loader.m_dbrushsides[sideid];
                                int planeid = brushside.planeNum;
                                BSPPlane& plane = loader.m_dplanes[planeid];
                                btVector3 planeEq;
                                planeEq.setValue(
                                    plane.normal[0],
                                    plane.normal[1],
                                    plane.normal[2]
                                );
                                planeEq[3] = -plane.dist;

                                planeEquations.push_back(planeEq);
                            }
                            btAlignedObjectArray<btVector3> vertices;
                            btGeometryUtil::getVerticesFromPlaneEquations(planeEquations,vertices);
                            if (vertices.size() > 0) {
                                btConvexHullShape* shape = new btConvexHullShape(&(vertices[0].getX()),vertices.size());
                                btShapeHull converter(shape);
                                converter.buildHull(0.f);
                                for(int i = 0; i < converter.numIndices(); i += 3) {
                                    trimesh->addTriangle(
                                        converter.getVertexPointer()[converter.getIndexPointer()[i]],
                                        converter.getVertexPointer()[converter.getIndexPointer()[i+1]],
                                        converter.getVertexPointer()[converter.getIndexPointer()[i+2]]
                                    );
                                }
                                delete shape;
                            }
                        }
                    }
                }
            }
        }
        btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(trimesh, false, true);
        btCollisionObject* triObj = new btCollisionObject();
        triObj->setCollisionShape(shape);
        world->addCollisionObject(triObj);
    }

    float Trace(const vec3_t from, const vec3_t to) {
        btCollisionWorld::ClosestRayResultCallback callback(convertVector(from), convertVector(to));
        world->rayTest(convertVector(from), convertVector(to), callback);
        return callback.m_closestHitFraction;
    }

    float BoxTrace(const vec3_t pFrom, const vec3_t pTo, const vec3_t pMins, const vec3_t pMaxs) {
        //FIXME
        btVector3 from = convertVector(pFrom);
        btVector3 to = convertVector(pTo);
        btVector3 mins = convertVector(pMins);
        btVector3 maxs = convertVector(pMaxs);
        btVector3 halfExtends = (maxs - mins) / 2;
        btVector3 rayMove = maxs - halfExtends;
        halfExtends = halfExtends.absolute();

        btBoxShape box(halfExtends);
        btTransform transFrom;
        transFrom.setIdentity();
        transFrom.setOrigin(from + rayMove);
        btTransform transTo;
        transTo.setIdentity();
        transTo.setOrigin(to + rayMove);

        btCollisionWorld::ClosestConvexResultCallback callback(from, to);
        world->convexSweepTest(&box, transFrom, transTo, callback);

        return callback.m_closestHitFraction;
    }
}
