#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>

NORI_NAMESPACE_BEGIN

class DirectIntegrator : public Integrator {
public:
    DirectIntegrator(const PropertyList &props) {
        /* No parameters this time */
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        /* Return black when no intersection */
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return Color3f(0.0f);

        /* Light transportation */
        Color3f light_total(0.f);
        auto bsdf = its.mesh->getBSDF();
        auto emitters = scene->getLights();
        for (auto const emitter: emitters)
        {
            EmitterQueryRecord lRec(its.p);
            Color3f light = emitter->sample(lRec, Point2f(0.f,0.f)); 
            Intersection its_s; // shadow ray intersection
            if(!scene->rayIntersect(lRec.shadowRay, its_s))
            {
                BSDFQueryRecord bRec(its.toLocal(lRec.wi), its.toLocal(-ray.d), ESolidAngle);
                bRec.uv = its.uv;
                light_total += bsdf->eval(bRec) * light * std::max(lRec.wi.dot(its.shFrame.n), 0.f);
            }
        }
        
        /* Return accumulated light intensity */
        return light_total;
    }

    std::string toString() const {
        return "DirectIntegrator[]";
    }
};

NORI_REGISTER_CLASS(DirectIntegrator, "direct");
NORI_NAMESPACE_END