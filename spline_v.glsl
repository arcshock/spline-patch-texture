// bpfrag_v.glsl
// Glenn G. Chappell
// 18 Oct 2013
//
// For CS 381 Fall 2013
// GLSL Vertex Shader for use with useshaders.cpp
// Blinn-Phong illumination model, per-fragment lighting


varying vec3 surfpt;       // Point on surface (camera coords)
varying vec3 surfnorm_un;  // Surface normal (camera coords)
                           // Normalize before & after sending


void main()
{
    // Compute projected vertex position as usual
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    // Send paint color to fragment shader
    gl_FrontColor = gl_Color;

    // Find object position (camera coords)
    vec4 surfpt4 = gl_ModelViewMatrix * gl_Vertex;
    surfpt = surfpt4.xyz / surfpt4.w;

    // Transform normal vector and ensure it has length 1
    surfnorm_un = normalize(gl_NormalMatrix * gl_Normal);
}

