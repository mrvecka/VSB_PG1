#include "stdafx.h"
#include "raytracer.h"
#include "objloader.h"
#include "tutorials.h"
#include "material.h"
#include "background.h"
#include "utils.h"
#define _USE_MATH_DEFINES
#include <math.h>

Raytracer::Raytracer(const int width, const int height,
	const float fov_y, const Vector3 view_from, const Vector3 view_at,
	const char * config) : SimpleGuiDX11(width, height)
{
	InitDeviceAndScene(config);

	camera_ = Camera(width, height, fov_y, view_from, view_at);
	background = SphericalBackground("../../../data/background.jpg");
}

Vector3 get_hit_point(const RTCRay &ray)
{
	return Vector3(
		ray.org_x + ray.tfar * ray.dir_x,
		ray.org_y + ray.tfar * ray.dir_y,
		ray.org_z + ray.tfar * ray.dir_z);
}


float SQR(float r) {
	return r * r;
}

Raytracer::~Raytracer()
{
	ReleaseDeviceAndScene();
}

int Raytracer::InitDeviceAndScene(const char * config)
{
	device_ = rtcNewDevice(config);
	error_handler(nullptr, rtcGetDeviceError(device_), "Unable to create a new device.\n");
	rtcSetDeviceErrorFunction(device_, error_handler, nullptr);

	ssize_t triangle_supported = rtcGetDeviceProperty(device_, RTC_DEVICE_PROPERTY_TRIANGLE_GEOMETRY_SUPPORTED);

	// create a new scene bound to the specified device
	scene_ = rtcNewScene(device_);

	return S_OK;
}

int Raytracer::ReleaseDeviceAndScene()
{
	rtcReleaseScene(scene_);
	rtcReleaseDevice(device_);

	return S_OK;
}

inline Vector3 reflect(const Vector3 & v, const Vector3 & n) {
	return 2.0f * (n.DotProduct(v))* n - v;
}

float Raytracer::trace_shadow_ray(const Vector3 & p, const Vector3 & l_d, const float dist) {
	RTCHit hit;
	hit.geomID = RTC_INVALID_GEOMETRY_ID;
	hit.primID = RTC_INVALID_GEOMETRY_ID;

	RTCRay ray = RTCRay();
	ray.org_x = p.x; // ray origin
	ray.org_y = p.y;
	ray.org_z = p.z;

	ray.dir_x = l_d.x;
	ray.dir_y = l_d.y;
	ray.dir_z = l_d.z;

	ray.tnear = 0.1f;
	ray.tfar = dist;

	ray.time = 0.0f;

	ray.mask = 0; // can be used to mask out some geometries for some rays
	ray.id = 0; // identify a ray inside a callback function
	ray.flags = 0; // reserved

	RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	rtcOccluded1(scene_, &context, &ray);

	if (ray.tfar < dist) {
		return 0.0;
	}
	else {
		return 1.0;
	}
}

void Raytracer::LoadScene(const std::string file_name)
{
	const int no_surfaces = LoadOBJ(file_name.c_str(), surfaces_, materials_);

	// surfaces loop
	for (auto surface : surfaces_)
	{
		RTCGeometry mesh = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);

		Vertex3f * vertices = (Vertex3f *)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
			sizeof(Vertex3f), 3 * surface->no_triangles());

		Triangle3ui * triangles = (Triangle3ui *)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
			sizeof(Triangle3ui), surface->no_triangles());

		rtcSetGeometryUserData(mesh, (void*)(surface->get_material()));

		rtcSetGeometryVertexAttributeCount(mesh, 2);

		Normal3f * normals = (Normal3f *)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
			sizeof(Normal3f), 3 * surface->no_triangles());

		Coord2f * tex_coords = (Coord2f *)rtcSetNewGeometryBuffer(
			mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
			sizeof(Coord2f), 3 * surface->no_triangles());

		// triangles loop
		for (int i = 0, k = 0; i < surface->no_triangles(); ++i)
		{
			Triangle & triangle = surface->get_triangle(i);

			// vertices loop
			for (int j = 0; j < 3; ++j, ++k)
			{
				const Vertex & vertex = triangle.vertex(j);

				vertices[k].x = vertex.position.x;
				vertices[k].y = vertex.position.y;
				vertices[k].z = vertex.position.z;

				normals[k].x = vertex.normal.x;
				normals[k].y = vertex.normal.y;
				normals[k].z = vertex.normal.z;

				tex_coords[k].u = vertex.texture_coords[0].u;
				tex_coords[k].v = vertex.texture_coords[0].v;
			} // end of vertices loop

			triangles[i].v0 = k - 3;
			triangles[i].v1 = k - 2;
			triangles[i].v2 = k - 1;
		} // end of triangles loop

		rtcCommitGeometry(mesh);
		unsigned int geom_id = rtcAttachGeometry(scene_, mesh);
