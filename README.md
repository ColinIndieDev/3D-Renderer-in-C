# 3D CPU Renderer
## About
This is a CPU Renderer inspired by how OpenGL works written in pure C using minimal dependencies. 
This way it can be applied and used anywhere like on a window with GLFW or even by treating each pixel as a character
on the terminal. Note that it is not entirely optimized or uses the GPU at any time therefore it is really slow.

But it is cool to render 3D graphics without needing a GPU and also to see how a graphical api looks like behind the scenes.
It is also possible to write a „shader“ but unlike in OpenGL with glsl, the fragment and vertex shader are C functions
where you need to pass the function pointer to the shader of the renderer which can be bound and used for rendering obviously
