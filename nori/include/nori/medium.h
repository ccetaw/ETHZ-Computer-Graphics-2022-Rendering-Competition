/*
Author: Jingyu Wang, ETHz Data Science Master student
Reference: https://www.pbr-book.org/3ed-2018/Volume_Scattering
*/
#if !defined(__NORI_MEDIUM_H)
#define __NORI_MEDIUM_H

#include <nori/object.h>
#include <nori/phasefunction.h>

NORI_NAMESPACE_BEGIN

// Data structure records ray interaction inside the medium
struct MediumInteraction
{
    // Position of interaction
    Point3f p;
    // Unoccluded distance along the ray
    float t;
    // Incident direction of the ray, following the same convention as bsdf in nori
    Vector3f wi;
    // Scattered direction of the ray
    Vector3f wo;
    // Phase fucntion for this medium
    const PhaseFunction *phase;
    // Pointer to the medium
    const Medium *medium;

    MediumInteraction() { phase = nullptr; medium = nullptr;}

    MediumInteraction(Point3f p_, float t_, Vector3f wi_):
    p(p_), t(t_), wi(wi_), phase(nullptr), medium(nullptr) { }

    MediumInteraction(Point3f p_, float t_, Vector3f wi_, const PhaseFunction *phase_, const Medium *medium_):
    p(p_), t(t_), wi(wi_), phase(phase_), medium(medium_) { }

    bool isValid() const { return phase != nullptr; }
};

class Medium: public NoriObject {
public:
    /// Calculate the transmittance give the ray
    virtual Color3f Tr(const Ray3f &ray) const = 0;

    virtual Color3f sample(const Ray3f &ray, const Point2f &sample, MediumInteraction &mRec) const = 0;

    const PhaseFunction *getPhase() const { return m_phase; }

    virtual void activate() override
    {
        if (!m_phase) {
            /* If no material was assigned, instantiate a diffuse BRDF */
            m_phase = static_cast<PhaseFunction *>(
                NoriObjectFactory::createInstance("HG", PropertyList()));
            m_phase->activate();
        }
    }

    virtual void addChild(NoriObject *obj) override
    {
        switch (obj->getClassType()) {
        case EPhaseFunction:
            if (m_phase)
                throw NoriException(
                    "Medium: tried to register multiple BSDF instances!");
            m_phase = static_cast<PhaseFunction *>(obj);
            break;

        default:
            throw NoriException("Medium::addChild(<%s>) is not supported!",
                                classTypeName(obj->getClassType()));
        }
    }

    virtual EClassType getClassType() const override { return EMedium; }

protected:
    PhaseFunction *m_phase = nullptr;
};


// Data structure records ray-MediumInterface interacion
struct MediumInterface {
    // Inside and outside medium
    Medium *inside, *outside;

    // When only one medium is provided, both sides are the same
    MediumInterface(): inside(nullptr), outside(nullptr) { }

    MediumInterface(Medium *medium)
        : inside(medium), outside(medium) { }
    
    MediumInterface(Medium *inside, Medium *outside)
        : inside(inside), outside(outside) { }

    bool IsMediumTransition() const { return inside != outside; }
};





NORI_NAMESPACE_END

#endif /* __NORI_MEDIUM_H */