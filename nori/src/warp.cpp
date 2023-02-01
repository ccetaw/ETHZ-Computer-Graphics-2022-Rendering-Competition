/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/warp.h>
#include <nori/vector.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

Vector3f Warp::sampleUniformHemisphere(Sampler *sampler, const Normal3f &pole) {
    // Naive implementation using rejection sampling
    Vector3f v;
    do {
        v.x() = 1.f - 2.f * sampler->next1D();
        v.y() = 1.f - 2.f * sampler->next1D();
        v.z() = 1.f - 2.f * sampler->next1D();
    } while (v.squaredNorm() > 1.f);

    if (v.dot(pole) < 0.f)
        v = -v;
    v /= v.norm();

    return v;
}

Point2f Warp::squareToUniformSquare(const Point2f &sample) {
    return sample;
}

float Warp::squareToUniformSquarePdf(const Point2f &sample) {
    return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;
}

Point2f Warp::squareToUniformDisk(const Point2f &sample) {
    float r = sqrt(sample(0));
    float theta = 2*M_PI*sample(1);
    return Point2f(r*cos(theta), r*sin(theta));
}

float Warp::squareToUniformDiskPdf(const Point2f &p) {
    return p.norm() < 1.f ? INV_PI : 0.f;
}

Vector3f Warp::squareToUniformCylinder(const Point2f &sample, float z1, float z2){
    float z = (z2-z1) * sample(0) + z1;
    float phi = 2 * M_PI * sample(1);
    return Vector3f(cos(phi), sin(phi), z);
}

float Warp::squareToUniformCylinderPdf(const Vector3f &p, float z1, float z2){
    if ((p.norm() < 1+Epsilon) && (p.norm() > 1-Epsilon) &&  (p(2) < z2 ) && (p(2) > z1))
    return 0.5/(z2-z1) * INV_PI;
}

Vector3f Warp::squareToUniformSphereCap(const Point2f &sample, float cosThetaMax) {
    Vector3f sample_cylinder = Warp::squareToUniformCylinder(sample, cosThetaMax, 1.f);
    float z = sample_cylinder(2);
    float r = sqrt(1-z*z);
    float x = r * sample_cylinder(0);
    float y = r * sample_cylinder(1);
    return Vector3f(x,y,z);
}

float Warp::squareToUniformSphereCapPdf(const Vector3f &v, float cosThetaMax) {
    if (v.norm() < 1+Epsilon && v.norm() > 1-Epsilon && v(2) > cosThetaMax)
        return 0.5 / (1-cosThetaMax) * INV_PI;
    else
        return 0;
}

Vector3f Warp::squareToUniformSphere(const Point2f &sample) {
    Vector3f sample_cylinder = Warp::squareToUniformCylinder(sample, -1.f, 1.f);
    float z = sample_cylinder(2);
    float r = sqrt(1-z*z);
    float x = r * sample_cylinder(0);
    float y = r * sample_cylinder(1);
    return Vector3f(x,y,z);
}

float Warp::squareToUniformSpherePdf(const Vector3f &v) {
    if (v.norm() < 1+Epsilon && v.norm() > 1-Epsilon)
        return 0.25 * INV_PI;
    else
        return 0;
}

Vector3f Warp::squareToUniformHemisphere(const Point2f &sample) {
    Vector3f sample_cylinder = Warp::squareToUniformCylinder(sample, 0, 1.f);
    float z = sample_cylinder(2);
    float r = sqrt(1-z*z);
    float x = r * sample_cylinder(0);
    float y = r * sample_cylinder(1);
    return Vector3f(x,y,z);
}

float Warp::squareToUniformHemispherePdf(const Vector3f &v) {
    if (v.norm() < 1+Epsilon && v.norm() > 1-Epsilon && v(2)>0)
        return 0.5 * INV_PI;
    else
        return 0;
}

Vector3f Warp::squareToCosineHemisphere(const Point2f &sample) {
    // Cast from uniform disk to the sphere
    Point2f sample_disk = Warp::squareToUniformDisk(sample);
    float z = sqrt(1-sample_disk.squaredNorm());

    return Vector3f(sample_disk(0), sample_disk(1), z);
}

float Warp::squareToCosineHemispherePdf(const Vector3f &v) {
    if (v.norm() < 1+Epsilon && v.norm() > 1-Epsilon && v(2)>0)
        return v(2) * INV_PI;
    else
        return 0;
}

Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha) {
    float phi = 2 * M_PI * sample(0);
    float theta = atan(sqrt(-(alpha*alpha) * log(1-sample(1))));
    return Vector3f(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
}

float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha) {
    if (m.norm() < 1+Epsilon && m.norm() > 1-Epsilon && m(2)>0)
    {
        float x = m(2);
        float tan_theta2 = (1.f - x*x) / (x*x);
        return exp(-tan_theta2/(alpha*alpha)) * INV_PI / (alpha*alpha) / (x*x*x);
    }
    else
        return 0;
}

Vector3f Warp::squareToUniformTriangle(const Point2f &sample) {
    float su1 = sqrtf(sample.x());
    float u = 1.f - su1, v = sample.y() * su1;
    return Vector3f(u,v,1.f-u-v);
}

NORI_NAMESPACE_END
