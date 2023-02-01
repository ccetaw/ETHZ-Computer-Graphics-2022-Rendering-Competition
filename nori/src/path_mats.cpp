#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

class PathMATS : public Integrator {
public:
    PathMATS(const PropertyList &props) {
        /* No parameters this time */
    }

    /*
    * Deprecated recursive version
    * Works but bugs exist
    */

    // Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
    //     float q = 0.04; // Russian roulette factor
    //     /* Return black when no intersection */
    //     Intersection its;
    //     if (!scene->rayIntersect(ray, its))
    //         return Color3f(0.0f);

    //     /* Light transportation */
    //     Color3f light_e(0.f); // Light source
    //     Color3f light_r(0.f); // Reflected light (direct illumination)
    //     auto bsdf = its.mesh->getBSDF();

    //     // Hits an emitter
    //     if (its.mesh->isEmitter())
    //     {
    //         auto emitter = its.mesh->getEmitter();
    //         EmitterQueryRecord lRec(ray.o);
    //         lRec.p = its.p;
    //         lRec.wi = (lRec.p - lRec.ref).normalized();
    //         lRec.n = its.shFrame.n;
    //         light_e = emitter->eval(lRec);
    //     }

    //     // Sample BSDF
    //     BSDFQueryRecord bRec(its.toLocal((-ray.d).normalized()));
    //     bRec.uv = its.uv;
    //     Color3f brdf = bsdf->sample(bRec, sampler->next2D());

    //     // Construct shadow ray and trace it
    //     if (sampler->next1D() > q)
    //     {
    //         Ray3f shadow_ray(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
    //         light_r = brdf * Li(scene, sampler, shadow_ray);
    //     }
    //     else
    //         return light_e + Color3f(0.f);
        
    //     return light_e + light_r / (1-q);
    // }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const{
        Color3f L(0.0f), beta(1.f);
		Ray3f ray_s = ray;
		for(int bounces = 0; ; bounces++) {
            // Find intersection
            Intersection its;
            bool findIntersection = scene->rayIntersect(ray_s, its);
            /*
            * Monte Carlo integration using BSDF sampling.
            * The first term L_e is incorporated smoothly into this process.
            * If using emitter sampling, the first bounce and the specular case should be treated explicitly.
            */ 
            if (findIntersection)
            {
                if (its.mesh->isEmitter()) // Intersection with emitter
                {
                    auto emitter = its.mesh->getEmitter();
                    EmitterQueryRecord lRec(ray_s.o, its.p, its.shFrame.n);
                    L += beta * emitter->eval(lRec);
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
                        L += beta * emitter->eval(lRec);
                    }
                }
                break;
            }

            // Russian roulette termination starts from the 3rd iteration
            if (bounces > 3)
            {
                const float q = std::min(beta.maxCoeff(), 0.99f);
                if (sampler->next1D() > q || q == 0.f)
                    break;
                else
                    beta /= q;
            }
            
            // Sample BSDF, update the throughput, generate next ray
			auto bsdf = its.mesh->getBSDF();
			BSDFQueryRecord bRec(its.toLocal(-ray_s.d));
			bRec.uv = its.uv;
			bRec.p = its.p;
			const Color3f brdf = bsdf->sample(bRec, sampler->next2D());
			beta *= brdf;
			ray_s = Ray3f(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
		}
		return L;
    }

    std::string toString() const {
        return "PathMATS[]";
    }
};

NORI_REGISTER_CLASS(PathMATS, "path_mats");
NORI_NAMESPACE_END