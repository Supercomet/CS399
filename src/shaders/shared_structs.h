
#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>
// GLSL Type
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

#ifdef __cplusplus
#define BOOL(name) bool name; bool pad##name[3] 
#else
#define BOOL(name) bool name
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
 #define START_ENUM(a) enum a {
 #define END_ENUM() }
#else
 #define START_ENUM(a)  const uint
 #define END_ENUM() 
#endif

START_ENUM(ScBindings)
  eMatrices  = 0,  // Global uniform containing camera matrices
  eObjDescs = 1,  // Access to the object descriptions
  eTextures = 2   // Access to textures
END_ENUM();

START_ENUM(RtBindings)
  eTlas     = 0,  // Top-level acceleration structure
  eOutCurrImage = 1,   // Ray tracer output image
  eLights = 2,   // Access to lights
  eColorHistoryImage = 2,
  eOutPrevImage = 3,
  eOutCurrNd = 4,
  eOutPrevNd = 5,
  eOutCurrKd = 6
END_ENUM();
// clang-format on



// Information of a obj model when referenced in a shader
struct ObjDesc
{
  int      txtOffset;             // Texture index offset in the array of textures
  uint64_t vertexAddress;         // Address of the Vertex buffer
  uint64_t indexAddress;          // Address of the index buffer
  uint64_t materialAddress;       // Address of the material buffer
  uint64_t materialIndexAddress;  // Address of the triangle material index buffer
};

// Uniform buffer set at each frame
struct MatrixUniforms
{
  mat4 viewProj;     // Camera view * projection
  mat4 priorViewProj;     // Camera view * projection
  mat4 viewInverse;  // Camera inverse view matrix
  mat4 projInverse;  // Camera inverse projection matrix
};

// Push constant structure for the raster
struct PushConstantRaster
{
  mat4  modelMatrix;  // matrix of the instance
  vec3  lightPosition;
  uint  objIndex;
  float lightIntensity;
  int   lightType;
  int	BRDF;
};


// Push constant structure for the ray tracer
struct PushConstantRay
{
  int   frameSeed;
  int   depth;
  float rr;
  int   moved;
  int   frame;
  BOOL(historyView);
  BOOL(ExplicitLightRays);
  float posTolerance;
  float n_threshold;
  float d_threshold;
  BOOL(useHistory);
};

struct Vertex  // Created by readModel; used in shaders
{
  vec3 pos;
  vec3 nrm;
  vec2 texCoord;
};

struct Material  // Created by readModel; used in shaders
{
  vec3  diffuse;
  vec3  specular;
  vec3  emission;
  float shininess;
  int   textureId;
};


// Push constant structure for the ray tracer
struct PushConstantDenoise
{
  float normFactor;
  float depthFactor;
  float varianceFactor;
  float lumenFactor;

  int  dist;
  bool varianceView;
  bool placeholder1;
  bool placeholder2;

};

struct RayPayload
{
	bool hit; // Does the ray intersect anything or not?
	vec3 hitPos; // The world coordinates of the hit point.
	int instanceIndex; // Index of the object instance hit (we have only one, so =0)
	int primitiveIndex; // Index of the hit triangle primitive within object
	vec3 bc; // Barycentric coordinates of the hit point within triangle
	uint seed; // random seed
	bool occluded;
	float depth;
};

struct Emitter
{
	vec3 v0, v1, v2; // Vertices of light emitting triangle
	vec3 emission; // Its emission
	vec3 normal; // its normal
	float area; // Its triangle area.
	uint index; // Not needed, but used for verification
};


#endif
