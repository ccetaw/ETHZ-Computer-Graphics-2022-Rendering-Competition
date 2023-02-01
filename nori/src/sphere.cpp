/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Romain Prévost

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

#include <nori/shape.h>
#include <nori/bsdf.h>
#include <nori/emitter.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

class Sphere : public Shape {
public:
    Sphere(const PropertyList & propList) {
        m_position = propList.getPoint3("center", Point3f());
        m_radius = propList.getFloat("radius", 1.f);

        m_bbox.expandBy(m_position - Vector3f(m_radius));
        m_bbox.expandBy(m_position + Vector3f(m_radius));
    }

    virtual BoundingBox3f getBoundingBox(uint32_t index) const override { return m_bbox; }

    virtual Point3f getCentroid(uint32_t index) const override { return m_position; }

    virtual bool rayIntersect(uint32_t index, const Ray3f &ray, float &u, float &v, float &t) const override {
	/* to be implemented */
        auto oc = ray.o - m_position;
        auto a = ray.d.squaredNorm();
        auto half_b = oc.dot(ray.d);
        auto c = oc.squaredNorm() - m_radius*m_radius;
        auto discriminant = half_b * half_b - a*c;
        if (discriminant<0)
            return false;
        auto sqrtd = sqrt(discriminant);
        // Find the nearest root that lies in the acceptable range
        auto root = (-half_b - sqrtd) / a;
        if (root < ray.mint || ray.maxt < root)
        {
            root = (-half_b + sqrtd) / a;
            if (root < ray.mint || ray.maxt < root)
                return false;
        }
        t = root;
        return true;
    }

    virtual void setHitInformation(uint32_t index, const Ray3f &ray, Intersection & its) const override {
        its.p = ray(its.t);
        its.geoFrame = Frame((its.p - m_position).normalized());
        its.shFrame = Frame((its.p - m_position).normalized());
        its.uv = sphericalCoordinates((its.p - m_position).normalized());
        its.uv(1) /= M_PI;
        its.uv(0) /= (M_PI * 2.0);
        its.mesh = this;
    }

    virtual void sampleSurface(ShapeQueryRecord & sRec, const Point2f & sample) const override {
        Vector3f q = Warp::squareToUniformSphere(sample);
        sRec.p = m_position + m_radius * q;
        sRec.n = q;
        sRec.pdf = std::pow(1.f/m_radius,2) * Warp::squareToUniformSpherePdf(Vector3f(0.0f,0.0f,1.0f));
    }
    virtual float pdfSurface(const ShapeQueryRecord & sRec) const override {
        return std::pow(1.f/m_radius,2) * Warp::squareToUniformSpherePdf(Vector3f(0.0f,0.0f,1.0f));
    }


    virtual std::string toString() const override {
        return tfm::format(
                "Sphere[\n"
                "  center = %s,\n"
                "  radius = %f,\n"
                "  bsdf = %s,\n"
                "  emitter = %s,\n"
                "  inside = %s,\n"
                "  outside = %s,\n"
                "]",
                m_position.toString(),
                m_radius,
                m_bsdf ? indent(m_bsdf->toString()) : std::string("null"),
                m_emitter ? indent(m_emitter->toString()) : std::string("null"),
                m_mediumInterface.inside ? indent(m_mediumInterface.inside->toString()) : std::string("null"),
                m_mediumInterface.outside ? indent(m_mediumInterface.outside->toString()) : std::string("null")
        );
    }

protected:
    Point3f m_position;
    float m_radius;
};

NORI_REGISTER_CLASS(Sphere, "sphere");
NORI_NAMESPACE_END
