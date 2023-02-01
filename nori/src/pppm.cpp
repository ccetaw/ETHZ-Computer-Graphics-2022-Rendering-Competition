/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/
#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/scene.h>
#include <nori/photon.h>

NORI_NAMESPACE_BEGIN

class ProbabilisticProgressivePhotonMapper : public Integrator {
public:
    /// Photon map data structure
    typedef PointKDTree<Photon> PhotonMap;

    ProbabilisticProgressivePhotonMapper(const PropertyList &props) {
        /* Lookup parameters */
        m_photonCount  = props.getInteger("photonCount", 1000000);
        m_photonRadius = props.getFloat("photonRadius", 0.0f /* Default: automatic */);
        m_photonRadiusCopy = m_photonRadius;
        m_numIterations = props.getInteger("numIterations", 100);
        m_alpha = props.getFloat("alpha", 0.5);
    }

    virtual int multiple_render() override {return m_numIterations;}

    virtual void preprocess(const Scene *scene) override {
        cout << "Gathering " << m_photonCount << " photons .. " << endl;

        /* Create a sample generator for the preprocess step */
        

        /* Allocate memory for the photon map */
        m_photonMap = std::unique_ptr<PhotonMap>(new PhotonMap());
        m_photonMap->reserve(m_photonCount);

		/* Estimate a default photon radius */
		if (m_photonRadius == 0)
			m_photonRadius = scene->getBoundingBox().getExtents().norm() / 500.0f;

	

		/* How to add a photon?
		 * m_photonMap->push_back(Photon(
		 *	Point3f(0, 0, 0),  // Position
		 *	Vector3f(0, 0, 1), // Direction
		 *	Color3f(1, 2, 3)   // Power
		 * ));
		 */

		// put your code to trace photons here
        int currentPhotonCount = 0;
        m_photonEmitted = 0;
        while (currentPhotonCount < m_photonCount)
        {
            auto const emitters = scene->getLights();
            for (auto const emitter: emitters)
            {
                // Emit a photon
                m_photonEmitted++;
                Ray3f photonRay;
                Color3f power = emitter->samplePhoton(photonRay, m_sampler->next2D(), m_sampler->next2D());
                Color3f beta(1.f);
                while (true)
                {
                    // Deposit the photon when intersection
                    Intersection its;
                    if(!scene->rayIntersect(photonRay, its))
                        break;
                    if (its.mesh->getBSDF()->isDiffuse())
                    {
                        m_photonMap->push_back(Photon(its.p, -photonRay.d, beta*power));
                        currentPhotonCount++;
                    }

                    // Russian roulette termination
                    const float q = std::min(beta.maxCoeff(), 0.99f);
                    if (m_sampler->next1D() > q)
                        break;
                    else
                        beta /= q;
                
                    // Sample new direction
                    BSDFQueryRecord bRec(its.toLocal(-photonRay.d));
                    beta *= its.mesh->getBSDF()->sample(bRec, m_sampler->next2D());
                    photonRay = Ray3f(its.p, its.toWorld(bRec.wo), Epsilon, INFINITY);
                }
            }

            float progress = (currentPhotonCount+0.f)/m_photonCount;
            if ((currentPhotonCount+1)%(int(0.01*m_photonCount)) == 0)
                printProgress(progress);
        }
        std::cout << std::endl;

		/* Build the photon map */
        m_photonMap->build();
    }

