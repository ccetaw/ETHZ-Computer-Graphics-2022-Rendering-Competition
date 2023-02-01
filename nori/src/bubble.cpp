/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/

#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/texture.h>

NORI_NAMESPACE_BEGIN

/// Ideal Bubble BSDF
class Bubble : public BSDF {
public:
    Bubble(const PropertyList &propList) {
        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f);

        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f);

        m_reflectance = propList.getColor("reflectance", Color3f(1.f));
        m_transmittance = propList.getColor("transmittance", Color3f(1.f));

    }

    virtual Color3f eval(const BSDFQueryRecord &) const override {
        /* Discrete BRDFs always evaluate to zero in Nori */
        return Color3f(0.0f);
    }

    virtual float pdf(const BSDFQueryRecord &) const override {
        /* Discrete BRDFs always evaluate to zero in Nori */
        return 0.0f;
    }

    virtual Color3f sample(BSDFQueryRecord &bRec, const Point2f &sample) const override {
        bRec.measure = EDiscrete;
        float R = fresnel(abs(Frame::cosTheta(bRec.wi)), m_extIOR, m_intIOR), T; // Fresnel coefficient
        R *= 2.f / (1.f + R);
        T = 1 - R;
        float eta, eta_inv;
        Normal3f n; // Normal at the same side as the incident camera ray is (in local coordinates)

        if (sample.x() < R) // Reflection in local coordinates
        {
            bRec.eta = 1.f; // no change, still in the same medium
            bRec.wo = Vector3f(
                -bRec.wi.x(),
                -bRec.wi.y(),
                bRec.wi.z()
            );
        
            return m_reflectance; 
        }
        else // Refraction in local coordinates
        {
            bRec.eta = 1.f;
            bRec.wo = -bRec.wi;

            return m_transmittance;
        }
    }

    virtual bool isDelta() const override{ return true; }

    virtual std::string toString() const override {
        return tfm::format(
            "Bubble[\n"
            "  intIOR = %f,\n"
            "  extIOR = %f\n"
            "]",
            m_intIOR, m_extIOR);
    }
private:
    float m_intIOR, m_extIOR;
    Color3f m_reflectance;
    Color3f m_transmittance;
};

NORI_REGISTER_CLASS(Bubble, "bubble");
NORI_NAMESPACE_END
