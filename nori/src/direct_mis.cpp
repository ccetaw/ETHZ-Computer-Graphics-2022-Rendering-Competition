#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class DirectIntegratorMIS : public Integrator {
public:
    DirectIntegratorMIS(const PropertyList &props) {
        /* No parameters this time */
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        // First ray intersection
        Intersection its;
        
        if (!scene->rayIntersect(ray, its)) /* Return envmap when no intersection */
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

        Color3f light_r(0.f); // Reflected light (direct illumination)
        Color3f light_e(0.f); // Directly emitted light
        double pdf_em, pdf_mat;
        auto bsdf = its.mesh->getBSDF();
        auto emitter = scene->getRandomEmitter(sampler->next1D());
        int n_emitters = scene->getLights().size();

        if (its.mesh->isEmitter()) // Get the radiance directly from the emitter
        {
            auto emitter = its.mesh->getEmitter();
            EmitterQueryRecord lRec(ray.o, its.p, its.shFrame.n);
            light_e = emitter->eval(lRec);
        }

        // Second ray intersection
        Intersection its_2;
        BSDFQueryRecord bRec(its.toLocal((-ray.d).normalized()));
        EmitterQueryRecord lRec(its.p);
        Color3f light;
        Color3f brdf;
        Ray3f shadow_ray;
        bRec.uv = its.uv;
        bRec.p = its.p;
        bRec.measure = ESolidAngle;
        
        // Sample BSDF
        brdf = bsdf->sample(bRec, sampler->next2D());
        pdf_mat = bsdf->pdf(bRec);
        shadow_ray = Ray3f(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
        if (scene->rayIntersect(shadow_ray, its_2))
        {
            if (its_2.mesh->isEmitter())
            {
                lRec = EmitterQueryRecord(its.p, its_2.p, its_2.shFrame.n);
                pdf_em = its_2.mesh->getEmitter()->pdf(lRec);
                light = its_2.mesh->getEmitter()->eval(lRec); 
                if (bRec.measure == EDiscrete)
                    light_r += brdf * light; // cosine term is included in the brdf
                else if (pdf_mat + pdf_em > 1e-8)
                    light_r += pdf_mat / (pdf_em+pdf_mat) * brdf * light; 
            }
        }
        else // Light from env
        {
            for (auto emitter: scene->getLights())
            {
                if (emitter->isInfinity())
                {
                    lRec.wi = shadow_ray.d;
                    pdf_em = emitter->pdf(lRec);
                    light = emitter->eval(lRec);
                    if (bRec.measure == EDiscrete)
                        light_r += brdf * light; // cosine term is included in the brdf
                    else if (pdf_mat + pdf_em > 1e-8)
                        light_r += pdf_mat / (pdf_em+pdf_mat) * brdf * light; 
                }
            }
            
        }

        // Sample emitter
        if (bRec.measure != EDiscrete)
        {
            light = emitter->sample(lRec, sampler->next2D()); 
            pdf_em = emitter->pdf(lRec);
            shadow_ray = lRec.shadowRay;
            if(!scene->rayIntersect(shadow_ray, its_2)) // No occlusion
            {
                bRec = BSDFQueryRecord(its.toLocal((-ray.d).normalized()), its.toLocal(lRec.wi), ESolidAngle);
                bRec.uv = its.uv;
                brdf = bsdf->eval(bRec);
                pdf_mat = bsdf->pdf(bRec);
                if (pdf_mat + pdf_em > 1e-8)
                    light_r += pdf_em / (pdf_em+pdf_mat) * n_emitters * brdf * light * std::max(lRec.wi.dot(its.shFrame.n), 0.f);
            } 
        }
        
        
        /* Return accumulated light intensity */
        return light_r + light_e;
    }

    std::string toString() const {
        return "DirectIntegratorMIS[]";
    }
};

NORI_REGISTER_CLASS(DirectIntegratorMIS, "direct_mis");
NORI_NAMESPACE_END