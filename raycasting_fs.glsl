#version 450
#define EPSILON 1e-6
#define UIMAGE3D_GRID 1
#define LINKED_LIST 0 // exclusive with UIMAGE3D_GRID !!!
in vec3 ray_dir;
layout(location = 0) out vec4 frag_color;

uniform vec3 eye;
uniform vec3 eye_grid;
uniform float inv_model; // world to grid coordinate
#if UIMAGE3D_GRID
layout(binding = 0, r32ui) uniform readonly restrict uimage3D grid;
#endif
#if LINKED_LIST
layout(binding = 0, std430) coherent buffer Next {
	uint next[];
};
layout(binding = 1, std430) coherent buffer Comp { 
	int comp[]; // encoded signed distance between voxel center and a triangle
};
layout(binding = 2, std430) coherent buffer Data { 
	uint data[]; // triangle's id
};
uniform uint size;
uniform uint size3;
#endif

struct Box //  axis-aligned box
{
	vec3 pmin;
	vec3 pmax;
};
struct Ray
{
	vec3 ro;
	vec3 rd; // normalized ray direction
	vec3 ird; // inversed ray direction, i.e. dt / da (differential distance along axis)
	float tmin; // >=0
	float tmax; // >=0
};
// Understanding the Efficiency of Ray Traversal on GPUs ¨C Kepler and Fermi Addendum
bool intersect(in Box aab, inout Ray ray)
{
	// slab intersection
	vec3 tmin = (aab.pmin - ray.ro) * ray.ird; // [-inf, inf]
	vec3 tmax = (aab.pmax - ray.ro) * ray.ird; // [-inf, inf]
	// span intersection
	vec3 bmin = min(tmin, tmax);
	vec3 bmax = max(tmin, tmax);
	ray.tmin = max(ray.tmin, max(bmin.x, max(bmin.y, bmin.z)));
	ray.tmax = min(ray.tmax, min(bmax.x, min(bmax.y, bmax.z)));

	return ray.tmin <= ray.tmax;
}
// signed distance of sphere
float sphere_sd(vec3 p, float d) { return length(p) - d; }
// branchless 3D-DDA traversal of uniform grid, find the 1st incident voxel along the ray
// pos: box position(i.e. index), rt: distance to the hit point, nor: surface normal of the hit point
bool traverse(in Ray ray, out vec3 pos, out float rt, out vec3 nor)
{
	const vec3 ro = ray.ro + ray.tmin * ray.rd; // assuming inside a box
	pos = floor(ro); // index of the the starting box, assuming it is empty
	const vec3 dir = sign(ray.rd); // direction to next box, e.g. (+1, +1, +1) for the 1st quadrant
	vec3 da = pos + 0.5 + dir * 0.5 - ro; // differential distance w.r.t. axis
	vec3 dt = da * ray.ird; // slab intersection, differential distance w.r.t. ray

	rt = ray.tmin;
	while (rt <= ray.tmax)
	{
		rt = ray.tmin + min(dt.x, min(dt.y, dt.z)); // span intersection
		// all components of minimum mask (i.e. x <= y && x <= z, y <= x && y <= z, z <= y && z <= x) 
		// are false except for the corresponding smallest component of dt (if no mask), which 
		// is the axis along which the ray should be incremented
		vec3 mm = step(vec3(dt.xyz), vec3(dt.yxy)) * step(vec3(dt.xyz), vec3(dt.zzx));
		da = mm * dir;
		// visit next box
		pos += da;
		dt += da * ray.ird;
		// hit test
		#if UIMAGE3D_GRID
		if (imageLoad(grid, ivec3(pos)).r > 0)
		#elif LINKED_LIST
		if (next[(uint(pos.z) * size + uint(pos.y)) * size + uint(pos.x)] != 0)
		#else
		if (sphere_sd(pos + 0.5 - inv_model, inv_model) <= 0.0) // move sphere to the grid center
		#endif
		{
			nor = -da;
			break;
		}
	}

	return rt <= ray.tmax;
}

void main() {
	Ray ray;
	ray.ro = eye;
	ray.rd = normalize(ray_dir);
	ray.ird = 1.0 / ray.rd;
	ray.tmin = 0.0;
	ray.tmax = 1e4;

	Box aab = Box(vec3(-1.0), vec3(1.0));
	frag_color = vec4(0.f, 153.0/255.0, 1.f, 1.f);
	if(intersect(aab, ray))
	{
		// transform ray into *grid* coordinate
		// note uniform scaling does not change direction
		ray.ro = eye_grid;
		ray.tmin *= inv_model;
		ray.tmax *= inv_model;
		// avoid of missing boxes on the boundary due to precision
		ray.tmin -= EPSILON;
		vec3 pos, nor;
		float rt;
		if(traverse(ray, pos, rt, nor))
		{
			frag_color = vec4(abs(nor), 1.0);
			//frag_color = vec4(clamp(
			//	dot(normalize(eye_grid - (ray.ro + rt * ray.rd)), nor), 
			//	0.0, 1.0));
		}
	}
}