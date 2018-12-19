#pragma once
#include "simpleguidx11.h"
#include "surface.h"
#include "camera.h"
#include "structs.h"
#include "SphericalBackground.h"

/*! \class Raytracer
\brief General ray tracer class.

\author Tomáš Fabián
\version 0.1
\date 2018
*/
class Raytracer : public SimpleGuiDX11
{
public:
	Raytracer( const int width, const int height, 
		const float fov_y, const Vector3 view_from, const Vector3 view_at,
		const char * config = "threads=0,verbose=3" );
	~Raytracer();

	int InitDeviceAndScene( const char * config );

	int ReleaseDeviceAndScene();

	void LoadScene( const std::string file_name );

	Color4f get_pixel( const int x, const int y, const float t = 0.0f ) override;

	Color4f start_tracer(RayHitWithIOR ray, int depth);

	float linearToSrgb(float color);

	float trace_shadow_ray(const Vector3 & p, const Vector3 & l_d, const float dist);

	Vector3 HemisphereSampling(Vector3 normal);

	Color4f getAttenuationOfRay(Vector3 vectorToIntersection, Material* material, float actualIor);

	float getGeometryTerm(Vector3 omegaI, RTCIntersectContext context, Vector3 vectorToLight, Vector3 intersectionPoint, Vector3 normal);

	float castShadowRay(RTCIntersectContext context, Vector3 vectorToLight, float dstToLight, Vector3 intersectionPoint, Vector3 normal);

	Color4f DoShaderNormal(Vector3 normal);
	Color4f DoShaderLambert(Vector3 hitPoint, Vector3 lightPos, Material *material, Vector3 light_pos, Vector3 normal, Coord2f tex_coord);
	Color4f DoShaderPhong(RayHitWithIOR rayHitWithIor, Vector3 hitPoint, Vector3 lightPos, Vector3 normal, Material *material, Coord2f texture);
	Color4f DoShaderPhongBRDF(RayHitWithIOR rayHitWithIor, Vector3 hitPoint, Vector3 lightPos, Vector3 normal, Material *material, Coord2f texture,int depth);
	Color4f DoShaderGlass(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material,int depth);
	Color4f DoShaderReflected(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material, int depth);
	Color4f DoPathTracer(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material, int depth);

	bool russianRoulete();

	int Ui();

private:
	std::vector<Surface *> surfaces_;
	std::vector<Material *> materials_;

	RTCDevice device_;
	RTCScene scene_;
	Camera camera_;
	SphericalBackground background;
};
