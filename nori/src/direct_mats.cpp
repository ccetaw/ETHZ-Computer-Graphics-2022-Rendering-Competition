#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class DirectIntegratorMATS : public Integrator {
public:
    DirectIntegratorMATS(const PropertyList &props) {
        /* No parameters this time */
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        /* Light transportation */
        Intersection its;
        Color3f L(0.f);
        /* Find if there is env map when no intersection */
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

        // Get the radiance directly from the emitter
        auto bsdf = its.mesh->getBSDF();
        if (its.mesh->isEmitter())
        {
            auto emitter = its.mesh->getEmitter();
            EmitterQueryRecord lRec(ray.o);
            lRec.p = its.p;
            lRec.wi = (lRec.p - lRec.ref).normalized();
            lRec.n = its.shFrame.n;
            L += emitter->eval(lRec); // Directly emitted light
        }

        // Sample BSDF
        BSDFQueryRecord bRec(its.toLocal((-ray.d).normalized()));
        bRec.uv = its.uv;
        Color3f brdf = bsdf->sample(bRec, sampler->next2D());

        // Construct shadow ray and test what it intersects
        Ray3f shadow_ray(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
        Intersection its_s;
        if (scene->rayIntersect(shadow_ray, its_s)) // intersection -> if area emitter
        {
            if (its_s.mesh->isEmitter())
            {
                auto emitter = its_s.mesh->getEmitter();
                EmitterQueryRecord lRec(its.p);
                lRec.p = its_s.p;
                lRec.n = its_s.shFrame.n;
                lRec.wi = (lRec.p - lRec.ref).normalized();
                Color3f light = emitter->eval(lRec); 
                // Reflected light (direct illumination)
                L += brdf * light; // cosine term is included in the brdf
            }
        }
        else // no intersection -> get env map
        {
            for (auto emitter: scene->getLights())
            {
                if (emitter->isInfinity())
                {
                    EmitterQueryRecord lRec;
                    lRec.wi = shadow_ray.d;
                    L += brdf * emitter->eval(lRec);
                }    
            }
        }
        
        /* Return accumulated light intensity */
        return L;
    }

    std::string toString() const {
        return "DirectIntegratorMATS[]";
    }
};

NORI_REGISTER_CLASS(DirectIntegratorMATS, "direct_mats");
NORI_NAMESPACE_END