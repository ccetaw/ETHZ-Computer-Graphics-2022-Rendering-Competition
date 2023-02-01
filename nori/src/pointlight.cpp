#include <nori/emitter.h>


NORI_NAMESPACE_BEGIN

class PointLight : public Emitter {
public:
    PointLight(const PropertyList &props) {
        position = props.getPoint3("position");
        power = props.getColor("power");
    }

    virtual Color3f sample(EmitterQueryRecord &lRec, const Point2f &sample) const override
    {
        lRec.p = position;
        lRec.wi = (lRec.p - lRec.ref).normalized();
        lRec.shadowRay = Ray3f(lRec.ref, lRec.wi, Epsilon, (lRec.ref - lRec.p).norm()-Epsilon, this->getMedium());
        return eval(lRec) / pdf(lRec);
    }

    virtual Color3f eval(const EmitterQueryRecord &lRec) const override
    {
        return power * INV_FOURPI / (lRec.p - lRec.ref).squaredNorm();
    }

    virtual float pdf(const EmitterQueryRecord &lRec) const override
    {
        return 1.f;
    }

    std::string toString() const {
        return tfm::format("PointLight[position = {%f, %f, %f}, \n power = {%f, %f, %f} ,\n"
                           "medium = %s\n"
                            "]", 
                            position.x(), position.y(), position.z(), power.x(), power.y(), power.z(),
                            m_medium ? indent(m_medium->toString()) : std::string("null")
                            );
    }

private:
    Point3f position;
    Color3f power;
};

NORI_REGISTER_CLASS(PointLight, "point");
NORI_NAMESPACE_END