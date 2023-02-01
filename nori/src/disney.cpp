/*
Author: Jingyu Wang, ETHz Data Science Master student
Reference: [UCSD CSE 272 Assignment 1: Disney Principled BSDF](https://cseweb.ucsd.edu/~tzli/cse272/homework1.pdf)
           [Rendering the Moana Island Scene Part 1: Implementing the Disney BSDF (schuttejoe.github.io)](https://schuttejoe.github.io/post/disneybsdf/)
Notation is the same as UCSD assignment. Efficiency is not the priority, the clearness of the code matters most. You 
could easily associate the code with the math formulas.
*/

#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/warp.h>
#include <nori/texture.h>

NORI_NAMESPACE_BEGIN

Vector3f cross(const Vector3f &a, const Vector3f &b)
{
    Vector3f r(0.f);
    r(0) = a(1)*b(2)-a(2)*b(1);
    r(1) = a(2)*b(0)-a(0)*b(2);
    r(2) = a(0)*b(1)-a(1)*b(0);
    return r;
}

class Disney : public BSDF {

    

public:
    Disney(const PropertyList &propList) : m_baseColor(nullptr) {
        if(propList.has("baseColor")) {
            PropertyList l;
            l.setColor("value", propList.getColor("baseColor"));
            m_baseColor = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
        }
        cout << m_baseColor << endl;

        m_subsurface = propList.getFloat("subsurface", 0.f);
        m_sheen = propList.getFloat("sheen", 0.f);
        m_sheenTint = propList.getFloat("sheenTint", 0.f);
        m_clearcoat = propList.getFloat("clearcoat", 0.f);
        m_clearcoatGloss = propList.getFloat("clearcoatGloss", 0.f);
        m_specular = propList.getFloat("specular", 0.f);
        m_specTint = propList.getFloat("specTint", 0.f);
        m_specTrans = propList.getFloat("specTrans", 0.f);
        m_roughness = propList.getFloat("roughness", 0.f);
        m_anisotropic = propList.getFloat("anistropic", 0.f);
        m_metallic = propList.getFloat("metallic", 0.f);
        m_eta = 2.f / (1.f - sqrt(0.08f * m_specular)) - 1.f;

        // Secondary parameters
        m_aspect = sqrt(1 - 0.9 * m_anisotropic);
        m_alpha_min = 1e-4;
        m_alpha_x = std::max(m_alpha_min, m_roughness * m_roughness / m_aspect);
        m_alpha_y = std::max(m_alpha_min, m_roughness * m_roughness * m_aspect);
        m_alpha_g = (1 - m_clearcoatGloss) * 0.1f + m_clearcoatGloss * 0.001;
    }
    virtual ~Disney() {
        delete m_baseColor;
    }

    /// Add texture for the albedo
    virtual void addChild(NoriObject *obj) override {
        switch (obj->getClassType()) {
            case ETexture:
                if(obj->getIdName() == "baseColor") {
                    if (m_baseColor)
                        throw NoriException("There is already an albedo defined!");
                    m_baseColor = static_cast<Texture<Color3f> *>(obj);
                }
                else {
                    throw NoriException("The name of this texture does not match any field!");
                }
                break;

            default:
                throw NoriException("Disney::addChild(<%s>) is not supported!",
                                    classTypeName(obj->getClassType()));
        }
    }

    virtual void activate() override {
        if(!m_baseColor) {
            PropertyList l;
            l.setColor("value", Color3f(0.5f));
            m_baseColor = static_cast<Texture<Color3f> *>(NoriObjectFactory::createInstance("constant_color", l));
            m_baseColor->activate();
        }
    }

    /* Helper Functions*/
    // Diffuse
    Vector3f halfVec(Vector3f wi, Vector3f wo) const
    {
        return (wi+wo).normalized();
    }

    float F_D(Vector3f w, float F_D90) const
    {
        float x = clamp(1 - abs(Frame::cosTheta(w)), 0.f, 1.f);
        return 1 + (F_D90 - 1) * pow(x, 5);
    }

    float F_D90(Vector3f wh, Vector3f wo) const
    {
        return 0.5 + 2 * m_roughness * wh.dot(wo) * wh.dot(wo);
    }

    float F_SS(Vector3f w, float F_SS90) const
    {
        float x = clamp(1 - abs(Frame::cosTheta(w)), 0.f, 1.f);
        return 1 + (F_SS90 - 1) * pow(x, 5);
    }

    float F_SS90(Vector3f wh, Vector3f wo) const
    {
        return m_roughness * wh.dot(wo) * wh.dot(wo);
    }


