/*
Author: Jingyu Wang, ETH Zurich Data Science Master student
*/
#include <nori/texture.h>
#include <filesystem/resolver.h>
#include <lodepng.h>

NORI_NAMESPACE_BEGIN

class ImageTexture : public Texture<Color3f> {
public:
    ImageTexture(const PropertyList &props);

    virtual std::string toString() const override;

    virtual Color3f eval(const Point2f & uv) override;

    float inverseGammaCorrect(float value)
    {
        if (value <= 0.04045f)
        return value * 1.f / 12.92f;
        return std::pow((value + 0.055f) * 1.f / 1.055f, 2.4f);
    }

    Color3f pixel(int i, int j)
    {
        // Clamp the boundary
        if (i < 0) i = 0;
        if (i > m_size.x() - 1) i = m_size.x() - 1;
        if (j < 0) j = 0;
        if (j > m_size.y() - 1) j = m_size.y() - 1;

        long index = ((m_size.y() - 1 - j) * m_size.x() + i )*4; // Something wroing with the uv coordinate system, you need to flip around the u axis
        if (index > m_image.size())
            throw NoriException("Out of range");
        float r = inverseGammaCorrect(m_image[index] / 255.0);
        float g = inverseGammaCorrect(m_image[index + 1] / 255.0);
        float b = inverseGammaCorrect(m_image[index + 2] / 255.0);
        if (r < 0 || g < 0 || b < 0 || r > 1 || g > 1 || b > 1)
            throw NoriException("Invalid color");
        
        return Color3f(r, g, b);
    }

    // Map outbound uv coordinates to (width, height)
    Point2f remap(float u, float v)
    {
        if (m_extension == "clamp")
        {
            if (u < 0)                      u = 0;
            else if (u > m_size.x())        u = m_size.x();
            
            if (v < 0)                      v = 0;
            else if (v > m_size.y())        v = m_size.y();
        }
        else if (m_extension == "repeat")  
        {
            u -= floor(u/m_size.x()) * m_size.x();
            v -= floor(v/m_size.y()) * m_size.y();
        }

        return Point2f(u, v);
    }
    
    Color3f bilerp(float x_weight, float y_weight, Color3f bot_left, Color3f  bot_right, Color3f  top_left, Color3f  top_right)
    {
        Color3f a1 = (1 - x_weight) * (1 - y_weight) * bot_left;
        Color3f a2 = x_weight * (1 - y_weight) * bot_right;
        Color3f a3 = (1 - x_weight) * y_weight * top_left;
        Color3f a4 = x_weight * y_weight * top_right;

        return a1+a2+a3+a4;
    }

protected:
    std::string m_file;
    float m_scale;
    Vector2i m_size;
    std::string m_interpolation;
    std::string m_extension;
    std::vector<unsigned char> m_image;
};

ImageTexture::ImageTexture(const PropertyList &props) {
    // Extract parameters
    m_file = props.getString("filename", "");
    m_scale = props.getFloat("scale", 1.f);
    m_interpolation = props.getString("interpolation", "bilinear");
    m_extension = props.getString("extension", "repeat");

    if (m_extension != "repeat" && m_extension != "clamp")      
        throw NoriException("Invalid wrap mode \"%s\", must be one of: \"repeat\", "
                            ", or \"clamp\"!", m_extension);

    if (m_interpolation != "bilinear" && m_interpolation != "nearest")      
        throw NoriException("Invalid filter type \"%s\", must be one of: \"nearest\", or "
                            "\"bilinear\"!", m_interpolation);

    // Load the .png file
    filesystem::path filePath = getFileResolver()->resolve(m_file);
    unsigned width, height;
    unsigned error = lodepng::decode(m_image, width, height, filePath.str());
    if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    
    // For information pringting
    m_size = Vector2i(width, height);
}

Color3f ImageTexture::eval(const Point2f & uv) {
    if (m_interpolation == "nearest")
    {
        Point2f uv_aug = remap(uv.x()*m_size.x() / m_scale, uv.y()*m_size.y() / m_scale);
        // Find the nearest pixel
        int i = round(uv_aug.x() - 0.5);
        int j = round(uv_aug.y() - 0.5);

        return pixel(i, j);
    }
    else if (m_interpolation == "bilinear")
    {
        Point2f uv_aug = remap(uv.x()*m_size.x() / m_scale, uv.y()*m_size.y() / m_scale);
        // Find the top-left pixel
        int i = int(uv_aug.x() + 0.5) - 1;
        int j = int(uv_aug.y() + 0.5) - 1;
        float x_weight = uv_aug.x() - 0.5 - i;
        float y_weight = uv_aug.y() - 0.5 - j;

        return bilerp(x_weight, y_weight, pixel(i, j), pixel(i+1, j), pixel(i, j+1), pixel(i+1, j+1));
    }
}

std::string ImageTexture::toString() const {
    return tfm::format(
        "ImageTexture[\n"
                "  file = %s,\n"
                "  size = %s,\n"
                "  scale = %d,\n"
                "  interpolation = %s,\n"
                "  extension = %s,\n"
                "]",
        m_file,
        m_scale,
        m_size.toString(),
        m_interpolation,
        m_extension
    );
}

NORI_REGISTER_CLASS(ImageTexture, "image_texture")
NORI_NAMESPACE_END
