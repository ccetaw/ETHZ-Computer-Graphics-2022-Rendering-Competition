/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/
#include <nori/medium.h>

NORI_NAMESPACE_BEGIN


class HomogeneousMedium : public Medium {
public:
    HomogeneousMedium(const PropertyList &propList){
        m_sigma_a = propList.getColor("sigma_a", 0.5f);
        m_sigma_s = propList.getColor("sigma_s", 0.5f);
        m_sigma_t = m_sigma_a + m_sigma_s;
    }

    // Calculate the transmittance give a ray
    virtual Color3f Tr(const Ray3f &ray) const override {
        return exp(- m_sigma_t * std::min(ray.maxt, MAXFLOAT));
    }

    virtual Color3f sample(const Ray3f &ray, const Point2f &sample, MediumInteraction &mRec) const override {
        // Sample a channel and distance along the ray
        const int N_CHANNELS = 3;
        int channel = std::min(int(sample.x() * N_CHANNELS), N_CHANNELS - 1);
        float dist = -std::log(1 - sample.y()) / m_sigma_t(channel);
        float t = std::min(dist * ray.d.norm(), ray.maxt);
        // cout << "t " << t << endl;
        bool sampleMedium = t < ray.maxt;
        // cout << "maxt " << ray.maxt << endl;
        if (sampleMedium)
            mRec = MediumInteraction(ray(t), t, -(ray.d).normalized(), this->getPhase(), this);

        // Compute the trasmittance and sampling density
        Color3f Tr = exp(- m_sigma_t * std::min(t, MAXFLOAT) * ray.d.norm());
            

        // Return weighting factor for scattering from homogeneous medium
        Color3f density = sampleMedium ? (m_sigma_t * Tr) : Tr;
        float pdf = 0;
        for (int i = 0; i < N_CHANNELS; i++)
            pdf += density(i);
        pdf *= 1.f / N_CHANNELS;
        if (sampleMedium)
            return Tr * m_sigma_s / pdf;
        else 
            return Tr / pdf ;  
        
              
    }

    /// Return a human-readable summary
    virtual std::string toString() const override {
        return tfm::format(
            "HomogeneousMedium[\n"
            "  sigma_a = %s,\n"
            "  sigma_s = %s,\n"
            "  sigma_t = %s,\n"
            "]",
            m_sigma_a.toString(),
            m_sigma_s.toString(),
            m_sigma_t.toString()
        );
    }

private:
    Color3f m_sigma_a, m_sigma_s, m_sigma_t;
};

NORI_REGISTER_CLASS(HomogeneousMedium, "homogeneous");
NORI_NAMESPACE_END
