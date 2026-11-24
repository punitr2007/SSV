#include "math_types.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace SSV::Math;

void TestMathVectors() {
    std::cout << "[TEST] Running Math: Vector Operations..." << std::endl;

    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);

    Vector3 sum = a + b;
    assert(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f);

    float dot = a.Dot(b);
    assert(dot == (1.0f * 4.0f + 2.0f * 5.0f + 3.0f * 6.0f)); // 32.0f

    Vector3 cross = a.Cross(b);
    // cross = (2*6 - 3*5, 3*4 - 1*6, 1*5 - 2*4) = (-3, 6, -3)
    assert(cross.x == -3.0f && cross.y == 6.0f && cross.z == -3.0f);

    Vector3 c(3.0f, 4.0f, 0.0f);
    assert(c.Length() == 5.0f);
    Vector3 cNorm = c.Normalized();
    assert(std::abs(cNorm.Length() - 1.0f) < 1e-5f);

    std::cout << "  -> Vector operations passed." << std::endl;
}

void TestMathMatrices() {
    std::cout << "[TEST] Running Math: Matrix Transformations..." << std::endl;

    Matrix4x4 ident = Matrix4x4::Identity();
    Vector3 p(10.0f, 20.0f, 30.0f);
    Vector3 pTrans = ident.TransformPoint(p);
    assert(pTrans.x == 10.0f && pTrans.y == 20.0f && pTrans.z == 30.0f);

    Matrix4x4 trans = Matrix4x4::Translation(Vector3(5.0f, -2.0f, 10.0f));
    Vector3 pMoved = trans.TransformPoint(p);
    assert(pMoved.x == 15.0f && pMoved.y == 18.0f && pMoved.z == 40.0f);

    Matrix4x4 scale = Matrix4x4::Scaling(Vector3(2.0f, 0.5f, 3.0f));
    Vector3 pScaled = scale.TransformPoint(p);
    assert(pScaled.x == 20.0f && pScaled.y == 10.0f && pScaled.z == 90.0f);

    // Matrix multiplication: Scale first, then Translate in row-vector convention (v * S * T)
    Matrix4x4 combo = scale * trans;
    Vector3 pCombo = combo.TransformPoint(Vector3(0, 0, 0));
    // (0,0,0) scaled is (0,0,0), then translated by (5, -2, 10)
    assert(std::abs(pCombo.x - 5.0f) < 1e-4f && std::abs(pCombo.y - (-2.0f)) < 1e-4f && std::abs(pCombo.z - 10.0f) < 1e-4f);

    // LookAtLH
    Matrix4x4 view = Matrix4x4::LookAtLH(Vector3(0, 0, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
    Vector3 targetView = view.TransformPoint(Vector3(0, 0, 0));
    assert(std::abs(targetView.x) < 1e-4f && std::abs(targetView.y) < 1e-4f && std::abs(targetView.z - 10.0f) < 1e-4f);

    std::cout << "  -> Matrix transformations passed." << std::endl;
}

void TestMathIntersection() {
    std::cout << "[TEST] Running Math: Ray & AABB Intersections..." << std::endl;

    AABB box(Vector3(-5, -5, -5), Vector3(5, 5, 5));
    assert(box.Contains(Vector3(0, 0, 0)));
    assert(!box.Contains(Vector3(6, 0, 0)));

    Ray hitRay(Vector3(0, 0, -20), Vector3(0, 0, 1));
    float tHit = 0.0f;
    assert(box.IntersectsRay(hitRay, tHit));
    assert(std::abs(tHit - 15.0f) < 1e-4f); // Distance from -20 to -5 is 15

    Ray missRay(Vector3(20, 20, -20), Vector3(0, 0, 1));
    float tMiss = 0.0f;
    assert(!box.IntersectsRay(missRay, tMiss));

    std::cout << "  -> Ray-AABB intersection passed." << std::endl;
}

void TestMathQuaternion() {
    std::cout << "[TEST] Running Math: Quaternion Rotations..." << std::endl;

    Quaternion qRotY = Quaternion::FromAxisAngle(Vector3(0, 1, 0), 90.0f * DEG_TO_RAD);
    Vector3 fwd(0, 0, 1);
    Vector3 rotated = qRotY.Rotate(fwd);
    // 90 deg rotation around Y turns forward (0,0,1) into right (1,0,0) in left-handed coordinates
    assert(std::abs(rotated.x - 1.0f) < 1e-4f);
    assert(std::abs(rotated.y) < 1e-4f);
    assert(std::abs(rotated.z) < 1e-4f);

    std::cout << "  -> Quaternion rotations passed." << std::endl;
}

void RunAllMathTests() {
    TestMathVectors();
    TestMathMatrices();
    TestMathIntersection();
    TestMathQuaternion();
}