    // Metal
    Color3f F_m(const BSDFQueryRecord &bRec) const
    {
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        float x = clamp(1 - abs(wh.dot(wo)), 0.f, 1.f);

        return m_baseColor->eval(bRec.uv) + (1 - m_baseColor->eval(bRec.uv)) * pow(x, 5);
    }

    float D_m(Vector3f wh) const
    {
        float tmp = wh.x() * wh.x() / (m_alpha_x * m_alpha_x) + wh.y() * wh.y() / (m_alpha_y * m_alpha_y) + wh.z() * wh.z();

        return INV_PI * 1.f / (m_alpha_x * m_alpha_y * tmp * tmp);
    }

    float Lambda_w(Vector3f w) const // In local frame, already
    {
        return ( sqrt( 1.f + (pow(w.x() * m_alpha_x, 2) + pow(w.y() * m_alpha_y, 2) ) / pow(w.z(), 2) ) - 1.f ) / 2.f;
    }

    float G_w(Vector3f w) const
    {
        return 1.f / (1 + Lambda_w(w));
    }

    float G_m(Vector3f wi, Vector3f wo) const
    {
        return G_w(wi) * G_w(wo);
    }

    Color3f K_s(const BSDFQueryRecord &bRec) const
    {
        return Color3f(1.f - m_specTint) + m_specTint * C_tint(bRec);
    }

    Color3f C_0(const BSDFQueryRecord &bRec) const
    {
        return m_specular * R_0(m_eta) * (1.f - m_metallic) * K_s(bRec) + m_metallic * m_baseColor->eval(bRec.uv);
    }

    Color3f F_m_hat(const BSDFQueryRecord &bRec) const
    {
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        float x = clamp(1 - abs(wh.dot(wo)), 0.f, 1.f);
        Color3f c = C_0(bRec);
        // cout << c << endl;
        return c + (Color3f(1.f) - c) * pow(x, 5);
    }

    // Clearcoat
    float R_0(float eta) const
    {
        return (eta - 1.f) * (eta - 1.f) / (eta + 1.f) / (eta + 1.f);
    }

    float F_c(const BSDFQueryRecord &bRec) const
    {
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        float x = clamp(1 - abs(wh.dot(wo)), 0.f, 1.f);

        return R_0(1.5) + (1 - R_0(1.5)) * pow(x, 5);
    }

    float D_c(Vector3f wh) const
    {
        float tmp = M_PI * log(m_alpha_g * m_alpha_g) * (1 + (m_alpha_g * m_alpha_g -1) * wh.z() * wh.z());
        return (m_alpha_g * m_alpha_g - 1) / tmp;
    }

    float Lambda_c_w(Vector3f w) const
    {
        return ( sqrt( 1.f + (pow(w.x() * 0.25f, 2) + pow(w.y() * 0.25f, 2) ) / pow(w.z(), 2) ) - 1.f ) / 2.f;
    }

    float G_c_w(Vector3f w) const
    {
        return 1.f / (1 + Lambda_c_w(w));
    }

    float G_c(Vector3f wi, Vector3f wo) const
    {
        return G_c_w(wi) * G_c_w(wo);
    }

    // Sheen
    Color3f C_tint(const BSDFQueryRecord &bRec) const
    {
        float luminance = m_baseColor->eval(bRec.uv).getLuminance();
        Color3f baseColor = m_baseColor->eval(bRec.uv);
        if (luminance > 0)
        {
            return baseColor;
        }
        else
            return Color3f(1.f);
    }

    Color3f C_sheen(const BSDFQueryRecord &bRec) const 
    {
        return Color3f(1 - m_sheenTint) + m_sheenTint * C_tint(bRec);
    }

    /* Diffuse 
    *  Parameters:
    *   baseColor
    *   roughness
    *   subsurface
    */
    Color3f baseDiffuse(const BSDFQueryRecord &bRec) const
    {
        Vector3f wi = bRec.wi;
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(wi, wo);
        float FD90 = F_D90(wh, wo);

        return m_baseColor->eval(bRec.uv) * INV_PI * F_D(wi, FD90) * F_D(wo, FD90);
    }

    Color3f subsurface(const BSDFQueryRecord &bRec) const
    {
        Vector3f wi = bRec.wi;
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(wi, wo);
        float FSS90 = F_SS90(wh, wo);
        float middleTerm = F_SS(wi, FSS90) * F_SS(wo, FSS90) * ( 1/ ( abs(Frame::cosTheta(wi)) + abs(Frame::cosTheta(wo)) ) - 0.5) + 0.5;
        return 1.25 * m_baseColor->eval(bRec.uv) * INV_PI * middleTerm;
    }

