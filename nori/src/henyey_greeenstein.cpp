/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/
#include <nori/phasefunction.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief HenyeyGreenstein phase function
 */
class HenyeyGreenstein : public PhaseFunction {
public:
    HenyeyGreenstein(const PropertyList &propList) {
        m_g = propList.getFloat("g", 0.f);
    }

    /// Compute the density of \ref sample() 
    virtual float pdf(const PhaseQueryRecord &pRec) const override {
        float denom = 1 + m_g * m_g + 2 * m_g * pRec.wi.dot(pRec.wo);
        return INV_FOURPI * (1 - m_g * m_g) / (denom * sqrt(denom));
    }

    /// Draw a a sample from the phase function
    virtual float sample(PhaseQueryRecord &pRec, const Point2f &sample) const override {
        float phi = 2 * M_PI * sample.x();
        float cosTheta;
        if (abs(m_g) < 1e-3)
            cosTheta = 1.f - 2 * sample.y();
        else
        {
            float sqrTerm = (1 - m_g * m_g) / (1 - m_g + 2 * m_g * sample.y());
            cosTheta = (1.f + m_g * m_g - sqrTerm * sqrTerm) / (2.f * m_g);
        }
        float sinTheta = sqrt(std::max(0.f, 1 - cosTheta * cosTheta));
        Vector3f v1, v2;
        coordinateSystem(pRec.wi, v1, v2);
        pRec.wo = sinTheta * cos(phi) * pRec.wi + sinTheta * sin(phi) * v2 + cos(phi) * v1;
        // pRec.wo = Warp::squareToUniformSphere(sample);
        return pdf(pRec);
    }

    /// Return a human-readable summary
    virtual std::string toString() const override {
        return tfm::format(
            "HenyeyGreenstein[\n"
            "  g = %f\n"
            "]",
            m_g
        );
    }

private:
    float m_g; // g must be in (-1, 1)
};

NORI_REGISTER_CLASS(HenyeyGreenstein, "HenyeyGreenstein");
NORI_NAMESPACE_END
