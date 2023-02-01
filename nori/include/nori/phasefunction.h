/*
Author: Jingyu Wang, ETHz Data Science Master student
Reference: https://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions
*/

#if !defined(__NORI_PHASEFUNCTION_H)
#define __NORI_PHASEFUNCTION_H

#include <nori/object.h>

NORI_NAMESPACE_BEGIN


struct PhaseQueryRecord {
    /// Interaction point
    Point3f p;

    /// Incident direction (in the world frame)
    Vector3f wi;

    /// Outgoing direction (in the world frame)
    Vector3f wo;

    /// Create a new record for sampling the phase function
    PhaseQueryRecord(const Vector3f &wi)
        : wi(wi) { }

    /// Create a new record for querying the phase function
    PhaseQueryRecord(const Vector3f &wi,
            const Vector3f &wo)
        : wi(wi), wo(wo) { }
};

/**
 * \brief Superclass of all phase functions
 */
class PhaseFunction : public NoriObject {
public:

    virtual float sample(PhaseQueryRecord &pRec, const Point2f &sample) const = 0;

    virtual float pdf(const PhaseQueryRecord &pRec) const = 0;

    virtual EClassType getClassType() const override { return EPhaseFunction; }
};

NORI_NAMESPACE_END

#endif /* __NORI_PHASEFUNCTION_H*/
