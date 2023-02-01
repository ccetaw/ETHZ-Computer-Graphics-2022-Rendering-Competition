# Competition Preview
The image showed here consists 72 shapes and about 150000 primitives. It's rendered using 512 spp to a 1920x1080 HD image.

The image is essentially a duality of fish tank in a room, i.e. the room is now in a tank floating on the sea. A girl and a corgi are confined in the tank and fishes swim around them. You could also see corals and starfishes on the seafloor. 

Technical details:
- The sky is an environment emitter using a 4k exr image. 
- The tank is a bubble(thin dielectric) BSDF.
- The tank is filled with homogeneous participating media to make the sunset effect more impressive.
- Every living creatures have their own texture on diffuse or Disney BSDF.
- The sea also contains homogeneous participating media. You could see the effect clearly near the tank. And the coral and starfish are almost invisible due to attenuation of the radiance.

![](/attachments/fishtank.png)
