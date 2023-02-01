/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/
#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/warp.h>
#include <nori/texture.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Transluscent / Lambertian BRDF model
 */
class Transluscent : public BSDF {
public:
    Transluscent(const PropertyList &propList){ 
        m_attenuation = propList.getColor("attenuation", Color3f(1.f));
    }

    /// Evaluate the BRDF model
    virtual Color3f eval(const BSDFQueryRecord &bRec) const override {
        return Color3f(0.0f);
    }

    /// Compute the density of \ref sample() wrt. solid angles
    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        return 0.f;
    }

    /// Draw a a sample from the BRDF model
    virtual Color3f sample(BSDFQueryRecord &bRec, const Point2f &sample) const override {
        bRec.measure = EDiscrete;

        // As if no surface present
        bRec.wo = -bRec.wi;

        /* Relative index of refraction: no change */
        bRec.eta = 1.0f;

        return m_attenuation;
    }

    virtual bool isTransluscent() const override { return true; }

    /// Return a human-readable summary
    virtual std::string toString() const override {
        return tfm::format(
            "Transluscent[\n"
            "]"
        );
    }

    virtual EClassType getClassType() const override { return EBSDF; }

private:
    Color3f m_attenuation;

};

NORI_REGISTER_CLASS(Transluscent, "transluscent");
NORI_NAMESPACE_END