    Color3f evalDiffuse(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return Color3f(0.0f);

        return (1 - m_subsurface) * baseDiffuse(bRec) + m_subsurface * subsurface(bRec);
    }

    float pdfDiffuse(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;
        
        return INV_PI * Frame::cosTheta(bRec.wo);
    }

    void sampleDiffuse(BSDFQueryRecord &bRec, const Point2f &sample) const
    {
        bRec.wo = Warp::squareToCosineHemisphere(sample);
    }


    /* Metal 
    *  Parameters:
    *   baseColor
    *   roughness
    *   anisotropic
    */
    Color3f evalMetal(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0)
            return Color3f(0.0f);
        Vector3f wi = bRec.wi;
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(wi, wo);

        return F_m_hat(bRec) * D_m(wh) * G_m(wi, wo) / (4 * Frame::cosTheta(wi) * Frame::cosTheta(wo));
    }

    float pdfMetal(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;

        Vector3f wi = bRec.wi;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        return D_m(wh) * G_w(bRec.wi) * std::max(0.f, wi.dot(wh)) / Frame::cosTheta(bRec.wi);
    }

    void sampleMetal(BSDFQueryRecord &bRec, const Point2f &sample) const
    {
        // Code adapted from https://jcgt.org/published/0007/04/01/slides.pdf
        // transforming the view direction to the hemisphere configuration
        Vector3f Vh(m_alpha_x * bRec.wi.x(), m_alpha_y * bRec.wi.y(), bRec.wi.z());
        Vh.normalize();

        // orthonormal basis
        // Eigen::cross deprecated, write a cross by hand
        Vector3f T0(0,0,1);
        Vector3f T1;
        if (Vh.z() < 0.9999)
            T1 = cross(T0, Vh);
        else
            T1 = Vector3f(1, 0, 0);
        T1.normalize();
        Vector3f T2 = cross(Vh, T1);

        // parameterization of the projected area
        float r = sqrt(sample.x());
        float phi = 2.f * M_PI * sample.y();
        float t1 = r * cos(phi);
        float t2 = r * sin(phi);
        float s = 0.5 * (1.f + Vh.z());
        t2 = (1.f - s) * sqrt(1.f - t1 * t1) + s*t2;

        // reprojection onto hemisphere
        Vector3f Nh = t1 * T1 + t2 * T2 + sqrt(std::max(0.f, 1.f - t1 * t1 - t2 * t2)) * Vh;

        // transforming the noraml back to the ellipsoid configuration
        Vector3f Ne = Vector3f(m_alpha_x * Nh.x(), m_alpha_y * Nh.y(), std::max(0.f, Nh.z()));
        Ne.normalize();

        bRec.wo = 2.f*(bRec.wi.dot(Ne)) * Ne - bRec.wi;
        bRec.wo.normalize();
    }


    /* Clearcoat 
    *   Parameters:
    *    clearcoatGloss
    */
    Color3f evalClearcoat(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return Color3f(0.0f);
        Vector3f wi = bRec.wi;
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(wi, wo);

        return F_c(bRec) * D_c(wh) * G_c(wi, wo) / (4 * abs(Frame::cosTheta(wi) * Frame::cosTheta(wo)));
    }

    float pdfClearcoat(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;

        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        return D_c(wh) * abs(Frame::cosTheta(wh));
    }

    void sampleClearcoat(BSDFQueryRecord &bRec, const Point2f &sample) const
    {
        float h_elevation = sqrt( (1 - pow(m_alpha_g * m_alpha_g, 2), 1 - sample.x()) / (1 - m_alpha_g * m_alpha_g));
        h_elevation = acos(clamp(h_elevation, -1.f, 1.f));
        float h_azimuth = 2 * M_PI * sample.y();
        Vector3f wh;
        wh.x() = sin(h_elevation) * cos(h_azimuth);
        wh.y() = sin(h_elevation) * sin(h_azimuth);
        wh.z() = cos(h_elevation);
        wh.normalize();
        bRec.wo = 2.f*(bRec.wi.dot(wh)) * wh - bRec.wi;
    }

    /* Sheen */
    Color3f evalSheen(const BSDFQueryRecord &bRec) const
    {
        Vector3f wo = bRec.wo;
        Vector3f wh = halfVec(bRec.wi, bRec.wo);
        float x = clamp(1 - abs(wh.dot(wo)), 0.f, 1.f);
        Color3f sheen = C_sheen(bRec) * pow(x, 5);

        return C_sheen(bRec) * pow(x, 5);
    }

    float pdfSheen(const BSDFQueryRecord &bRec) const
    {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;
        
        return INV_PI * Frame::cosTheta(bRec.wo);
    }

    /// Evaluate the BRDF model
    virtual Color3f eval(const BSDFQueryRecord &bRec) const override {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return Color3f(0.0f);

        Color3f principledBSDF(0.f);

        
        principledBSDF =  (1.f - m_metallic) * evalDiffuse(bRec) 
                        + (1.f - m_metallic) * m_sheen * evalSheen(bRec) 
                        + evalMetal(bRec)
                        + 0.25 * m_clearcoat * evalClearcoat(bRec);

        return principledBSDF;
    }

    /// Compute the density of \ref sample() wrt. solid angles
    virtual float pdf(const BSDFQueryRecord &bRec) const override {
        if (bRec.measure != ESolidAngle
            || Frame::cosTheta(bRec.wi) <= 0
            || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;

        float principledPDF;

        float diffuseWeight = 1.f - m_metallic;
        float metalWeight = 1.f;
        float clearcoatWeight = 0.25 * m_clearcoat;

        Vector3f pdf(diffuseWeight, metalWeight, clearcoatWeight);
        pdf.normalize();
        
        principledPDF =   pdf(0) * pdfDiffuse(bRec) 
                        + pdf(1) * pdfMetal(bRec) / (4.f * bRec.wo.dot(halfVec(bRec.wi, bRec.wo)));
                        + pdf(2) * pdfClearcoat(bRec) / (4.f * bRec.wo.dot(halfVec(bRec.wi, bRec.wo)));

        return principledPDF;
    }

    /// Draw a sample from the BRDF model
    virtual Color3f sample(BSDFQueryRecord &bRec, const Point2f &_sample) const override {
        if (Frame::cosTheta(bRec.wi) <= 0)
            return Color3f(0.0f);

        bRec.measure = ESolidAngle;

        float diffuseWeight = 1.f - m_metallic;
        float metalWeight = 1.f;
        float clearcoatWeight = 0.25 * m_clearcoat;

        // Construct cdf
        Vector3f cdf(diffuseWeight, metalWeight, clearcoatWeight);
        cdf /= cdf.sum();
        cdf.y() = cdf.x() + cdf.y();
        cdf.z() = cdf.y() + cdf.z();
        // cout << cdf << endl;

        Point2f sample = _sample;

        if (sample.x() < cdf.x()) // Diffuse
        {
            sample.x() /=  cdf.x();
            sampleDiffuse(bRec, sample);
        }
        else if(sample.x() < cdf.y()) // Metal
        {
            sample.x() = (sample.x() - cdf.x()) / (cdf.y() - cdf.x());
            sampleMetal(bRec, sample);
        }
        else // Clearcoat
        {
            sample.x() = (sample.x() - cdf.y()) / (1 - cdf.y());
            sampleClearcoat(bRec, sample);
        }

        float pdf_disney = pdf(bRec);

        if (pdf_disney > 0)
            return eval(bRec) / pdf_disney * std::max(Frame::cosTheta(bRec.wo), 0.f);
        else
            return Color3f(0.f);
    }

    virtual bool isDiffuse() const override {return true;}

    /// Return a human-readable summary
    virtual std::string toString() const override {
        return tfm::format(
            "Disney[\n"
            "  baseColor = %s,\n"
            "  subsurface = %f,\n"
            "  sheen = %f,\n"
            "  sheenTint = %f,\n"
            "  clearcoat = %f,\n"
            "  clearcoatGloss = %f,\n"
            "  specular = %f,\n"
            "  specTint= %f,\n"
            "  specTrans= %f,\n"
            "  roughness = %f,\n"
            "  anistropic = %f,\n"
            "  metallic = %f\n"
            "]",
            m_baseColor ? indent(m_baseColor->toString()) : std::string("null"),
            m_subsurface,
            m_sheen,
            m_sheenTint,
            m_clearcoat,
            m_clearcoatGloss,
            m_specular,
            m_specTint,
            m_specTrans,
            m_roughness,
            m_anisotropic,
            m_metallic
        );
    }

    virtual EClassType getClassType() const override { return EBSDF; }

private:
    Texture<Color3f> * m_baseColor;
    float m_subsurface;
    float m_sheen;
    float m_sheenTint;
    float m_clearcoat;
    float m_clearcoatGloss;
    float m_specular;
    float m_specTint;
    float m_specTrans;
    float m_roughness;
    float m_anisotropic;
    float m_metallic;
    float m_eta;

    // Secondary parameters
    float m_aspect;
    float m_alpha_min;
    float m_alpha_x;
    float m_alpha_y;
    float m_alpha_g;
};

NORI_REGISTER_CLASS(Disney, "disney");
NORI_NAMESPACE_END