rtcReleaseGeometry(mesh);
	} // end of surfaces loop

	rtcCommitScene(scene_);
}

Color4f Raytracer::get_pixel(const int x, const int y, const float t)
{
	// merge ray and hit structures
	RayHitWithIOR my_ray_hit;

	// Uniform supersampling
	Color4f colorSum = Color4f(0.0f, 0.0f, 0.0f, 1.0f);
	int size = 1;

	float offsetX = -0.5f;
	float offsetY = -0.5f;
	float offsetAddition = 1.0f / size;

	int samples = 5;

//#pragma omp parallel for schedule(dynamic, 5) shared(colorSum)
//	for (int i = 0; i < samples; i++) {
//		float offsetX = (x + Random());
//		float offsetY = (y + Random());
//
//		my_ray_hit.rayHit.ray = camera_.GenerateRay(offsetX, offsetY);
//		my_ray_hit.rayHit.hit = createEmptyHit();
//		my_ray_hit.ior = IOR_AIR;
//		Color4f traced = start_tracer(my_ray_hit, 4);
//		colorSum += traced;
//	}
//
//	return colorSum / static_cast<float>(samples);

	offsetX = (x + Random());
	offsetY = (y + Random());

	my_ray_hit.rayHit.ray = camera_.GenerateRay(offsetX, offsetY);
	my_ray_hit.rayHit.hit = createEmptyHit();
	my_ray_hit.ior = IOR_AIR;
	Color4f traced = start_tracer(my_ray_hit, 4);

	return traced;
}

