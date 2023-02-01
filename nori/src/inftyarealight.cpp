/*
Author: Jingyu Wang, ETHz Data Science Master student
*/

#include <nori/emitter.h>
#include <nori/warp.h>
#include <nori/shape.h>
#include <nori/bitmap.h>
#include <nori/transform.h>
#include <filesystem/path.h>
#include <filesystem/resolver.h>

NORI_NAMESPACE_BEGIN
/*
Adapted from pbrt 
*/
class InftyAreaEmitter : public Emitter {
public:
    InftyAreaEmitter(const PropertyList &props) {
        // Read parameters form .xml file
        m_file = props.getString("filename", "");
        m_lightToWorld = props.getTransform("toWorld", Transform());
        m_worldToLight = m_lightToWorld.getInverseMatrix();

        // Load the .exr file as environment
        filesystem::path filePath = getFileResolver()->resolve(m_file);
        
        if (filePath.extension() == "exr") {
            m_image = Bitmap(filePath.str());
        } else {
            cerr << "Environment light fails to load file: unknown file \"" << m_file
            << "\", expected an extension of type .exr" << endl;
        }
        m_size = Vector2i(m_image.rows(), m_image.cols());    

        // Precompute the pdf
        m_pPhi = Eigen::MatrixXf(m_size.x(), m_size.y());
        for (int x = 0; x < m_size.x(); x++)
        {
            float sinTheta = sin(M_PI * float(x + 0.5f) / float(m_size.x()));
            for (int y = 0; y < m_size.y(); y++)
            {
                m_pPhi(x, y) = m_image(x, y).getLuminance() * sinTheta;
            }
        }
        m_pTheta = Eigen::VectorXf(m_pPhi.rows());
        m_pTheta = m_pPhi.rowwise().sum();
        m_pTheta = m_pTheta / m_pTheta.sum();
        for (int i = 0; i < m_pPhi.rows(); i++)
        {
            if (m_pTheta(i) > 1e-5)
                m_pPhi.row(i) /= m_pTheta(i);
            else
                m_pPhi.row(i) *= 0;
        }
            
    }

    virtual std::string toString() const override {
        return tfm::format(
                "EnvironmentLight[\n"
                "  file = %s,\n"
                "  lightToWorld = %s\n"
                "]",
                m_file,
                indent(m_lightToWorld.toString(), 18));
    }

    virtual Color3f eval(const EmitterQueryRecord & lRec) const override {
        // Nearest neighbor interpolation
        Vector3f w = (m_worldToLight * lRec.wi).normalized();
        float theta = std::acos(clamp(w.z(), -1.f, 1.f));
        float p = std::atan2(w.y(), w.x());
        float phi = (p < 0) ? (p + 2 * M_PI) : p; 
        int x = round(theta * INV_PI * (m_size.x() - 1));
        int y = round(phi * INV_TWOPI * (m_size.y() - 1));
        x = clamp(x, 0, m_size.x()-1);
        y = clamp(y, 0, m_size.y()-1);
        return m_image(x, m_size.y() -1 - y);
    }

    virtual Color3f sample(EmitterQueryRecord & lRec, const Point2f & sample) const override {
        float theta = M_PI * sample.x(), phi = 2 * M_PI * sample.y();
        float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
        float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
        lRec.wi = m_lightToWorld * Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
        lRec.shadowRay = Ray3f(lRec.ref, lRec.wi, Epsilon, INFINITY, this->getMedium());
        if (pdf(lRec) > 1e-9)
            return eval(lRec)/pdf(lRec);
        else 
            return Color3f(0.f);
    }

    virtual float pdf(const EmitterQueryRecord &lRec) const override {
        Vector3f w = (m_worldToLight * lRec.wi).normalized();
        float theta = std::acos(clamp(w.z(), -1.f, 1.f));
        float p = std::atan2(w.y(), w.x());
        float phi = (p < 0) ? (p + 2 * M_PI) : p; 
        float sinTheta = std::sin(theta);
        if (sinTheta == 0) return 0;
        
        int x = round(theta * INV_PI * (m_size.x() - 1));
        int y = round(phi * INV_TWOPI * (m_size.y() -1));
        x = clamp(x, 0, m_size.x()-1);
        y = clamp(y, 0, m_size.y()-1);
        float pdf = m_pTheta(x) * m_pPhi(x, m_size.y() -1 - y);
        
        return pdf / (2 * sinTheta) * INV_PI * INV_PI;
    }

    virtual Color3f samplePhoton(Ray3f &ray, const Point2f &sample1, const Point2f &sample2) const override {

        // // Sample a point on the surface
        // ShapeQueryRecord sRec;
        // m_shape->sampleSurface(sRec, sample1);
        // Point3f o = sRec.p;

        // // Sample a direction
        // Vector3f dir = Warp::squareToCosineHemisphere(sample2);
        // Frame F(sRec.n);
        // dir = F.toWorld(dir);

        // ray = Ray3f(o, dir, Epsilon, INFINITY);

        // return M_PI * 1.f/m_shape->pdfSurface(sRec) * m_radiance;
    }

    virtual bool isInfinity() const override {return true;}

protected:
    std::string m_file;
    Bitmap m_image;
    Vector2i m_size;
    Transform m_worldToLight;
    Transform m_lightToWorld;
    Eigen::VectorXf m_pTheta;
    Eigen::MatrixXf m_pPhi;
};

NORI_REGISTER_CLASS(InftyAreaEmitter, "env")
NORI_NAMESPACE_END