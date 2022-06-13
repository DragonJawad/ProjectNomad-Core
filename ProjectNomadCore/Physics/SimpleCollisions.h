#pragma once

#include "Math/FixedPoint.h"
#include "Math/FPMath.h"
#include "Math/FPVector.h"
#include "CollisionHelpers.h"
#include "Line.h"
#include "Ray.h"
#include "Collider.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class SimpleCollisions {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        LoggerType& mLogger;
    
    public:
        SimpleCollisions(LoggerType& logger) : mLogger(logger) {} 
        
#pragma region isColliding: Collider vs Collider collision checks

        bool isColliding(const Collider& A, const Collider& B) {
            if (A.isNotInitialized()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isColliding",
                    "Collider A was not initialized type"
                );
                return false;
            }
            if (B.isNotInitialized()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isColliding",
                    "Collider B was not initialized type"
                );
                return false;
            }

            if (A.isBox()) {
                if (B.isBox()) {
                    return isBoxAndBoxColliding(A, B);
                }
                if (B.isCapsule()) {
                    return isBoxAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isBoxAndSphereColliding(A, B);
                }
            }
            if (A.isCapsule()) {
                if (B.isBox()) {
                    return isCapsuleAndBoxColliding(A, B);
                }
                if (B.isCapsule()) {
                    return isCapsuleAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isCapsuleAndSphereColliding(A, B);
                }
            }
            if (A.isSphere()) {
                if (B.isBox()) {
                    return isSphereAndBoxColliding(A, B);
                }
                if (B.isCapsule()) {
                    return isSphereAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isSphereAndSphereColliding(A, B);
                }
            }

            mLogger.logErrorMessage(
                "CollisionsSimple::isColliding",
                "Did not find a matching function for colliders A and B of types: "
                            + A.getTypeAsString() + ", " + B.getTypeAsString() 
            );
            return false;   
        }

        bool isBoxAndBoxColliding(const Collider& boxA, const Collider& boxB) {
            if (!boxA.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndBoxColliding",
                    "Collider A was not a box but instead a " + boxA.getTypeAsString()
                );
                return false;
            }
            if (!boxB.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndBoxColliding",
                    "Collider B was not a box but instead a " + boxA.getTypeAsString()
                );
                return false;
            }

            /* SAT collision test
            Based on https://gamedev.stackexchange.com/questions/44500/how-many-and-which-axes-to-use-for-3d-obb-collision-with-sat/
            1. Calculate normals from rotation
            2. Calculate all corners from rotation
            3. Do SAT. Test 15 axes. If found a separating axis, can quit early as we know there's separation
              - Test each box normal (a0, a1, a2, b0, b1, b2)
              - cross(a0, b0)
              - cross(a0, b1)
              - cross(a0, b2)
              - cross(a1, b0)
              - cross(a1, b1)
              - cross(a1, b2)
              - cross(a2, b0)
              - cross(a2, b1)
              - cross(a2, b2)
            4. If intersecting on all axes, then finally return true as there's definitely collision
            */
            // TODO: Use the Real-Time Collision Detection algorithm, which is far more efficient but still a bit over my head

            // Create necessary data used for SAT tests
            std::vector<FPVector> aNormals = boxA.getBoxNormalsInWorldCoordinates();
            std::vector<FPVector> bNormals = boxB.getBoxNormalsInWorldCoordinates();
            std::vector<FPVector> aVertices = boxA.getBoxVerticesInWorldCoordinates();
            std::vector<FPVector> bVertices = boxB.getBoxVerticesInWorldCoordinates();

            // Estimate penetration depth by keeping track of shortest penetration axis
            fp smallestPenDepth = fp{-1};
            FPVector penDepthAxis;

            // Check for separating axis with a and b normals
            for (const FPVector& aNormal : aNormals) {
                if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, aNormal, smallestPenDepth,
                                                                  penDepthAxis)) {
                    return false;
                }
            }
            for (const FPVector& bNormal : bNormals) {
                if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, bNormal, smallestPenDepth,
                                                                  penDepthAxis)) {
                    return false;
                }
            }

            // Now check for axes based on cross products with each combination of normals
            // FUTURE: Probs could do the deal-with-square-at-end trick instead of full normalizations
            FPVector testAxis;
            testAxis = aNormals.at(0).cross(bNormals.at(0)).normalized(); // Normalize to fix pen depth calculations
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(0).cross(bNormals.at(1)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(0).cross(bNormals.at(2)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(1).cross(bNormals.at(0)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(1).cross(bNormals.at(1)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(1).cross(bNormals.at(2)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(2).cross(bNormals.at(0)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(2).cross(bNormals.at(1)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            testAxis = aNormals.at(2).cross(bNormals.at(2)).normalized();
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }

            // We could find no separating axis, so definitely intersecting
            return true;
        }

        bool isCapsuleAndCapsuleColliding(const Collider& capA, const Collider& capB) {
            if (!capA.isCapsule()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndCapsuleColliding",
                    "Collider A was not a capsule but instead a " + capA.getTypeAsString()
                );
                return false;
            }
            if (!capB.isCapsule()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndCapsuleColliding",
                    "Collider B was not a capsule but instead a " + capB.getTypeAsString()
                );
                return false;
            }

            // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestCapsuleCapsule" method. Doc should be similar to other methods
            // TODO: Also rename vars

            // Get median line of capsules as points
            auto aLinePoints = capA.getCapsuleMedialLineExtremes();
            auto bLinePoints = capB.getCapsuleMedialLineExtremes();
            
            // Compute distance between the median line segments
            // "Compute (squared) distance between the inner structures of the capsules" (from book)
            fp s, t;
            FPVector c1, c2;
            fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
                aLinePoints.start, aLinePoints.end,
                bLinePoints.start, bLinePoints.end,
                s, t, c1, c2
            );

            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp radius = capA.getCapsuleRadius() + capB.getCapsuleRadius();
            bool isColliding = distSquared < radius * radius;

            return isColliding;
        }

        bool isSphereAndSphereColliding(const Collider& sphereA, const Collider& sphereB) {
            if (!sphereA.isSphere()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isSphereAndSphereColliding",
                    "Collider A was not a sphere but instead a " + sphereA.getTypeAsString()
                );
                return false;
            }
            if (!sphereB.isSphere()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isSphereAndSphereColliding",
                    "Collider B was not a sphere but instead a " + sphereB.getTypeAsString()
                );
                return false;
            }

            // Solid explanation of core algorithm with pictures here:
            // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres

            FPVector centerDifference = sphereB.getCenter() - sphereA.getCenter();
            fp centerDistance = centerDifference.getLength();
            fp intersectionDepth = (sphereA.getSphereRadius() + sphereB.getSphereRadius()) - centerDistance;

            return intersectionDepth > fp{0};
        }

        bool isBoxAndCapsuleColliding(const Collider& box, const Collider& capsule) {
            if (!box.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndCapsuleColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return false;
            }
            if (!capsule.isCapsule()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndCapsuleColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return false;
            }

            // Base logic is from Real-Time Collisions 5.5.7

            // Compute line points for capsule
            auto worldSpaceCapsulePoints = capsule.getCapsuleMedialLineExtremes();
            // Convert line points from world space to local space so can continue with Capsule vs AABB approach
            FPVector boxSpaceCapsulePointA = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.start);
            FPVector boxSpaceCapsulePointB = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.end);
            Line boxSpaceCapsuleMedialSegment(boxSpaceCapsulePointA, boxSpaceCapsulePointB);

            // Compute the AABB resulting from expanding box faces by capsule radius
            Collider checkAgainstBox(box);
            checkAgainstBox.setCenter(FPVector::zero());  // Doing everything in box local space, so center should be at origin
            checkAgainstBox.setRotation(FPQuat::identity()); // Unnecessary but like being clear that this is an AABB (no rotation)
            checkAgainstBox.setBoxHalfSize(box.getBoxHalfSize() + FPVector(capsule.getCapsuleRadius()));

            // Intersect ray against expanded box. Exit with no intersection if ray misses box, else get intersection point and time as result
            //          Side note: Pretty certain didn't need to convert to box space for this test (since raycast does that)
            //          Future minor optimization: Do this raycast test *before* converting to box local space
            //          HOWEVER, may not work due to following math (and if not possible, then update these comments so don't go down this path again)
            fp timeOfIntersection;
            FPVector intersectionPoint;
            Ray intersectionTestRay = Ray::fromPoints(boxSpaceCapsulePointA, boxSpaceCapsulePointB);
            bool didRaycastIntersectCheckBox = raycastWithBox(intersectionTestRay, checkAgainstBox,
                                                    timeOfIntersection, intersectionPoint);
            // If raycast did not hit at all then definitely no collision
            if (!didRaycastIntersectCheckBox) {
                return false;
            }

            // Check that raycast hit box in a timely manner but FIRST check for a relevant edge case 
            // Edge case: If capsule endpoints are entirely inside box then normal approach won't work. Thus explicitly
            //              check one of the endpoints to cover this case
            // Note: This is only necessary compared to Real-Time Collision approach as raycast implementation doesn't
            //              check if point is already within box
            // Note: Excluding on surface as design decision for surface touches not to count as collisions (eg, due to collision resolution not meaning much then)
            if (!checkAgainstBox.isLocalSpacePtWithinBoxExcludingOnSurface(boxSpaceCapsulePointA)) {
                // Given start of raycast was NOT in box, then raycast should have intersected with surface of box
                // before the distance of the capsule medial line (ie, we're converting this to a linetest)
                if (timeOfIntersection >= capsule.getMedialHalfLineLength() * 2) {
                    return false;
                }
            }
            
            // Compute which min and max faces of box the intersection point lies outside of
            // Note, u and v cannot have the same bits set and they must have at least one bit set among them
            uint32_t lessThanMinExtentChecks = 0, greaterThanMaxExtentChecks = 0;
            FPVector minBoxExtents = -box.getBoxHalfSize();
            FPVector maxBoxExtents = box.getBoxHalfSize();
            if (intersectionPoint.x < minBoxExtents.x) lessThanMinExtentChecks |= 1;
            else if (intersectionPoint.x > maxBoxExtents.x) greaterThanMaxExtentChecks |= 1;
            if (intersectionPoint.y < minBoxExtents.y) lessThanMinExtentChecks |= 2;
            else if (intersectionPoint.y > maxBoxExtents.y) greaterThanMaxExtentChecks |= 2;
            if (intersectionPoint.z < minBoxExtents.z) lessThanMinExtentChecks |= 4;
            else if (intersectionPoint.z > maxBoxExtents.z) greaterThanMaxExtentChecks |= 4;
            
            // "Or" all set bits together into a bit mask (note: here u + v == u | v)
            uint32_t mask = lessThanMinExtentChecks + greaterThanMaxExtentChecks;

            // If all 3 bits set (m == 7) then intersection point is in a vertex region
            if (mask == 7) {
                bool didIntersect;
                FPVector unused;
                
                // Must now intersect capsule line segment against the capsules of the three
                // edges meeting at the vertex and return the best time, if one or more hit
                fp tMin = FPMath::maxLimit();

                // Note that endpoint of test line will be changed for each test
                Line testCapsuleMedianLine(getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks), unused);

                testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 1);
                didIntersect = linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                                capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) {
                    tMin = FPMath::min(timeOfIntersection, tMin);
                }

                testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 2);
                didIntersect = linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                            capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) {
                    tMin = FPMath::min(timeOfIntersection, tMin);
                }

                testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 4);
                didIntersect = linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                            capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) {
                    tMin = FPMath::min(timeOfIntersection, tMin);
                }

                // If didn't find a single intersection, then confirmed that there was no intersection
                if (tMin == FPMath::maxLimit()) {
                    return false;
                }
                
                timeOfIntersection = tMin;
                return true; // Intersection at time t == tmin
            }
            
            // If only one bit set in m, then intersection point is in a face region
            if ((mask & (mask - 1)) == 0) {
                // Do nothing. Time t from intersection with expanded box is correct intersection time
                return true;
            }
            
            // p is in an edge region. Intersect against the capsule at the edge
            Line testCapsuleMedianLine(
                getCorner(minBoxExtents, maxBoxExtents, lessThanMinExtentChecks ^ 7),
                getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks)
            );
            bool didIntersect = linetestWithCapsule(
                boxSpaceCapsuleMedialSegment,
                testCapsuleMedianLine,
                capsule.getCapsuleRadius(),
                timeOfIntersection,
                intersectionPoint
            );
            
            return didIntersect;
        }

        bool isBoxAndSphereColliding(const Collider& box, const Collider& sphere) {
            if (!box.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndSphereColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return false;
            }
            if (!sphere.isSphere()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isBoxAndSphereColliding",
                    "Collider sphere was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return false;
            }

            // Algorithm references:
            // AABB vs Sphere https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres
            // OOBB vs Sphere https://gamedev.stackexchange.com/a/163874/67844

            // Convert sphere's center to OBB's local space, with OOB's center as origin
            // This allows us to use simplified Sphere vs AABB logic here on out
            FPVector localSphereCenter = box.toLocalSpaceFromWorld(sphere.getCenter());

            // Find the closest point on the box to the sphere's center
            // Essentially just uses sphere center coordinate on each axis IF within box, otherwise clamps to nearest box extent
            // Suggest visualizing with number lines
            const FPVector& extents = box.getBoxHalfSize(); // Represents min and max points, since using box center as origin
            FPVector closestBoxPointToSphere;
            closestBoxPointToSphere.x = FPMath::max(-extents.x, FPMath::min(localSphereCenter.x, extents.x));
            closestBoxPointToSphere.y = FPMath::max(-extents.y, FPMath::min(localSphereCenter.y, extents.y));
            closestBoxPointToSphere.z = FPMath::max(-extents.z, FPMath::min(localSphereCenter.z, extents.z));

            // Finally do ordinary point distance vs sphere radius checking
            FPVector closestPointOffSetToSphere = localSphereCenter - closestBoxPointToSphere;
            fp sphereCenterToBoxDistance = closestPointOffSetToSphere.getLength();
            if (sphereCenterToBoxDistance == fp{0}) {
                // Sphere center within box case
                return true;
            }

            fp intersectionDepth = sphere.getSphereRadius() - sphereCenterToBoxDistance;
            return intersectionDepth > fp{0};
        }

        bool isCapsuleAndSphereColliding(const Collider& capsule, const Collider& sphere) {
            if (!capsule.isCapsule()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndSphereColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return false;
            }
            if (!sphere.isSphere()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndSphereColliding",
                    "Collider sphere was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return false;
            }

            // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestSphereCapsule" method. Doc should be similar to other methods
            // TODO: Also rename vars

            // Get median line of capsule as points
            auto capsulePoints = capsule.getCapsuleMedialLineExtremes();

            // Compute (squared) distance between sphere center and capsule line segment
            fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(capsulePoints, sphere.getCenter());
            
            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp radius = sphere.getSphereRadius() + capsule.getCapsuleRadius();
            bool isColliding = distSquared < radius * radius;
            
            return isColliding;
        }

        // Extras below for ease of typing. Mostly due to ease of scanning for consistency in isColliding
        bool isCapsuleAndBoxColliding(const Collider& capsule, const Collider& box) {
            return isBoxAndCapsuleColliding(box, capsule);
        }
        bool isSphereAndBoxColliding(const Collider& sphere, const Collider& box) {
            return isBoxAndSphereColliding(box, sphere);
        }
        bool isSphereAndCapsuleColliding(const Collider& sphere, const Collider& capsule) {
            return isCapsuleAndSphereColliding(capsule, sphere);
        }
        