Color4f Raytracer::start_tracer(RayHitWithIOR rayHitWithIor, int depth) {
	
	// intersect ray with the scene
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);
	rtcIntersect1(scene_, &context, &rayHitWithIor.rayHit);

	if (rayHitWithIor.rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		// we hit something
		RTCGeometry geometry = rtcGetGeometry(scene_, rayHitWithIor.rayHit.hit.geomID);
		Normal3f normal;
		// get interpolated normal
		rtcInterpolate0(geometry, rayHitWithIor.rayHit.hit.primID, rayHitWithIor.rayHit.hit.u, rayHitWithIor.rayHit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &normal.x, 3);

		// and texture coordinates
		Coord2f tex_coord;
		rtcInterpolate0(geometry, rayHitWithIor.rayHit.hit.primID, rayHitWithIor.rayHit.hit.u, rayHitWithIor.rayHit.hit.v,
			RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &tex_coord.u, 2);
		
		tex_coord.v = 1.0f - tex_coord.v;

		Material * material = (Material *)(rtcGetGeometryUserData(geometry));

		//const Triangle & triangle = surfaces_[ray_hit]
		Vector3 light_pos = Vector3(-100, -100, 300);

		Vector3 hitPoint = get_hit_point(rayHitWithIor.rayHit.ray);
		Vector3 l_d = light_pos - hitPoint;
		l_d.Normalize();

		Vector3 rd = Vector3(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
		Vector3 new_normal = Vector3(normal.x, normal.y, normal.z);
		if (rd.DotProduct(new_normal) > 0) {
			new_normal *= -1;
		}

		if (depth <= 0) {
			Color4f bck = background.GetTexel(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);

			return Color4f(getSRGBColor(bck.r), getSRGBColor(bck.g), getSRGBColor(bck.b), 1.0f);
		}

		switch (material->shader_)
		{
			case Shader::NORMAL:
			{
				return DoShaderNormal(new_normal);
				break;
			}
			case Shader::LAMBERT:
			{
				return DoShaderLambert(hitPoint, l_d,material,l_d,new_normal,tex_coord);
				break;
			}
			case Shader::PHONG:
			{
				return DoShaderPhong(rayHitWithIor,hitPoint,l_d,new_normal,material,tex_coord);
				break;
			}
			case Shader::PHONG_BRDF:
			{
				return DoShaderPhongBRDF(rayHitWithIor, hitPoint, l_d, new_normal, material, tex_coord,depth);
				break;
			}
			case Shader::GLASS:
			{
				return DoShaderGlass(rayHitWithIor, new_normal, material,depth);
				break;
			}
			case Shader::WHITTED:
			{
				return DoShaderReflected(rayHitWithIor, new_normal, material, depth);
				break;
			}
			case Shader::PathTracer:
			{
				return DoPathTracer(rayHitWithIor, new_normal, material, depth);
				break;
			}
			default:
			{
				Vector3 diff = material->doDiffuse(&tex_coord);
				float dot = l_d.DotProduct(new_normal);
				Vector3 temp = max(0, dot) * diff;
				return Color4f(temp.x, temp.y, temp.z, 1.0f);
				break;
			}
		}

		return Color4f(0.0f, 0.0f, 0.0f, 1.0f);
	}
	else {
		Color4f bck = background.GetTexel(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
		//return background;
		return Color4f(getSRGBColor(bck.r), getSRGBColor(bck.g), getSRGBColor(bck.b), 1.0f);
	}
}

Color4f Raytracer::DoShaderNormal(Vector3 normal)
{
	return Color4f(normal.x * 0.5f + 0.5f, normal.y*0.5f + 0.5f, normal.z*0.5f + 0.5f, 1.0f);
}

Color4f Raytracer::DoShaderLambert(Vector3 hitPoint, Vector3 lightPos, Material *material, Vector3 light_pos,Vector3 normal, Coord2f texture)
{
	Vector3 diff = material->doDiffuse(&texture);
	float dot = light_pos.PosDotProduct(normal);
	float shadow = trace_shadow_ray(hitPoint, lightPos, lightPos.L2Norm());
	Vector3 resColor = max(0, dot) * diff;
	return Color4f(resColor.x + shadow , resColor.y + shadow, resColor.z + shadow, 1.0f);
}

Color4f Raytracer::DoShaderPhong(RayHitWithIOR rayHitWithIor, Vector3 hitPoint,Vector3 lightPos, Vector3 normal,Material *material, Coord2f texture)
{
	Vector3 v = Vector3(-rayHitWithIor.rayHit.ray.dir_x, -rayHitWithIor.rayHit.ray.dir_y, -rayHitWithIor.rayHit.ray.dir_z);
	Vector3 l_r = reflect(lightPos, normal);

	Vector3 diff = material->doDiffuse(&texture);

	const float enlight = trace_shadow_ray(hitPoint, lightPos, lightPos.L2Norm());
	Color4f resultColor = Color4f{
		(material->ambient.x + enlight * ((diff.x * normal.DotProduct(lightPos)) + pow(material->specular.x * v.DotProduct(l_r), material->shininess))),
		(material->ambient.y + enlight * ((diff.y * normal.DotProduct(lightPos)) + pow(material->specular.y * v.DotProduct(l_r), material->shininess))),
		(material->ambient.z + enlight * ((diff.z * normal.DotProduct(lightPos)) + pow(material->specular.z * v.DotProduct(l_r), material->shininess))),
		1 };
	return resultColor;
}
Color4f Raytracer::DoShaderPhongBRDF(RayHitWithIOR rayHitWithIor, Vector3 hitPoint, Vector3 lightPos, Vector3 normal, Material *material, Coord2f texture,int depth)
{
	RayHitWithIOR myReflectedRayHit;

	Vector3 diffuse = material->diffuse;
	Vector3 rd = Vector3(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
	Vector3 rv = Vector3(-rd.x, -rd.y, -rd.z);

	float n1 = rayHitWithIor.ior;
	float n2 = ((n1 == IOR_AIR) ? material->ior : IOR_AIR);

	Vector3 rr = (2.0f * (normal.DotProduct(rv))) * normal - rv;
	Vector3 vector = get_hit_point(rayHitWithIor.rayHit.ray);

	// Generate reflected ray
	myReflectedRayHit = createRayHit(vector, rr, FLT_MAX, 0.001f, n2);
	Vector3 rd2 = Vector3(myReflectedRayHit.rayHit.ray.dir_x, myReflectedRayHit.rayHit.ray.dir_y, myReflectedRayHit.rayHit.ray.dir_z);

	Color4f result = start_tracer(myReflectedRayHit, depth - 1);

	Vector3 v = Vector3(-rayHitWithIor.rayHit.ray.dir_x, -rayHitWithIor.rayHit.ray.dir_y, -rayHitWithIor.rayHit.ray.dir_z);
	Vector3 l_r = reflect(lightPos, normal);

	Vector3 diff = material->doDiffuse(&texture);

	const float enlight = trace_shadow_ray(hitPoint, lightPos, lightPos.L2Norm());
	Color4f resultColor = Color4f{
		(material->ambient.x + enlight * ((diff.x * normal.DotProduct(lightPos)) + pow(material->specular.x * v.DotProduct(l_r), material->shininess))),
		(material->ambient.y + enlight * ((diff.y * normal.DotProduct(lightPos)) + pow(material->specular.y * v.DotProduct(l_r), material->shininess))),
		(material->ambient.z + enlight * ((diff.z * normal.DotProduct(lightPos)) + pow(material->specular.z * v.DotProduct(l_r), material->shininess))),
		1 };

	return material->ambient * result;
}

Color4f Raytracer::DoShaderGlass(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material,int depth)
{
	RayHitWithIOR myRefractedRTCRayHit, reflectedRayHit;

	Vector3 diffuse = material->diffuse;
	Vector3 rayDirection = Vector3(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
	//direction to observer
	Vector3 rv = Vector3(-rayDirection.x, -rayDirection.y, -rayDirection.z);

	float n1 = rayHitWithIor.ior;
	float n2 = ((n1 == IOR_AIR) ? material->ior : IOR_AIR);

	float ior_divided = n1 / n2;
	float cos_01 = (normal.DotProduct(rv));

	//direction of the reflected ray
	Vector3 rr = (2.0f * (normal.DotProduct(rv))) * normal - rv;

	float tmp = 1.0f - SQR(ior_divided) * (1.0f - SQR(cos_01));
	Vector3 vector = get_hit_point(rayHitWithIor.rayHit.ray);

	// Generate reflected ray
	reflectedRayHit = createRayHit(vector, rr, FLT_MAX, 0.001f, n2);

	//absolute reflection
	if (tmp > 0) {
		float cos_02 = sqrt(tmp);
		Vector3 rl = (ior_divided * rayDirection) + ((ior_divided * cos_01 - cos_02) * normal);
		// Fresnel
		float Rs = SQR((n2 * cos_02 - n1 * cos_01) / (n2 * cos_02 + n1 * cos_01));
		float Rp = SQR((n2 * cos_01 - n1 * cos_02) / (n2 * cos_01 + n1 * cos_02));
		float R = 0.5f * (Rs + Rp); //coef. of reflected ray

		float T = 1.0f - R; // coef. of refracted ray

		// Generate refracted ray
		myRefractedRTCRayHit = createRayHit(vector, rl, FLT_MAX, 0.001f, n2);

		return diffuse * start_tracer(reflectedRayHit, depth - 1) * R + diffuse * start_tracer(myRefractedRTCRayHit, depth - 1) * T;
	}
	else {
		return diffuse * start_tracer(reflectedRayHit, depth - 1);
	}
}

Color4f Raytracer::DoShaderReflected(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material, int depth)
{
	RayHitWithIOR myReflectedRayHit;

	Vector3 diffuse = material->diffuse;
	Vector3 rd = Vector3(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
	Vector3 rv = Vector3(-rd.x, -rd.y, -rd.z);

	float n1 = rayHitWithIor.ior;
	float n2 = ((n1 == IOR_AIR) ? material->ior : IOR_AIR);

	Vector3 rr = (2.0f * (normal.DotProduct(rv))) * normal - rv;
	Vector3 vector = get_hit_point(rayHitWithIor.rayHit.ray);

	// Generate reflected ray
	myReflectedRayHit = createRayHit(vector, rr, FLT_MAX, 0.001f, n2);

	return diffuse * start_tracer(myReflectedRayHit, depth - 1);
}

Color4f Raytracer::DoPathTracer(RayHitWithIOR rayHitWithIor, Vector3 normal, Material *material, int depth)
{
	Color4f emmision = Color4f{ material->emission.x, material->emission.y, material->emission.z, 1 };
	if (emmision.r != 0 && emmision.g != 0 && emmision.b != 0) {
		return emmision;
	}

	Vector3 rd = Vector3(rayHitWithIor.rayHit.ray.dir_x, rayHitWithIor.rayHit.ray.dir_y, rayHitWithIor.rayHit.ray.dir_z);
	Vector3 rv = Vector3(-rd.x, -rd.y, -rd.z);


	Vector3 omegaI = HemisphereSampling(normal);
	float pdf = 1 / (2 * M_PI);

	Color4f l_i = start_tracer(createRayHit(get_hit_point(rayHitWithIor.rayHit.ray), omegaI, FLT_MAX, 0.001f, IOR_AIR), depth - 1);
	Vector3 fR = material->diffuse / M_PI;

	Color4f resultColor = fR * l_i * (normal.DotProduct(omegaI) / pdf);
	return resultColor;
}

Vector3 Raytracer::HemisphereSampling(Vector3 normal) {
	float randomU = Random();
	float randomV = Random();

	float x = 2 * cosf(2 * M_PI * randomU) * sqrt(randomV * (1 - randomV));
	float y = 2 * sinf(2 * M_PI * randomU) * sqrt(randomV * (1 - randomV));
	float z = 1 - 2 * randomV;

	Vector3 omegaI = Vector3{ x, y, z };
	omegaI.Normalize();

	if (omegaI.DotProduct(normal) < 0) {
		omegaI *= -1;
	}
	return omegaI;
}

float  Raytracer::castShadowRay(RTCIntersectContext context, Vector3 vectorToLight, float dstToLight, Vector3 intersectionPoint, Vector3 normal) {
	RTCRay rayFromIntersectPointToLight = createRay(intersectionPoint, vectorToLight, dstToLight, 0.1f);
	rtcOccluded1(scene_, &context, &rayFromIntersectPointToLight);
	return rayFromIntersectPointToLight.tfar < dstToLight ? 0.0f : 1.0f;
}

bool Raytracer::russianRoulete() {
	float random = Random();
	return random < 0.15f;
}

Color4f Raytracer::getAttenuationOfRay(Vector3 vectorToIntersection, Material* material, float actualIor) {
	Color4f attenuation = { 1,1,1,1 };
	if (actualIor != IOR_AIR) {
		float dstToIntersection = vectorToIntersection.L2Norm();
		attenuation = Color4f{ exp(-0.0001f*dstToIntersection) * material->diffuse.x, exp(-0.0001f*dstToIntersection) * material->diffuse.y, exp(-0.0001f*dstToIntersection) * material->diffuse.z, 1.0f };
	}
	return attenuation;
}

int Raytracer::Ui()
{
	static float f = 0.0f;
	static int counter = 0;

	// we use a Begin/End pair to created a named window
	ImGui::Begin("Ray Tracer Params");

	ImGui::Text("Surfaces = %d", surfaces_.size());
	ImGui::Text("Materials = %d", materials_.size());
	ImGui::Separator();
	ImGui::Checkbox("Vsync", &vsync_);

	//ImGui::Checkbox( "Demo Window", &show_demo_window );      // Edit bools storing our window open/close state
	//ImGui::Checkbox( "Another Window", &show_another_window );

	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
	//ImGui::ColorEdit3( "clear color", ( float* )&clear_color ); // Edit 3 floats representing a color

	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	// 3. Show another simple window.
	/*if ( show_another_window )
	{
	ImGui::Begin( "Another Window", &show_another_window );   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
	ImGui::Text( "Hello from another window!" );
	if ( ImGui::Button( "Close Me" ) )
	show_another_window = false;
	ImGui::End();
	}*/

	return 0;
}
