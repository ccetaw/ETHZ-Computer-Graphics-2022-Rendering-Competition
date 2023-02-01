#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class PathMIS : public Integrator {
public:
    PathMIS(const PropertyList &props) {
        /* No parameters this time */
    }

    /*
    * Deprecated recursive version 
    * -------------------------------------------------------------------------------
    */
    // Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
    //     float q = 0.04; // Russian roulette factor

    //     // First ray intersection
    //     Intersection its;
    //     if (!scene->rayIntersect(ray, its)) /* Return black when no intersection */
    //         return Color3f(0.0f);

    //     Color3f light_d(0.f); // Direct illuminamtion
    //     Color3f light_i(0.f); // Indirect illumination
    //     Color3f light_e(0.f); // Directly emitted light

        
    //     auto bsdf = its.mesh->getBSDF();
    //     auto emitter = scene->getRandomEmitter(sampler->next1D());
    //     int n_emitters = scene->getLights().size();

    //     if (its.mesh->isEmitter()) // Get the radiance directly from the emitter
    //     {
    //         auto emitter = its.mesh->getEmitter();
    //         EmitterQueryRecord lRec(ray.o);
    //         lRec.p = its.p;
    //         lRec.wi = (lRec.p - lRec.ref).normalized();
    //         lRec.n = its.shFrame.n;
    //         light_e = emitter->eval(lRec);
    //     }

    //     // Second ray intersection
    //     double pdf_em, pdf_mat, w_mat, w_em;
    //     Intersection its_2;
    //     BSDFQueryRecord bRec(its.toLocal((-ray.d).normalized()));
    //     EmitterQueryRecord lRec(its.p);
    //     Color3f light;
    //     Color3f brdf;
    //     Ray3f shadow_ray;
    //     bRec.uv = its.uv;
    //     bRec.p = its.p;

    //     if (sampler->next1D() > q) // Russian roulette survives
    //     {   
    //         // Sample BSDF and do path tracing
    //         brdf = bsdf->sample(bRec, sampler->next2D());
    //         shadow_ray = Ray3f(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
    //         if (bRec.measure != EDiscrete && scene->rayIntersect(shadow_ray, its_2) && its_2.mesh->isEmitter())
    //         {
    //             lRec.p = its_2.p;
    //             lRec.n = its_2.shFrame.n;
    //             lRec.wi = (lRec.p - lRec.ref).normalized();
    //             pdf_em = its_2.mesh->getEmitter()->pdf(lRec);
    //             pdf_mat = bsdf->pdf(bRec);
    //             w_mat = pdf_mat/(pdf_em+pdf_mat);
    //         }
    //         else
    //             w_mat = 1.f;

    //         light_i =  w_mat * brdf * Li(scene, sampler, shadow_ray); // cosine term is included in the brdf
    //     }
    //     else
    //         return light_e;

    //     // Sample emitter
    //     if (bsdf->isDiffuse()) // If not specular
    //     {
    //         bRec.measure = ESolidAngle;
    //         light = emitter->sample(lRec, sampler->next2D()); 
    //         pdf_em = emitter->pdf(lRec);
    //         shadow_ray = lRec.shadowRay;
    //         if(!scene->rayIntersect(shadow_ray, its_2)) // No occlusion
    //         {
    //             bRec.wo = its.toLocal(lRec.wi);
    //             brdf = bsdf->eval(bRec);
    //             pdf_mat = bsdf->pdf(bRec);
    //             w_em = pdf_em / (pdf_em+pdf_mat);
    //             light_d += n_emitters * w_em * brdf * light * std::max(lRec.wi.dot(its.shFrame.n), 0.f);
    //         } 
    //     }

    //     /* Return accumulated light intensity */
    //     return light_e + (light_d + light_i)/(1.f-q);
    // }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const{
        Color3f L(0.0f), beta(1.f);
		Ray3f ray_s = ray; // Next ray
        float pdf_em = 1.f, pdf_mat = 1.f;
        bool bounceSpecular = false;
        int n_emitters = scene->getLights().size();
		for(int bounces = 0; ; bounces++) 
        {
            // Find intersection
            Intersection its;
            bool findIntersection = scene->rayIntersect(ray_s, its);
            // BSDF sampling continue || first term L_e incorporated
            Color3f Le(0.f);
            if (findIntersection) 
            {
                if (its.mesh->isEmitter()) // Intersection with emitter
                {
                    auto emitter = its.mesh->getEmitter();
                    EmitterQueryRecord lRec(ray_s.o, its.p, its.shFrame.n);
                    Le = emitter->eval(lRec);
                    pdf_em = emitter->pdf(lRec);
                    if (bounces == 0 || bounceSpecular) // First bounce += Le || specular += beta * Le  
                        L += beta * Le;
                    else if (pdf_em + pdf_mat > 1e-8)
                        L += pdf_mat/(pdf_em+pdf_mat) * beta * Le;
                }  
            }
            else // Add env map and break 
            {
                for (auto emitter: scene->getLights())
                {
                    if (emitter->isInfinity())
                    {
                        EmitterQueryRecord lRec(its.p);
                        lRec.wi = ray_s.d;
                        pdf_em = emitter->pdf(lRec);
                        if (bounces == 0 || bounceSpecular) // First bounce += Le || specular += beta * Le  
                            L += beta * emitter->eval(lRec);
                        else if (pdf_em + pdf_mat > 1e-8)
                            L += pdf_mat/(pdf_em+pdf_mat) * beta * emitter->eval(lRec);
                    }
                }
                break;
            }

            // Russian roulette termination starts from the 4th iteration
            if (bounces > 4)
            {
                const float q = std::min(beta.maxCoeff(), 0.99f);
                if (sampler->next1D() > q || q == 0.f)
                    break;
                else
                    beta /= q;
            }

            // Emitter sampling, starting from the first integration P_1
            if (its.mesh->getBSDF()->isDiffuse()) // If not specular
            {
                // Choose a random light source
                auto emitter = scene->getRandomEmitter(sampler->next1D());
                
                
                // Shoot a ray
                EmitterQueryRecord lRec(its.p);
                Color3f Le = emitter->sample(lRec, sampler->next2D()); 
                pdf_em = emitter->pdf(lRec);

                Intersection its_2;
                if(!scene->rayIntersect(lRec.shadowRay, its_2)) // No occlusion
                {
                    BSDFQueryRecord bRec(its.toLocal(-ray_s.d), its.toLocal(lRec.wi), ESolidAngle);
                    bRec.p = its.p;
                    bRec.uv = its.uv;
                    Color3f brdf = its.mesh->getBSDF()->eval(bRec);
                    pdf_mat = its.mesh->getBSDF()->pdf(bRec);
                    if (pdf_em + pdf_mat > 1e-8)
                        L += pdf_em / (pdf_em+pdf_mat) * n_emitters * brdf * beta * Le * std::max(lRec.wi.dot(its.shFrame.n), 0.f);
                } 
            }
            // if specular, no ems contribution
            
            // BSDF sampling, update the throughput, generate next ray
            auto bsdf = its.mesh->getBSDF();
			BSDFQueryRecord bRec(its.toLocal(-ray_s.d));
			bRec.uv = its.uv;
			bRec.p = its.p;
			const Color3f brdf = bsdf->sample(bRec, sampler->next2D());
            if (bRec.measure == EDiscrete)
            {
                bounceSpecular = true;
                pdf_mat = 1.f;
            }
            else
            {
                bounceSpecular = false;
                pdf_mat = bsdf->pdf(bRec);
            }
                
			beta *= brdf;
			ray_s = Ray3f(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
		}
        
		return L;
    }

    std::string toString() const {
        return "PathMIS[]";
    }
};

NORI_REGISTER_CLASS(PathMIS, "path_mis");
NORI_NAMESPACE_END