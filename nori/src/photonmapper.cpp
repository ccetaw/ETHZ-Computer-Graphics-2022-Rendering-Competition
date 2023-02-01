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

#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/scene.h>
#include <nori/photon.h>

NORI_NAMESPACE_BEGIN



class PhotonMapper : public Integrator {
public:
    /// Photon map data structure
    typedef PointKDTree<Photon> PhotonMap;

    PhotonMapper(const PropertyList &props) {
        /* Lookup parameters */
        m_photonCount  = props.getInteger("photonCount", 1000000);
        m_photonRadius = props.getFloat("photonRadius", 0.0f /* Default: automatic */);
    }

    virtual void preprocess(const Scene *scene) override {
        cout << "Gathering " << m_photonCount << " photons .. " << endl;

        /* Create a sample generator for the preprocess step */
        Sampler *sampler = static_cast<Sampler *>(
            NoriObjectFactory::createInstance("independent", PropertyList()));

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
                Color3f power = emitter->samplePhoton(photonRay, sampler->next2D(), sampler->next2D());
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
                    if (sampler->next1D() > q)
                        break;
                    else
                        beta /= q;
                
                    // Sample new direction
                    BSDFQueryRecord bRec(its.toLocal(-photonRay.d));
                    beta *= its.mesh->getBSDF()->sample(bRec, sampler->next2D());
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

    virtual std::string toString() const override {
        return tfm::format(
            "PhotonMapper[\n"
            "  photonCount = %i,\n"
            "  photonRadius = %f\n"
            "]",
            m_photonCount,
            m_photonRadius
        );
    }
private:
    /* 
     * Important: m_photonCount is the total number of photons deposited in the photon map,
     * NOT the number of emitted photons. You will need to keep track of those yourself.
     */ 
    int m_photonCount;
    int m_photonEmitted;
    float m_photonRadius;
    std::unique_ptr<PhotonMap> m_photonMap;
};

NORI_REGISTER_CLASS(PhotonMapper, "photonmapper");
NORI_NAMESPACE_END
