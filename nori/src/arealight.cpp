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

#include <nori/emitter.h>
#include <nori/warp.h>
#include <nori/shape.h>

NORI_NAMESPACE_BEGIN

class AreaEmitter : public Emitter {
public:
    AreaEmitter(const PropertyList &props) {
        m_radiance = props.getColor("radiance");
    }

    virtual std::string toString() const override {
        return tfm::format(
                "AreaLight[\n"
                "  radiance = %s,\n"
                "  medium = %s,\n"
                "]",
                m_radiance.toString(),
                m_medium ? indent(m_medium->toString()) : std::string("null")
                );
    }

    virtual Color3f eval(const EmitterQueryRecord & lRec) const override {
        if(!m_shape)
            throw NoriException("There is no shape attached to this Area light!");

        // return m_radiance;
        if (lRec.wi.dot(lRec.n) < 0)
            return m_radiance;
        else 
            return Color3f(0.f);
    }

    virtual Color3f sample(EmitterQueryRecord & lRec, const Point2f & sample) const override {
        if(!m_shape)
            throw NoriException("There is no shape attached to this Area light!");

        ShapeQueryRecord sRec(lRec.ref);
        m_shape->sampleSurface(sRec, sample);
        lRec.p = sRec.p;
        lRec.n = sRec.n;
        lRec.wi = (lRec.p - lRec.ref).normalized();
        lRec.shadowRay = Ray3f(lRec.ref, lRec.wi, Epsilon, (lRec.ref - lRec.p).norm()-Epsilon, this->getMedium());
        lRec.pdf = sRec.pdf;

        // return eval(lRec) / pdf(lRec);
        if (pdf(lRec) > 1e-9)
            return eval(lRec) / pdf(lRec);
        else 
            return Color3f(0.f);
    }

    virtual float pdf(const EmitterQueryRecord &lRec) const override {
        if(!m_shape)
            throw NoriException("There is no shape attached to this Area light!");

        ShapeQueryRecord sRec(lRec.ref);
        sRec.p = lRec.p;
        float density_A = m_shape->pdfSurface(sRec); // Area density
        float density_O = (lRec.p-lRec.ref).squaredNorm() / fabsf(lRec.wi.dot(lRec.n)+1e-9) * density_A; // +1e-9  in case of zero-divided
        return density_O;
    }


    virtual Color3f samplePhoton(Ray3f &ray, const Point2f &sample1, const Point2f &sample2) const override {
        if(!m_shape)
            throw NoriException("There is no shape attached to this Area light!");

        // Sample a point on the surface
        ShapeQueryRecord sRec;
        m_shape->sampleSurface(sRec, sample1);
        Point3f o = sRec.p;

        // Sample a direction
        Vector3f dir = Warp::squareToCosineHemisphere(sample2);
        Frame F(sRec.n);
        dir = F.toWorld(dir);

        ray = Ray3f(o, dir, Epsilon, INFINITY);

        return M_PI * 1.f/m_shape->pdfSurface(sRec) * m_radiance;
    }


protected:
    Color3f m_radiance;
};

NORI_REGISTER_CLASS(AreaEmitter, "area")
NORI_NAMESPACE_END