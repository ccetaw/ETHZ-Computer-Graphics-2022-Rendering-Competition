#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class DirectIntegratorEMS : public Integrator {
public:
    DirectIntegratorEMS(const PropertyList &props) {
        /* No parameters this time */
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        /* Return black when no intersection */
        Intersection its;
        if (!scene->rayIntersect(ray, its))
        {
            for (auto emitter: scene->getLights())
            {
                if (emitter->isInfinity())
                {
                    EmitterQueryRecord lRec;
                    lRec.wi = ray.d;
                    return emitter->eval(lRec);
                }
            }
            return Color3f(0.f);
        }

        /* Light transportation */
        Color3f light_r(0.f); // Reflected light (direct illumination)
        Color3f light_e(0.f); // Directly emitted light
        auto bsdf = its.mesh->getBSDF();
        auto emitter = scene->getRandomEmitter(sampler->next1D());
        int n_emitters = scene->getLights().size();

        // Get the radiance directly from the emitter
        if (its.mesh->isEmitter())
        {
            auto emitter = its.mesh->getEmitter();
            EmitterQueryRecord lRec(ray.o);
            lRec.p = its.p;
            lRec.wi = (lRec.p - lRec.ref).normalized();
            lRec.n = its.shFrame.n;
            light_e = emitter->eval(lRec);
        }


        // Sample the emitters
        EmitterQueryRecord lRec(its.p);
        Color3f light = emitter->sample(lRec, sampler->next2D()); 
        Intersection its_s; // shadow ray intersection

        // No occlusion
        if(!scene->rayIntersect(lRec.shadowRay, its_s))
        {
            BSDFQueryRecord bRec(its.toLocal(-ray.d), its.toLocal(lRec.wi), ESolidAngle);
            bRec.uv = its.uv;
            light_r += n_emitters * bsdf->eval(bRec) * light * std::max(lRec.wi.dot(its.shFrame.n), 0.f);
        } 
        
        /* Return accumulated light intensity */
        return light_r + light_e;
    }

    std::string toString() const {
        return "DirectIntegratorEMS[]";
    }
};

NORI_REGISTER_CLASS(DirectIntegratorEMS, "direct_ems");
NORI_NAMESPACE_END