#pragma endregion

#pragma region Raycast + Linetest

        /// <summary>
        /// Check if and when a ray and sphere intersect
        /// Based on Game Physics Cookbook, Ch 7
        /// </summary>
        /// <param name="ray">Ray to test with</param>
        /// <param name="sphere">Sphere to test with</param>
        /// <param name="timeOfIntersection">If intersection occurs, this will be time which ray first intersects with sphere</param>
        /// <param name="pointOfIntersection">Location where intersection occurs</param>
        /// <returns>True if intersection occurs, false otherwise</returns>
        bool raycastWithSphere(const Ray& ray, const Collider& sphere, fp& timeOfIntersection, FPVector& pointOfIntersection) {
            if (!sphere.isSphere()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::raycast",
                    "Provided collider was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return false;
            }
            
            // TODO: Re-read book's "How it works" section, add more personal notes, and rename vars
            // Lotta segments and need picture and such... Ahhh....

            FPVector originToSphereCenter = sphere.getCenter() - ray.origin;
            fp distBetweenCentersSq = originToSphereCenter.getLengthSquared();
            fp radiusSq = sphere.getSphereRadius() * sphere.getSphereRadius();

            // Project the vector pointing from the origin of the ray to the sphere onto the direction 
            //  of the ray
            // Note: Ray direction should be normalized
            fp a = originToSphereCenter.dot(ray.direction); // TOOD: Rename var

            // Construct the sides a triangle using the radius of the circle at the projected point 
            //  from the last step.The sides of this triangle are radius, b, and f. We work with
            //  squared units
            fp bSq = distBetweenCentersSq - (a * a); // TODO: Rename vars
            fp f = sqrt(radiusSq - bSq);

            // Compare the length of the squared radius against the hypotenuse of the triangle from 
            //  the last step

            // Case: No collision has happened
            if (radiusSq - (distBetweenCentersSq - (a * a)) < fp{0}) {
                return false; // -1 = no intersection
            }
            // Case: Ray starts inside the sphere
            if (distBetweenCentersSq < radiusSq) {
                timeOfIntersection = a + f;
                pointOfIntersection = ray.origin + ray.direction * timeOfIntersection;
                return true; // No need to check if time of intersection is positive or not as ray started inside sphere
            }
            // Otherwise normal intersection
            timeOfIntersection = a - f;
            pointOfIntersection = ray.origin + ray.direction * timeOfIntersection;
            return timeOfIntersection >= fp{0}; // If negative, then sphere was behind ray
        }
        
        /// <summary>
        /// Checks if and when a ray and OBB intersect
        /// </summary>
        /// <param name="ray">Starting point and direction to check for intersection with</param>
        /// <param name="box">Box to check for intersection with</param>
        /// <param name="timeOfIntersection">
        /// If intersection occurs, this will be time which ray first intersects with OBB.
        /// Note that if ray origin is within OBB, then this will signify when the ray hits the OBB on the way out.
        /// </param>
        /// <param name="pointOfIntersection">Location where intersection occurs</param>
        bool raycastWithBox(const Ray& ray,
                            const Collider& box,
                            fp& timeOfIntersection,
                            FPVector& pointOfIntersection) {
            if (!box.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::raycast",
                    "Provided collider was not a box but instead a " + box.getTypeAsString()
                );
                return false;
            }
            
            // 1. Convert ray to box space
            Ray localSpaceRay(box.toLocalSpaceFromWorld(ray.origin), box.toLocalSpaceForOriginCenteredValue(ray.direction));

            // 2. Compute results via treating as standard AABB situation
            FPVector localPointOfIntersection;
            bool didIntersect = raycastForAABB(localSpaceRay, box, timeOfIntersection, localPointOfIntersection);

            // 3. Convert results to world space if necessary
            if (didIntersect) {
                pointOfIntersection = box.toWorldSpaceFromLocal(localPointOfIntersection); // Reverse of earlier operations for localSpaceRay origin
            }

            return didIntersect;
        }

        // TODO: Raycast capsule

        /// <summary>
        /// Checks if a line and OBB intersect
        /// Originally based on Game Physics Cookbook, Chapter 10
        /// </summary>
        /// <param name="line">Line to check if intersects against box</param>
        /// <param name="box">Box to test line against</param>
        /// <param name="timeOfIntersection">
        /// If intersection occurs, this will be time which line first intersects with OBB.
        /// Note that if line origin is within OBB, then this will signify when the line hits the OBB on the way out.
        /// </param>
        /// <param name="pointOfIntersection">Location where intersection occurs</param>
        /// <returns>True if line and OBB intersect, false otherwise</returns>
        bool linetestWithBox(const Line& line,
                            const Collider& box,
                            fp& timeOfIntersection,
                            FPVector& pointOfIntersection) {
            if (!box.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::raycast",
                    "Provided collider was not a box but instead a " + box.getTypeAsString()
                );
                return false;
            }
            
            // TODO: Check if using TestSegmentAABB from Real-Time Collision Detection 5.3.3 is more efficient

            // Reuse raycast logic for intersection testing
            Ray ray(
                line.start,
                (line.end - line.start).normalized()
            );
            bool didRaycastHit = raycastWithBox(ray, box, timeOfIntersection, pointOfIntersection);

            // Return results. If raycast hit, then linetest also hits if hit isn't too far
            //  Note: t vs length comparison works as ray/line travels 1 unit along ray/line per second. 
            //        Thus, can use time as a distance metric as well
            const fp& t = timeOfIntersection; // For easier reading
            return didRaycastHit && t >= fp{0} && t * t <= line.getLengthSquared();
        }

        bool linetestWithCapsule(const Line& line,
                                const Collider& capsule,
                                fp& timeOfIntersection,
                                FPVector& pointOfIntersection) {
            if (!capsule.isCapsule()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::linetest",
                    "Capsule provided was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return false;
            }

            return linetestWithCapsule(line, capsule.getCapsuleMedialLineExtremes(),
                capsule.getCapsuleRadius(), timeOfIntersection, pointOfIntersection);
        }

        /// <summary>
        /// TODO
        /// </summary>
        /// <param name="line">TODO</param>
        /// <param name="capsuleMedianLine">Median line of capsule. Note that this is NOT top and bottom, but rather "A" and "B" in relevant diagrams</param>
        /// <param name="capsuleRadius">TODO</param>
        /// <param name="timeOfIntersection">Time of intersection with respect to line. Between fp{0} and 1 when there is an intersection</param>
        /// <param name="pointOfIntersection">Point of intersection along line, if there is an intersection</param>
        /// <returns>True if intersection occurs, false otherwise</returns>
        bool linetestWithCapsule(const Line& line, const Line& capsuleMedianLine, const fp& capsuleRadius,
                                fp& timeOfIntersection, FPVector& pointOfIntersection) {

            // Reuse capsule vs capsule logic (or at least the theory behind it)
            // Comparing line vs capsule is just like comparing two capsules with one of fp{0} radius
            // IN SHORT, if line gets within capsule's radius of the capsule's center line, then we know there's an intersection

            // Find distance (squared) between the line and the median line of the capsule
            fp timeOfIntersectionForCapsuleLine;
            FPVector closestPointForCapsuleLine;
            fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
                line, capsuleMedianLine,
                timeOfIntersection, timeOfIntersectionForCapsuleLine,
                pointOfIntersection, closestPointForCapsuleLine
            );

            // If (squared) distance smaller than (squared) radius, the line and capsule collide
            return distSquared <= capsuleRadius * capsuleRadius;

            // TODO: Further calculate time of intersection and point of intersection when intersection is confirmed
        }

