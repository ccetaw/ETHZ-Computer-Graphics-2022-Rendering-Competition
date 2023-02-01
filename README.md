# Computer Graphics ETHz 2022 Rendering Project

*Name: Jingyu Wang*  
*Legi: 21-956-453*

Theme of this year:  
Many things have their dedicated place to be. For this competition, we want to escape the ordinary and allow everything to be "out of place".

## Introduction

### Inspirational Image

![borja-helix-ferrandez-lfw-tres](/attachments/borja-helix-ferrandez-lfw-tres.jpg)
[ArtStation - She's Gone - UE4 Fish Tank, Borja "Helix" Ferrandez](/attachments/https://www.artstation.com/artwork/w6BmDZ)

![xiangzhao-xi-3840](/attachments/xiangzhao-xi-3840.jpg)
[ArtStation - Ruins in the water, Xiangzhao Xi](/attachments/https://www.artstation.com/artwork/eJKEbZ)


### Initial Design

Some people like to have a fish tank in their house. In this design, every thing inside the fish tank is scaled out and every thing outside is put into the fish tank. In the initial imagination, a tank floats on the water under the sky. A batch of clouds float in the sky and fishes swim in the water. A room is scaled into the tank with its furniture and possibly other light sources and human characters.

![IMG_0073](/attachments/initial_desgin.jpg)

Noticeable technical aspects:
- Environment map of the sky
- Volumetric rendering for the clouds and water, if not suitable models could be found, clouds will be included in the environment map
- Disney BSDF for the furniture in the tank and stones in the water
- Image as textures for the room floor and ocean floor

### Features Selected

- Homogeneous Participating Media (15 points)
- Disney BSDF (15 points)
	- subsurface
	- metallic
	- specular
	- sheen
	- clearcoat
- Environment Map Emitter (15 points)
- Image as Textures (5 points)
- Probabilistic Progressive Photon Mapping (5 points)
- Depth of Field (5 points)


## Implementation & Validation

### Preliminary: How to use the same scene for validation in both `nori` and `mitsuba`

To make an identical scene for both renderers, we use blender as a bridge. There exists plugins for both renders thus we could build a scene in blender and then export it to nori and mitsuba format. 

From **`blender`** to **`nori`**
[Phil26AT/BlenderNoriPlugin: Export blender scenes to the Nori educational raytracer. Proposed and used in the Computer Graphics course at ETH Zurich, Fall 2020 (github.com)](/attachments/https://github.com/Phil26AT/BlenderNoriPlugin) 

Problems:
- Texture .png files are not exported, even enabled this setting.
- Obj normals might direct inward. Use **`meshlab`** to flip the normals. You can first view the obj in windows 3D viewer to see where the normals are.

From **`blender`** to **`mitsuba`**
[mitsuba-renderer/mitsuba-blender: Mitsuba integration add-on for Blender (github.com)](/attachments/https://github.com/mitsuba-renderer/mitsuba-blender)

Problems:
- Object may be classified as `two-sided`
- A constant emitter may be added automatically

You could download scenes from [Blend Swap | Home](/attachments/https://blendswap.com/) and directly export to **`nori`** and **`mitsuba`** .xml format which could save you some time.

Note that sometimes you need to manually rectify some errors but there's essentially little work to do. The procedure is very simple. Just replace all BSDF to diffuse and remove seemingly extra terms.

Besides, you might need to tune the path integrator in **`nori`** and **`mitsuba`** so that they give almost the same output for a simple scene, something like this in **`tev`**:
![CG Log-5](/attachments/CG%20Log-5.png)



### Homogeneous Participating Media

File added

```
include/nori/medium.h
include/nori/phasefunction.h
src/henyey_greeenstein.cpp
src/homogeneous.cpp
src/transluscent.cpp
src/volpath.cpp
```

File touched

```
CMakeLists.txt
include/nori/common.h
include/nori/camera.h
include/nori/emitter.h
include/nori/object.h
include/nori/ray.h
include/nori/shape.h
include/nori/bsdf.h
src/shape.cpp
src/sphere.cpp
src/arealight.cpp
src/dof_camera.cpp
src/mesh.cpp
src/perspective.cpp
src/pointlight.cpp
src/inftyarealight.cpp
src/mirror.cpp
```

This is the most difficult part of the project. The entire rendering pipeline is changed. Following the PBRT book, a medium class is created and attached to emitter, shape, camera and ray. Emitter, camera and ray are designated to one medium and shape serves as a boundary of two medium. When no boundary is specified, the scene is by default in vacuum. A special translucent BSDF is implemented to serve as invisible boundary. This BSDF is only valid for path integrator or photonmapper. A special integrator `volpath.cpp` is implemented. Only with this integrator can we render participating media. Using other integrators, the media is not visible. 

When a ray travels in a medium, it could be scattered or absorbed by the medium. (Possibly enhanced by radiance from other directions or the medium is itself a emitter, but there are not modeled in the implementation). We follow the same strategy used in `path_mis`:
A path is sampled, and at each vertex of this path we do emitter sampling and BSDF sampling. 
This time a path contains both volumetric vertex and surface vertex. The medium class decides which type of interaction happens and the phase function class decides which direction the ray is scattered to when a medium interaction is sampled. Whichever interaction is sampled, the attenuation of the ray must be taken into account. 

**Result:**

Global participating medium
![AnyConv.com__vol_global](/attachments/AnyConv.com__vol_global.png)

Smoke confined in a dielectric and translucent surface
![AnyConv.com__vol_confined](/attachments/AnyConv.com__vol_confined.png)

**Remark:**
A new ray is generated for the next iteration. Its `maxt` is usually set to be `INFINITY` but remember to reset this value when an intersection is found in the next iteration. Otherwise a medium interaction will always be sampled. 

### Disney BSDF

File added

```
src/disney.cpp
src/twosided.cpp
```

File touched

```
CMakeLists.txt
src/path_mis.cpp
src/direct_mis.cpp
```

A strict implementation following [UCSD CSE 272 Assignment 1: Disney Principled BSDF](/attachments/https://cseweb.ucsd.edu/~tzli/cse272/homework1.pdf). Implemented parameters:

```cpp
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
```

|            | 0.0                                            | 0.2                                            | 0.4                                            | 0.6                                              | 0.8                                              | 1.0                                            |
| ---------- | ---------------------------------------------- | ---------------------------------------------- | ---------------------------------------------- | ------------------------------------------------ | ------------------------------------------------ | ---------------------------------------------- |
| subsurface | ![AnyConv.com__disney_nori_subsurface00](/attachments/AnyConv.com__disney_nori_subsurface00.png) | ![AnyConv.com__disney_nori_subsurface02](/attachments/AnyConv.com__disney_nori_subsurface02.png) | ![AnyConv.com__disney_nori_subsurface04](/attachments/AnyConv.com__disney_nori_subsurface04.png) | ![AnyConv.com__disney_nori_subsurface06](/attachments/AnyConv.com__disney_nori_subsurface06.png) | ![AnyConv.com__disney_nori_subsurface08](/attachments/AnyConv.com__disney_nori_subsurface08.png) | ![AnyConv.com__disney_nori_subsurface10](/attachments/AnyConv.com__disney_nori_subsurface10.png) |
| metallic   | ![AnyConv.com__disney_nori_metallic00](/attachments/AnyConv.com__disney_nori_metallic00.png)   | ![AnyConv.com__disney_nori_metallic02](/attachments/AnyConv.com__disney_nori_metallic02.png)   | ![AnyConv.com__disney_nori_metallic04](/attachments/AnyConv.com__disney_nori_metallic04.png)   | ![AnyConv.com__disney_nori_metallic06](/attachments/AnyConv.com__disney_nori_metallic06.png)     | ![AnyConv.com__disney_nori_metallic08](/attachments/AnyConv.com__disney_nori_metallic08.png)     | ![AnyConv.com__disney_nori_metallic10](/attachments/AnyConv.com__disney_nori_metallic10.png)   |
| specular   | ![AnyConv.com__disney_nori_specular00](/attachments/AnyConv.com__disney_nori_specular00.png)   | ![AnyConv.com__disney_nori_specular02](/attachments/AnyConv.com__disney_nori_specular02.png)   | ![AnyConv.com__disney_nori_specular04](/attachments/AnyConv.com__disney_nori_specular04.png)   | ![AnyConv.com__disney_nori_specular06](/attachments/AnyConv.com__disney_nori_specular06.png)     | ![AnyConv.com__disney_nori_specular08](/attachments/AnyConv.com__disney_nori_specular08.png)     | ![AnyConv.com__disney_nori_specular10](/attachments/AnyConv.com__disney_nori_specular10.png)   |
| sheen      | ![AnyConv.com__disney_nori_sheen00](/attachments/AnyConv.com__disney_nori_sheen00.png)      | ![AnyConv.com__disney_nori_sheen02](/attachments/AnyConv.com__disney_nori_sheen02.png)      | ![AnyConv.com__disney_nori_sheen04](/attachments/AnyConv.com__disney_nori_sheen04.png)      | ![AnyConv.com__disney_nori_sheen06](/attachments/AnyConv.com__disney_nori_sheen06.png)        | ![AnyConv.com__disney_nori_sheen08](/attachments/AnyConv.com__disney_nori_sheen08.png)        | ![AnyConv.com__disney_nori_sheen10](/attachments/AnyConv.com__disney_nori_sheen10.png)      |
| clearcoat  | ![AnyConv.com__disney_nori_clearcoat00](/attachments/AnyConv.com__disney_nori_clearcoat00.png)  | ![AnyConv.com__disney_nori_clearcoat02](/attachments/AnyConv.com__disney_nori_clearcoat02.png)  | ![AnyConv.com__disney_nori_clearcoat04](/attachments/AnyConv.com__disney_nori_clearcoat04.png)  | ![AnyConv.com__disney_nori_clearcoat06 1](/attachments/AnyConv.com__disney_nori_clearcoat06%201.png)  | ![AnyConv.com__disney_nori_clearcoat08 1](/attachments/AnyConv.com__disney_nori_clearcoat08%201.png)  | ![AnyConv.com__disney_nori_clearcoat10 1](/attachments/AnyConv.com__disney_nori_clearcoat10%201.png)                                               |

### Environment Map Emitter

File added

```
src/inftyarealight.cpp
```


File touched

```
CMakeLists.txt
include/nori/integrator.h
include/nori/emitter.h
src/pppm.cpp
src/photonmapper.cpp
src/path_mis.cpp
src/path_mats.cpp
src/direct_mis.cpp
src/direct_mats.cpp
src/direct_ems.cpp
```

The PBRT book gives a nice example for the process. Here a simpler implementation using nearest neighbor strategy is given:
1. Load the .exr file into a bitmap which is essentially an Eigne array. 
   The loading infrastructure already exists in nori, find it in `hdrToLdr.cpp`. 
2. Precompute the pdf as indicated in lecture slides. Use Eigen matrix and vector to store the pdf information. 
   Use Eigen library use greatly simplify the process. Check it here [Eigen: Getting started](/attachments/https://eigen.tuxfamily.org/dox/GettingStarted.html).
3. Mapping a direction to the spherical coordinates and use nearest neighbor interpolation to find the pixel.
   Note that the PBRT book uses bilinear interpolation which is usually a better interpolation strategy. But it will be tedious to deal with the probability of this interpolation. Thus the nearest neighbor interpolation is used so that importance sampling is much simpler. 

**Results:**

| nori                                   | mitsuba |
| -------------------------------------- | ------- |
| ![300](/attachments/AnyConv.com__envmap_nori.png) | ![300](/attachments/AnyConv.com__envmap_mitsuba.png)        |

**Remarks:**
There are some slight differences which might because of nearest neighbor interpolation and bilinear interpolation. All integrators are changed to incorporate environment mapping. 

### Image as Textures

File added

```
include/lodepng.h
src/imagetexture.cpp
src/lodepng.cpp
```

File touched

```
CMakeLists.txt
src/direct_mis.cpp
src/path_mis.cpp
```

.obj and .ply files already store the uv coordinates thus we don't need to worry about the process of creating uv coordinates for the mesh. 

1. Load .png files 
   There is a powerful opensource repo on GitHub [lvandeve/lodepng: PNG encoder and decoder in C and C++. (github.com)](/attachments/https://github.com/lvandeve/lodepng) which provides the utility. Simply look into the examples it provides and you will quickly learn how to use it.

Another important thing is that .png files are gamma-corrected. You need to undo gamma correction to get the true color. Example code 

``` cpp
float inverseGammaCorrect(float value)
{
	if (value <= 0.04045f)
		return value * 1.f / 12.92f;
	return std::pow((value + 0.055f) * 1.f / 1.055f, 2.4f);
}
```

2. Map the uv coordinates to pixel coordinates
   First of all, assume all uv coordinates lie in $[0,1]$. Simple multiply each by the width and height respectively we are in pixel field. After mapping to width and height, two interpolations are implemented:
	- Nearest neighbor 
	- Bilinear
	Bilinear will usually give better results but if you use high resolution pictures, the difference could be neglected.

3. Deal with uv coordinates out of bound / Scaling
If there are uv coordinates out of bound $[0, 1]$ accidentally or we want to scale the picture, we could either use repeat or clamp strategy. Both are implemented.

**Results:**

nori / mitsuba

| nori | mitsuba |
| ---- | ------- |
| ![](/attachments/AnyConv.com__texture_nori.png)     | ![](/attachments/AnyConv.com__texture_mitsuba.png)        |

scaling

| shrink | expand |
| ------ | ------ |
| ![](/attachments/AnyConv.com__texture_nori_scale05.png)       | ![](/attachments/AnyConv.com__texture_nori_scale2.png)       |

**Remarks:**
Check the integrators if they correctly set the uv coordinates in BSDFQueryRecord. 

### Probabilistic Progressive Photon Mapping

File added
```
src/pppm.cpp
```

File touched
```
CMakeLists.txt
include/nori/integrator.h
src/photonmapper.cpp
src/render.cpp
```

To implement this feature, we need to touch the rendering pipeline i.e. `render.cpp` and `integrtor.h` so that you are allowed to render multiple images. 

The algorithm is simple, we only need to add an outer loop to the main rendering loop. You may need an additional parameter in .xml files to specify the number of iterations. The output of rendering is also changed so that when you have multiple image outputs, nori will create a directory for you. 

**Result:**

| parameters                                                           | results                            |
| -------------------------------------------------------------------- | ---------------------------------- |
| - 400 iterations <br> - 250000 photons <br> - 32 spp <br> - 17.6 min | ![500](/attachments/AnyConv.com__pppm400.png) |
| - 1 iterations <br> - 10000000 photons <br> - 512 spp <br> - 1.7 min | ![500](/attachments/photonmapper.png)                                   |


| Iteration | Image                           |
| ---------- | ------------------------------- |
| 1          | ![AnyConv.com__pppm1](/attachments/AnyConv.com__pppm1.png)     |
| 50         | ![AnyConv.com__pppm50](/attachments/AnyConv.com__pppm50.png)    |
| 100        | ![AnyConv.com__pppm100](/attachments/AnyConv.com__pppm100.png)   |
| 400        | ![AnyConv.com__pppm400 1](/attachments/AnyConv.com__pppm400%201.png) | 

**Remark:**
The sampler in the integrator could not be reset, otherwise it will always sample the same point for every image.

### Depth of Field

File added
```
src/dof_camera.cpp
```

File touched
```
CMakeLists.txt
```

This feature is fairly simple, following [Projective Camera Models (pbr-book.org)](/attachments/https://www.pbr-book.org/3ed-2018/Camera_Models/Projective_Camera_Models) or course slides, it could be easily implemented.

A problem for validation is that, parameters of lens for cameras in nori are provided in local coordinates while in mitsuba they are in world coordinates. This makes validation quite difficult. 

**Results:**

| nori | mitsuba |
| ---- | ------- |
| ![](/attachments/AnyConv.com__dof_nori.png)     | ![](/attachments/AnyConv.com__dof_mitsuba.png)        |

### Additional Features

#### Transluscent BSDF

This class of BSDF serves as an invisible boundary for homogeneous participating medium. Only path integrator or photonmapper will show the invisible effect.

#### Twosided BSDF

This class of BSDF is designed to be visible on both sides so that you don't need to care about the normals, since sometimes the output of blender may not have the correct normals as you wish.

#### Bubble BSDF
![](/attachments/bubble.png)
This class of BSDF models thin dielectric, mainly bubbles. Since my scene contains bubbles this class is very useful.

#### Transmittance in Homogeneous Media (Beer’s Law)

#### Henyey-Greenstein Phase Function

#### Textured Dielectric

Now dielectric material could have texture.

## Final Image
The image showed here consists 72 shapes and about 150000 primitives. It's rendered using 512 spp to a 1920x1080 HD image.

The image is essentially a duality of fish tank in a room, i.e. the room is now in a tank floating on the sea. A girl and a corgi are confined in the tank and fishes swim around them. You could also see corals and starfishes on the seafloor. 

Technical details:
- The sky is an environment emitter using a 4k exr image. 
- The tank is a bubble(thin dielectric) BSDF.
- The tank is filled with homogeneous participating media to make the sunset effect more impressive.
- Every living creatures have their own texture on diffuse or Disney BSDF.
- The sea also contains homogeneous participating media. You could see the effect clearly near the tank. And the coral and starfish are almost invisible due to attenuation of the radiance.

Final image
![fishtank](/attachments/fishtank.png)

## Resources 
[Dartmouth rendering competition](/attachments/https://www.cs.dartmouth.edu/~rendering-competition/)

[ArtStation - She's Gone - UE4 Fish Tank, Borja "Helix" Ferrandez](/attachments/https://www.artstation.com/artwork/w6BmDZ)

[ArtStation - Ruins in the water, Xiangzhao Xi](/attachments/https://www.artstation.com/artwork/eJKEbZ)

[Physically Based Shading At Disney (disneyanimation.com)](/attachments/https://www.disneyanimation.com/publications/physically-based-shading-at-disney/)

[UCSD CSE 272 Assignment 1: Disney Principled BSDF](/attachments/https://cseweb.ucsd.edu/~tzli/cse272/homework1.pdf)

[Raytracing - UV Mapping and Texturing | 1000 Forms of Bunnies (viclw17.github.io)](/attachments/https://viclw17.github.io/2019/04/12/raytracing-uv-mapping-and-texturing)

[lvandeve/lodepng: PNG encoder and decoder in C and C++. (github.com)](/attachments/https://github.com/lvandeve/lodepng)

[Progressive Photon Mapping: A Probabilistic Approach](/attachments/https://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Knaus11.pdf)

[Infinite Area Lights (pbr-book.org)](/attachments/https://www.pbr-book.org/3ed-2018/Light_Sources/Infinite_Area_Lights)

[Sampling Light Sources (pbr-book.org)](/attachments/https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Sampling_Light_Sources#InfiniteAreaLight::Sample_Li)

[Rendering the Moana Island Scene Part 1: Implementing the Disney BSDF (schuttejoe.github.io)](/attachments/https://schuttejoe.github.io/post/disneybsdf/)
[Volume Scattering (pbr-book.org)](/attachments/https://www.pbr-book.org/3ed-2018/Volume_Scattering)

[Light Transport II: Volume Rendering (pbr-book.org)](/attachments/https://www.pbr-book.org/3ed-2018/Light_Transport_II_Volume_Rendering)

Model credit to:
[Blend Swap | CGTrace's Material Ball](/attachments/https://blendswap.com/blend/11511)
[Mitsuba Material Ball](/attachments/https://github.com/mitsuba-renderer/mitsuba3)

Environment credit to:
[Limpopo Golf Course HDRI • Poly Haven](/attachments/https://polyhaven.com/a/limpopo_golf_course)
