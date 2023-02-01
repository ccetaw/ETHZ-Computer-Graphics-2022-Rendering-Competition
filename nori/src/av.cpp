#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

class AverageVisibilityIntegrator : public Integrator {
public:
    AverageVisibilityIntegrator(const PropertyList &props) {
        length = props.getFloat("length");
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const {
        /* Find the surface that is visible in the requested direction */
        Intersection its;
        if (!scene->rayIntersect(ray, its))
            return Color3f(1.0f);

        /*
        Using the intersection point its.p, the world space shading normal its.shFrame.n, 
        and the provided sampler, generate a point on the hemisphere and trace a ray into 
        this direction. The ray should have a user-specifiable length. The integrator 
        should return Color3f(0.0f) if the ray segment is occluded and Color3f(1.0f) 
        otherwise.
        */
        Normal3f n = its.shFrame.n;
        Vector3f d = Warp::sampleUniformHemisphere(sampler, n);
        Ray3f ray_sec(its.p, d, 0.01, length);
        if (!scene->rayIntersect(ray_sec, its))
            return Color3f(1.0f);
        else
            return Color3f(0.0f);
    }

    std::string toString() const {
        return "AverageVisibilityIntegrator[]";
    }

private:
    float length;
};

NORI_REGISTER_CLASS(AverageVisibilityIntegrator, "av");
NORI_NAMESPACE_END