#pragma endregion

#pragma region Collision Helpers (ie, should be private but not cuz ComplexCollisions usage)

        bool isIntersectingAlongAxisAndUpdatePenDepthVars(
            const std::vector<FPVector>& boxAVertices, const std::vector<FPVector>& boxBVertices,
            const FPVector& testAxis,
            fp& smallestPenDepth, FPVector& penDepthAxis) {

            // Edge case when two normals are parallel. For now, just continue with other SAT tests
            // Possibly error prone, but the Real-Time Collision Detection book algorithm will handle this differently
            if (testAxis == FPVector::zero()) {
                return true;
            }

            fp currentIntersectionDist = CollisionHelpers::getIntersectionDistAlongAxis(
                boxAVertices, boxBVertices, testAxis);
            if (currentIntersectionDist <= fp{0}) {
                // If not intersecting, then return false immediately as SAT tests are over
                return false;
            }

            // Otherwise, update penetration depth variables and then return true
            if (smallestPenDepth == fp{-1} || currentIntersectionDist < smallestPenDepth) {
                smallestPenDepth = currentIntersectionDist;
                penDepthAxis = testAxis;
            }
            return true;
        }

        // Support function that returns the AABB vertex with index
        static FPVector getCorner(FPVector minBoxExtents, FPVector maxBoxExtents, uint32_t n) {
            // Based on Real-Time Collision Detection book, section 5.5.7 (method called "Corner")
            FPVector result;
            
            result.x = ((n & 1) ? maxBoxExtents.x : minBoxExtents.x);
            result.y = ((n & 1) ? maxBoxExtents.y : minBoxExtents.y);
            result.z = ((n & 1) ? maxBoxExtents.z : minBoxExtents.z);
            
            return result;
        }

