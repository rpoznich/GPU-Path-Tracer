# Minecraft++

**Ryan Poznich (rjp2433)**
**David Vu (dvh254)**

**A non-trivial program mimicking Minecraft**

# Running The Code

To run the code run the following commands on the lab machines:

1. mkdir build
2. cd build
3. cmake .. 
    * or for the optimized build 

    * cmake -DCMAKE_BUILD_TYPE=Release ..

4. make -j8 && ./bin/minecraft

Press 'C' to toggle movement on and off

# Implemented Features

Some of the implemented features include:

* Bidirectional photon mapping

    This creates multiple effects
    
        1. Global illumination which allows for color bleeding
        2. More realistic lighting
        3. More effecient photon mapping
        4. Soft shadows

* Semi real time rendering

    In order to make the rendering be real time, we knew that a minecraft type world would be ideal because
    
        1. Everything is static
        2. All intersections are ray cube intersections
        3. It's easy to cache the photon map so you don't have to continuosly shoot photons after the initial process of photon scattering
        4. It's easy to do a lot of culling for which cubes actually need to be ray traced

* Ray Tracing on the GPU

    In order to make the process more efficient we did the ray tracing in a compute shader, and wrote the pixel colors to a texture
    
# Extra Credit

 **Both Ryan and David completed the online course instructor survey**

# Resources Used

[1] https://github.com/LWJGL/lwjgl3-wiki/wiki/2.6.1.-Ray-tracing-with-OpenGL-Compute-Shaders-%28Part-I%29

[2] https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl