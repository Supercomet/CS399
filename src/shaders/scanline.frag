#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

#include "shared_structs.h"

const vec3 ambientIntensity = vec3(0.2);

layout(push_constant) uniform _PushConstantRaster
{
  PushConstantRaster pcRaster;
};

// clang-format off
// Incoming 
layout(location=1) in vec3 worldPos;
layout(location=2) in vec3 worldNrm;
layout(location=3) in vec3 viewDir;
layout(location=4) in vec2 texCoord;
// Outgoing
layout(location = 0) out vec4 fragColor;

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; };    // Positions of an object
layout(buffer_reference, scalar) buffer Indices {uint i[]; };       // Triangle indices
layout(buffer_reference, scalar) buffer Materials {Material m[]; }; // Array of materials
layout(buffer_reference, scalar) buffer MatIndices {int i[]; };     // Material ID for each triangle

layout(binding=eObjDescs, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;
layout(binding=eTextures) uniform sampler2D[] textureSamplers;

float pi = 3.14159;

float approx_tan(vec3 V, vec3 N)
{
    float VN = dot(V,N);
    return sqrt( 1.0-pow(VN,2) )/VN;
}

float G_SOSS_Approximaton(vec3 L, vec3 H)
{
    return 1.0/ pow(dot(L,H),2);
}

float G_Phong_Beckman_impl(float a)
{
    if(a < 1.6) return 1.0;

    float aa= pow(a,2);
    return (3.535*a + 2.181*aa)/(1.0+2.276*a+ 2.577*aa);
}

float G_Phong(vec3 V, vec3 H, float a)
{
    float vTan = approx_tan(V,H);
    if(vTan == 0.0) return 1.0;
    float alpha = sqrt(a/2+1.0) / vTan;
    return G_Phong_Beckman_impl(alpha);
}

float G_Beckman(vec3 V, vec3 H, float a){
    float vTan = approx_tan(V,H);
    if(vTan == 0.0) return 1.0;
    float alpha = 1.0/(a*vTan);
    return G_Phong_Beckman_impl(alpha);
}

float G_GGX(vec3 V, vec3 M , float a)
{
    float alphaG = sqrt(2.0/ (a+2));

    float vTan = approx_tan(V,M);

    return 2.0/ (1.0+ sqrt(1+alphaG*alphaG*vTan*vTan));
}


float D_Phong(vec3 N, vec3 H, float alpha)
{
    return (alpha+2.0)/ (2*pi) * pow(dot(N,H), alpha);
}

float D_Beckham(vec3 N, vec3 H, float alpha)
{
    float alphaB = sqrt(2.0/ (alpha+2));
    float aBaB = alphaB*alphaB;
    float vTan = approx_tan(H,N);
    float NdotH = dot(N,H);

    return 1.0/ (pi*aBaB * NdotH*NdotH) * exp(-vTan*vTan/aBaB);
}

float D_GGX(vec3 N, vec3 H , float alpha)
{
    float alphaG = sqrt(2.0/ (alpha+2));
    float aGaG = pow(alphaG,2);
    float invAG = pow(alphaG-1.0,2);
    float NdotH = dot(N,H);

    return aGaG / (pi*pow( NdotH*NdotH*(aGaG-1.0) + 1.0  , 2));
}

vec3 PhongBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
    
    float D = D_Phong(N,H,alpha); // Ditribution
    vec3 F = Ks + (1.0-Ks) * pow(1-LH,5); // Fresnel approximation
    float G = G_Phong(V,H,alpha)* G_Phong(L,H,alpha); // Self-occluding self-shadowing
    
    return Kd/pi + D*F/4.0 * G;
}

vec3 GGXBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
    
    float D = D_GGX(N,H,alpha);
    vec3 F = Ks + (1.0-Ks) * pow(1-LH,5);
    float G = G_GGX(L,H,alpha) * G_GGX(V,H,alpha);
    
    return Kd/pi + D*F/4.0 * G;
}

vec3 BeckhamBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
    
    float D = D_Beckham(N,H,alpha);
    vec3 F = Ks + (1.0-Ks) * pow(1-LH,5);
    float G = G_Beckman(L,H,alpha) * G_Beckman(V,H,alpha);
    
    return Kd/pi + D*F/4.0 * G;
}

void main()
{
  // Material of the object
  ObjDesc    obj = objDesc.i[pcRaster.objIndex];
  MatIndices matIndices  = MatIndices(obj.materialIndexAddress);
  Materials  materials   = Materials(obj.materialAddress);
  
  int               matIndex = matIndices.i[gl_PrimitiveID];
  Material mat      = materials.m[matIndex];
  
  vec3 N = normalize(worldNrm);
  vec3 V = normalize(viewDir);
  vec3 lDir = pcRaster.lightPosition - worldPos;
  vec3 L = normalize(lDir);

  float NL = max(dot(N,L),0.0);

  vec3 H = normalize(L+V);
  
  vec3 Kd = mat.diffuse;
  vec3 Ks = mat.specular;
  const float alpha = mat.shininess;
  
  if (mat.textureId >= 0)
  {
    int  txtOffset  = obj.txtOffset;
    uint txtId      = txtOffset + mat.textureId;
    Kd = texture(textureSamplers[nonuniformEXT(txtId)], texCoord).xyz;
  }
  
  vec3 ambient = vec3(0.005);

  // This very minimal lighting calculation should be replaced with a modern BRDF calculation. 
  //fragColor.xyz = pcRaster.lightIntensity*NL*Kd/pi;
  fragColor.xyz = ambient+pcRaster.lightIntensity*NL*GGXBRDF(L,V,H,N,alpha,Kd,Ks);

}