#pragma endregion 
    
    private:
        /// <summary>
        /// Checks if and when a ray intersects an AABB (which is effectively local space check against OBB).
        /// Note: Based on Real-Time Collision Detection, Section 5.3.3
        /// </summary>
        /// <param name="relativeRay">
        /// Assumes in OBB box local space already (and thus using same AABB algorithm).
        /// Origin should be relative to OBB's center (ie, OBB's center is treated as center of world and then rotated).
        /// Direction should also be converted to OBB local space.
        /// </param>
        /// <param name="box">Box to test with./param>
        /// <param name="timeOfIntersection">
        /// If intersection occurs, this will be time which ray first intersects with the box.
        /// Note that if ray origin is within the box, then this will signify when the ray hits the OBB on the way out.
        /// </param>
        /// <param name="pointOfIntersection">Location where intersection occurs</param>
        /// <returns>True if intersection occurs, false otherwise.</returns>
        bool raycastForAABB(const Ray& relativeRay,
                            const Collider& box,
                            fp& timeOfIntersection,
                            FPVector& pointOfIntersection) {
            if (!box.isBox()) {
                mLogger.logErrorMessage(
                    "CollisionsSimple::raycastForAABB",
                    "Provided collider was not a box but instead a " + box.getTypeAsString()
                );
                return false;
            }
            
            fp timeOfEarliestHitSoFar = FPMath::minLimit(); // Set to negative max to get first hit/intersection
            fp timeOfLatestHitSoFar = FPMath::maxLimit(); // Set to max distance ray can travel (for segment), so get last hit on line

            FPVector boxMin = -box.getBoxHalfSize();
            FPVector boxMax = box.getBoxHalfSize();

            // Do checks for all three slabs (axis extents)...
            for (int i = 0; i < 3; i++) {
                // If ray is not moving on this axis...
                if (FPMath::isNear(FPMath::abs(relativeRay.direction[i]), fp{0}, fp{0.0001f})) {
                    // No hit if origin not within slab (axis extents) already
                    if (relativeRay.origin[i] < boxMin[i] || relativeRay.origin[i] > boxMax[i]) {
                        return false;
                    }
                }
                // Otherwise - since ray is moving on this axis - check that ray does enter the slab (axis extents)...
                else {
                    // Optimization: Denominator portion computed only once. Also note never dividing by 0 due to earlier if statement check
                    fp directionFraction = fp{1} / relativeRay.direction[i];
                    
                    // Compute intersection time value of ray with either (near and far relative to ray) plane of axis slab
                    // NOTE: Ray is defined by following equation: x = d*t + b, where d = direction on axis and b = starting value
                    //       Thus, to solve for t given a specific coordinate value, we'd do t = (x - b)/d
                    fp timeOfNearPlaneIntersection = (boxMin[i] - relativeRay.origin[i]) * directionFraction;
                    fp timeOfFarPlaneIntersection = (boxMax[i] - relativeRay.origin[i]) * directionFraction;

                    // Make sure variable name-expectations are correct for latter check
                    if (timeOfNearPlaneIntersection > timeOfFarPlaneIntersection) {
                        FPMath::swap(timeOfNearPlaneIntersection, timeOfFarPlaneIntersection);
                    }

                    // Compute the intersection of slab intersection intervals
                    // (ie, update our earliest and latest hits across all axes thus far)
                    if (timeOfNearPlaneIntersection > timeOfEarliestHitSoFar) timeOfEarliestHitSoFar = timeOfNearPlaneIntersection;
                    if (timeOfFarPlaneIntersection < timeOfLatestHitSoFar) timeOfLatestHitSoFar = timeOfFarPlaneIntersection;

                    // Exit with no collision as soon as slab intersection becomes empty
                    // (This covers both case where ray does not enter slab at all and case where ray doesn't enter all
                    //      slabs/axis extents at the same time)
                    if (timeOfEarliestHitSoFar > timeOfLatestHitSoFar) return false;
                }
            }

            // If timeOfLatestHitSoFar is less than zero, the ray is intersecting box in negative direction
            //  ie, entire AABB is behind origin of ray and thus no intersection
            // NOTE: Added equality check (<= vs <) as don't want to consider ray that only starts on surface of box to be an intersection
            //          (and checking an "epsilon'/close to 0 value instead of 0 to catch possible minor math inaccuracies)
            if (timeOfLatestHitSoFar <= fp{0.001f}) return false;

            bool doesRayStartInBox = timeOfEarliestHitSoFar < fp{0}; // If "earliest" intersection is negative then ray started in origin
            
            // Check for raycast only touching box surface
            if (!doesRayStartInBox) { // If ray inside box then has to exit box so no need to check further in that case
                // Approach:
                // Already covered cases where only touching surface of box once and - by process of elimination - we know
                //      that the ray touches the box more than once on its surface (eg, from one side to other of box).
                //      Thus need to check if intersection "segment" actually goes into the box.
                // Easy way to do so is to get the beginning and end of the "segment", and then check if both points are
                //      on different faces.
                // (Tried to do an approach with line direction and perpendicular vector with another raycast intersection
                //      test, but that seemed to be too expensive)

                // Calculate initial and final intersection locations. Note that position = direction * time + origin (eg, x = a*t + b)
                FPVector initialPointOfIntersection = relativeRay.direction * timeOfEarliestHitSoFar + relativeRay.origin;
                FPVector finalPointOfIntersection = relativeRay.direction * timeOfLatestHitSoFar + relativeRay.origin;

                // Get faces that each point touches (if any)
                std::vector<FPVector> initialPointFaces;
                std::vector<FPVector> finalPointFaces;
                box.getFacesThatLocalSpacePointTouches(initialPointOfIntersection, initialPointFaces);
                box.getFacesThatLocalSpacePointTouches(finalPointOfIntersection, finalPointFaces);

                // If intersection segment only touches surface and does not go into box, then there should be at least one
                //  face that both beginning and end of segment touch. (Easy to see after drawing rays across a box on paper for a while)
                // SIDE NOTE: Could perhaps do some sort of optimized intersection method (with or without HashSet), but
                //              this is at work 3x3 = 9 loop iterations. Don't find this worth further optimizing atm
                for (const auto& initialPointFace : initialPointFaces) {
                    for (const auto& finalPointFace : finalPointFaces) {
                        if (initialPointFace == finalPointFace) {
                            return false; // Confirmed segment only touches surface of box!
                        }
                    }
                }
            }
            
            // As confirmed ray intersects all 3 slabs (axis extents) and not only on surface, we can confirm that there
            //  really was an intersection
            timeOfIntersection = doesRayStartInBox ? timeOfLatestHitSoFar : timeOfEarliestHitSoFar;
            pointOfIntersection = relativeRay.origin + relativeRay.direction * timeOfIntersection;
            return true;
        }
    };
}