    virtual Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &_ray) const override {
    	
		/* How to find photons?
		 * std::vector<uint32_t> results;
		 * m_photonMap->search(Point3f(0, 0, 0), // lookup position
		 *                     m_photonRadius,   // search radius
		 *                     results);
		 *
		 * for (uint32_t i : results) {
		 *    const Photon &photon = (*m_photonMap)[i];
		 *    cout << "Found photon!" << endl;
		 *    cout << " Position  : " << photon.getPosition().toString() << endl;
		 *    cout << " Power     : " << photon.getPower().toString() << endl;
		 *    cout << " Direction : " << photon.getDirection().toString() << endl;
		 * }
		 */

		// put your code for path tracing with photon gathering here

        int n_emitters = scene->getLights().size();
        Color3f L(0.0f), beta(1.f);
		Ray3f ray_s = _ray;
		for(int bounces = 0; ; bounces++) {
            // Find intersection
            Intersection its;
            bool findIntersection = scene->rayIntersect(ray_s, its);
            /*
            * Monte Carlo integration using BSDF sampling.
            * The first term L_e is incorporated smoothly into this process.
            * If using emitter sampling, the first bounce and the specular case should be treated explicitly.
            */ 
            if (findIntersection) // Break when no intersection 
            {
                if (its.mesh->isEmitter()) // Intersection with emitter
                {
                    auto emitter = its.mesh->getEmitter();
                    EmitterQueryRecord lRec(ray_s.o, its.p, its.shFrame.n);
                    L += beta * emitter->eval(lRec);
                }  
            }
            else // env light
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

            // When diffuse surface, add photon contribution and terminate
            if (its.mesh->getBSDF()->isDiffuse())
            {
                std::vector<uint32_t> results;
                m_photonMap->search(its.p, // lookup position
                                    m_photonRadius,   // search radius
                                    results);

                Color3f photonRadianceEstimation(0.f);
                for (uint32_t i : results) 
                {
                    const Photon &photon = (*m_photonMap)[i];
                    // cout << "Found photon!" << endl;
                    // cout << " Position  : " << photon.getPosition().toString() << endl;
                    // cout << " Power     : " << photon.getPower().toString() << endl;
                    // cout << " Direction : " << photon.getDirection().toString() << endl;
                    BSDFQueryRecord bRec(its.toLocal(-ray_s.d), its.toLocal(photon.getDirection()), ESolidAngle);
                    bRec.uv = its.uv;
                    photonRadianceEstimation += its.mesh->getBSDF()->eval(bRec) * photon.getPower() * INV_PI / (m_photonRadius * m_photonRadius);
                }
                L += beta * photonRadianceEstimation / m_photonEmitted * n_emitters;

                break;
            }

            // Russian roulette termination starts from the 3rd iteration
            if (bounces > 3)
            {
                const float q = std::min(beta.maxCoeff(), 0.99f);
                if (sampler->next1D() > q)
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

    virtual void postprocess(int i){
        m_photonMap.reset();

        float r_next = m_photonRadiusCopy * m_photonRadiusCopy;
        for (int k=1; k<i+1; k++)
            r_next *= (k + m_alpha) / k;
        r_next /= i+1;

        // Set radius according to the iterations
        // Iteration starts from 0
        m_photonRadius = sqrt(r_next);
        cout << "Image " << i  << " done\n";
        std::cout.flush();
        
    }

    

    virtual std::string toString() const override {
        return tfm::format(
            "ProbabilisticProgressivePhotonMapper[\n"
            "  photonCount = %i,\n"
            "  photonRadius = %f\n"
            "  numIterations = %i\n"
            "  alpha = %f\n"
            "]",
            m_photonCount,
            m_photonRadius,
            m_numIterations,
            m_alpha
        );
    }
private:
    /* 
     * Important: m_photonCount is the total number of photons deposited in the photon map,
     * NOT the number of emitted photons. You will need to keep track of those yourself.
     */ 
    int m_photonCount;
    int m_photonEmitted;
    int m_numIterations;
    float m_photonRadius;
    float m_photonRadiusCopy;
    float m_alpha;
    std::unique_ptr<PhotonMap> m_photonMap;
    Sampler* m_sampler = static_cast<Sampler *>(
            NoriObjectFactory::createInstance("independent", PropertyList()));;
    
};

NORI_REGISTER_CLASS(ProbabilisticProgressivePhotonMapper, "pppm");
NORI_NAMESPACE_END
