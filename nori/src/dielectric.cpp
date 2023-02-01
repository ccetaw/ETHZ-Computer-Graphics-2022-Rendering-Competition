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

#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/texture.h>

NORI_NAMESPACE_BEGIN

/// Ideal dielectric BSDF
class Dielectric : public BSDF {
public:
    Dielectric(const PropertyList &propList): m_reflectance(nullptr), m_transmittance(nullptr) {
        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f);

        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f);

        if(propList.has("reflectance")) {
            PropertyList l;
            l.setColor("value", propList.getColor("reflectance"));
            m_reflectance = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
        }

        // m_reflectance = propList.getColor("reflectance", Color3f(1.f));

        if(propList.has("transmittance")) {
            PropertyList l;
            l.setColor("value", propList.getColor("transmittance"));
            m_transmittance = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
        }
        // m_transmittance = propList.getColor("transmittance", Color3f(1.f));
    }

    virtual void addChild(NoriObject *obj) override {
        switch (obj->getClassType()) {
            case ETexture:
                // if(obj->getIdName() == "reflectance") {
                //     if (m_reflectance)
                //         throw NoriException("There is already a reflectance defined!");
                //     m_reflectance = static_cast<Texture<Color3f> *>(obj);
                // }
                // else {
                //     throw NoriException("The name of this texture does not match any field!");
                // }

                // if(obj->getIdName() == "transmittance") {
                //     if (m_transmittance)
                //         throw NoriException("There is already a transmittance defined!");
                //     m_transmittance = static_cast<Texture<Color3f> *>(obj);
                // }
                // else {
                //     throw NoriException("The name of this texture does not match any field!");
                // }

                if (m_transmittance)
                        throw NoriException("There is already a transmittance defined!");
                m_transmittance = static_cast<Texture<Color3f> *>(obj);

                if (m_reflectance)
                    throw NoriException("There is already a reflectance defined!");
                m_reflectance = static_cast<Texture<Color3f> *>(obj);
                
                break;

            default:
                throw NoriException("Dielectric::addChild(<%s>) is not supported!",
                                    classTypeName(obj->getClassType()));
        }
    }

    virtual void activate() override {
        if(!m_reflectance) {
            PropertyList l;
            l.setColor("value", Color3f(0.5f));
            m_reflectance = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
            m_reflectance->activate();
        }
        if(!m_transmittance) {
            PropertyList l;
            l.setColor("value", Color3f(0.5f));
            m_transmittance = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
            m_transmittance->activate();
        }
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
        float F_r = fresnel(Frame::cosTheta(bRec.wi), m_extIOR, m_intIOR); // Fresnel coefficient
        float eta, eta_inv;
        Normal3f n; // Normal at the same side as the incident camera ray is (in local coordinates)

        if (Frame::cosTheta(bRec.wi) < 0) // ray shoot from the dielectric to the air
        {
            eta = m_extIOR / m_intIOR;
            eta_inv = 1.f/eta;
            n = Normal3f(0.f, 0.f, -1.f);
        }
        else if (Frame::cosTheta(bRec.wi) >= 0) // ray shoot from the air to the dielectric
        {
            eta = m_intIOR / m_extIOR;
            eta_inv = 1.f/eta;
            n = Normal3f(0.f, 0.f, 1.f);
        }
        else return Color3f(0.f);

        if (sample.x() < F_r) // Reflection in local coordinates
        {
            bRec.eta = 1.f; // no change, still in the same medium
            bRec.wo = Vector3f(
                -bRec.wi.x(),
                -bRec.wi.y(),
                bRec.wi.z()
            );
        
            /*
            The result has followed the convention that BSDF returns sampled values multiplied by cosTheta_o. Besides, because of Monte Carlo sampling, the 
            result is furthure divided by F_r to preserve the energy. In the end it gives abedo (1.0, 1.0, 1.0)
            */
            return m_reflectance->eval(bRec.uv); 
        }
        else // Refraction in local coordinates
        {
            bRec.eta = eta;
            float wi_dot_n = bRec.wi.dot(n);
            bRec.wo = -eta_inv * (bRec.wi - wi_dot_n*n) - n*sqrtf(1 - eta_inv*eta_inv * (1-wi_dot_n*wi_dot_n));

            /*
            See comments above. The result is divided by 1-F_r
            */
            return m_transmittance->eval(bRec.uv);
        }

    }

    virtual bool isDelta() const override{ return true; }

    virtual std::string toString() const override {
        return tfm::format(
            "Dielectric[\n"
            "  intIOR = %f,\n"
            "  extIOR = %f\n"
            "]",
            m_intIOR, m_extIOR);
    }
private:
    float m_intIOR, m_extIOR;
    Texture<Color3f> * m_reflectance;
    Texture<Color3f> * m_transmittance;
};

NORI_REGISTER_CLASS(Dielectric, "dielectric");
NORI_NAMESPACE_END
