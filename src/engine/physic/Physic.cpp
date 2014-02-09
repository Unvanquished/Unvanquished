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

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/circulator.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel CGALKernel;
typedef CGALKernel::Point_3 CGALPoint;
typedef CGAL::Polyhedron_3<CGALKernel> CGALPolyhedron;

namespace Physic {

    btCollisionDispatcher* dispatcher = nullptr;
    btDbvtBroadphase* broadphase = nullptr;
    btDefaultCollisionConfiguration* collisionConf = nullptr;
    btCollisionWorld* world = nullptr;

    void Init() {
        collisionConf = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConf);
        broadphase = new btDbvtBroadphase();
        world = new btCollisionWorld(dispatcher, broadphase, collisionConf);
    }

    void Shutdown() {
        delete world;
        world = nullptr;
        delete broadphase;
        broadphase = nullptr;
        delete dispatcher;
        dispatcher = nullptr;
        delete collisionConf;
        collisionConf = nullptr;
    }

    std::vector<btVector3> ConvexHullTrianglesFromVertices(const btAlignedObjectArray<btVector3> vertices) {
        std::vector<CGALPoint> cgalPoints;
        for (int i = 0; i < vertices.size(); i++) {
            cgalPoints.push_back(CGALPoint(vertices[i][0], vertices[i][1], vertices[i][2]));
        }

        CGALPolyhedron poly;
        CGAL::convex_hull_3(cgalPoints.begin(), cgalPoints.end(), poly);

        std::vector<btVector3> res;
        for (auto it = poly.facets_begin(); it != poly.facets_end(); it++) {
            CGAL::Container_from_circulator<CGALPolyhedron::Facet::Halfedge_around_facet_circulator> vertices(it->facet_begin());
            auto vit = vertices.begin();

            CGALPoint first = vit->vertex()->point();
            vit ++;
            CGALPoint oldPoint = vit->vertex()->point();
            vit ++;
            while(vit != vertices.end()) {
                CGALPoint nextPoint = vit->vertex()->point();

                res.push_back({first[0], first[1], first[2]});
                res.push_back({oldPoint[0], oldPoint[1], oldPoint[2]});
                res.push_back({nextPoint[0], nextPoint[1], nextPoint[2]});

                oldPoint = nextPoint;
                vit ++;
            }
        }

        return std::move(res);
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
                                std::vector<btVector3> triangleVertices = ConvexHullTrianglesFromVertices(vertices);
                                for(unsigned i = 0; i < triangleVertices.size(); i += 3) {
                                    trimesh->addTriangle(
                                        triangleVertices[i],
                                        triangleVertices[i + 1],
                                        triangleVertices[i + 2]
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
        btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(trimesh, false, true);
        btCollisionObject* triObj = new btCollisionObject();
        triObj->setCollisionShape(shape);
        world->addCollisionObject(triObj, COLLISION_SOLID);
    }

    float Trace(const vec3_t from, const vec3_t to) {
        btCollisionWorld::ClosestRayResultCallback callback(q3ToBullet(from), q3ToBullet(to));
        world->rayTest(q3ToBullet(from), q3ToBullet(to), callback);
        return callback.m_closestHitFraction;
    }

    void BoxTrace(const vec3_t pFrom, const vec3_t pTo, const vec3_t pMins, const vec3_t pMaxs, int brushMask, TraceResults& results) {
        //FIXME
        btVector3 from = q3ToBullet(pFrom);
        btVector3 to = q3ToBullet(pTo);
        btVector3 mins = q3ToBullet(pMins);
        btVector3 maxs = q3ToBullet(pMaxs);
        btVector3 halfExtends = (maxs - mins) / 2;
        btVector3 rayOffset = maxs - halfExtends;
        short mask = convertCollisionMask(brushMask);

        btBoxShape box(halfExtends);
        btTransform transFrom;
        transFrom.setIdentity();
        transFrom.setOrigin(from + rayOffset);
        btTransform transTo;
        transTo.setIdentity();
        transTo.setOrigin(to + rayOffset);

        ClosestConvexResultCallback callback(from + rayOffset, to + rayOffset);
        callback.m_collisionFilterMask = mask;
        world->convexSweepTest(&box, transFrom, transTo, callback);

        float fraction = results.fraction = callback.m_closestHitFraction;

        bulletToQ3(callback.m_hitNormalWorld, results.normal);

        btVector3 diff = to - from;
        btVector3 rawRes = from + fraction * diff;;
        float traceLength = diff.length();

        btVector3 tamperedRes = rawRes - diff.normalized() * std::min(1.0f / 16.0f, traceLength);
        bulletToQ3(tamperedRes, results.endpos);

        //if(callback.triangleIndex != 0 && callback.triangleIndex != -1) {
            //Log::Debug("%? %?\n", callback.shapePart, callback.triangleIndex);
        //}
    }

    void TempBox(const vec3_t pMins, const vec3_t pMaxs) {
        btVector3 mins = q3ToBullet(pMins);
        btVector3 maxs = q3ToBullet(pMaxs);
        btVector3 halfExtends = (maxs - mins) / 2;
        btVector3 offset = maxs - halfExtends;
        static btBoxShape boxShape(btVector3(0, 0, 0));
        static btCollisionObject boxObject;
        static btCollisionWorld* lastWorld = nullptr;
        boxShape = btBoxShape(halfExtends);
        boxObject.setCollisionShape(&boxShape);
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(offset);
        boxObject.setWorldTransform(transform);
        if(world && world != lastWorld) {
            lastWorld = world;
            world->addCollisionObject(&boxObject, COLLISION_BODY);
        }
    }
}
