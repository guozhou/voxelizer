#version 450
#define LINKED_LIST 0
in gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
} gl_in[];
out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};
#if LINKED_LIST
layout(triangles_adjacency) in;
#else
layout(triangles) in;
#endif
layout(triangle_strip, max_vertices = 3) out;

uniform mat4 model_view_proj_mat;
uniform uint size;
// 'restrict' applies to the pointer, not the data being pointed to 
// according to shader_image_load_store (crashing now however)
layout(binding = 0, r32ui) uniform writeonly restrict uimage3D grid;

#if LINKED_LIST
layout(binding = 0, offset = 0) uniform atomic_uint new_node; // = size^3
layout(binding = 0, std430) coherent buffer Next {
	uint next[];
};
layout(binding = 1, std430) coherent buffer Comp { 
	int comp[]; // encoded signed distance between voxel center and a triangle
};
layout(binding = 2, std430) coherent buffer Data { 
	int data[]; // triangle's id (no head)
};

in vec3 normal_model[];

uniform uint size3;
// record comp value nearer to zero into head
void insert_head(uint i, float c)
{
	int old, curr = comp[i], val;
	const float ca = abs(c);
	do {
		old = curr;
		const float oldf = intBitsToFloat(old);
		const vec2 a = vec2(abs(oldf), ca);
		if(a.x == a.y) return; // same value does not need to be recorded again
		val = floatBitsToInt(dot(step(a.xy, a.yx), vec2(oldf, c)));
		curr = atomicCompSwap(comp[i], old, val); // step(a.xy, a.yx) is minimum mask
	} while(curr != old);
}

void insert_triangle(uvec3 p, float c)
{
	const uint mine = atomicCounterIncrement(new_node);
	comp[mine] = floatBitsToInt(c);
	data[mine - size3] = gl_PrimitiveIDIn;
	
	uint prior = (p.z * size + p.y) * size + p.x, link = next[prior], old;
	insert_head(prior, c);
	do { // insert mine after prior, i.e. next[prior] -> new, next[mine] -> link
		old = link;
		next[mine] = old; // test if both next[mine] -> link and next[prior] -> link
		link = atomicCompSwap(next[prior], link, mine);
	} while(link != old);
}
// find the signed distance from p to current triangle
float triangle_sd(vec3 p)
{
	vec3 a = vec3(gl_in[0].gl_Position), b = vec3(gl_in[2].gl_Position), c = vec3(gl_in[4].gl_Position);
	const vec3 ab = b - a, ac = c - a, bc = c - b;
	// Check if P in vertex region outside A
	vec3 ap = p - a;
	float d1 = dot(ab, ap);
	float d2 = dot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f) // barycentric coordinates (1,0,0)
		return sign(dot(ap, normal_model[0])) * length(ap);
	// Check if P in vertex region outside B
	vec3 bp = p - b;
	float d3 = dot(ab, bp);
	float d4 = dot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3) // barycentric coordinates (0,1,0)
		return sign(dot(bp, normal_model[2])) * length(bp);
	// Check if P in vertex region outside C
	vec3 cp = p - c;
	float d5 = dot(ab, cp);
	float d6 = dot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6) // barycentric coordinates (0,0,1)
		return sign(dot(cp, normal_model[4])) * length(cp);
	
	vec3 n = normalize(cross(ab, ac));
	// Check if P in edge region of AB, if so return projection of P onto AB
	float vc = d1*d4 - d3*d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		float v = d1 / (d1 - d3);
		vec3 n1 = normalize(cross(vec3(gl_in[1].gl_Position) - a, ab));
		vec3 r = p - (a + v * ab);
		return sign(dot(r, n + n1)) * length(r); // barycentric coordinates (1-v,v,0)
	}
	// Check if P in edge region of AC, if so return projection of P onto AC
	float vb = d5*d2 - d1*d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		float w = d2 / (d2 - d6);
		vec3 n5 = normalize(cross(ac, vec3(gl_in[5].gl_Position) - a));
		vec3 r = p - (a + w * ac); // barycentric coordinates (1-w,0,w)
		return sign(dot(r, n + n5)) * length(r);
	}
	// Check if P in edge region of BC, if so return projection of P onto BC
	float va = d3*d6 - d5*d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		vec3 n3 = normalize(cross(vec3(gl_in[3].gl_Position) - b, bc));
		vec3 r = p - (b + w * bc); // barycentric coordinates (0,1-w,w)
		return sign(dot(r, n + n3)) * length(r);
	}
	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	vec3 r = p - (a + ab * v + ac * w); // = u*a + v*b + w*c, u = va * denom = 1.0f - v - w
	return sign(dot(r, n)) * length(r);
}

void test_insert_triangle()
{
	uvec3 p;
	for(p.z = 0; p.z < size; p.z++)
		for(p.y = 0; p.y < size; p.y++)
			for(p.x = 0; p.x < size; p.x++)
				insert_triangle(p, 1.0);
}
#endif

const mat3 swizzleLUT[] = {
	mat3(0, 0, 1, 1, 0, 0, 0, 1, 0), 
	mat3(0, 1, 0, 0, 0, 1, 1, 0, 0), 
	mat3(1, 0, 0, 0, 1, 0, 0, 0, 1)
};
const mat3 unswizzleLUT[] = {
	mat3(0, 1, 0, 0, 0, 1, 1, 0, 0),
	mat3(0, 0, 1, 1, 0, 0, 0, 1, 0),
	mat3(1, 0, 0, 0, 1, 0, 0, 0, 1)
};

int swizzle_triangle(inout vec3 p0, inout vec3 p1, inout vec3 p2)
{
	vec3 e0 = p1 - p0, e1 = p2 - p1, nor = abs(cross(e0, e1));
	// determine the dominant normal direction by maximum mask
	// y <= x && z <= x, x <= y && z <= y, x <= z && y <= z
	vec3 mm = step(vec3(nor.yxx), vec3(nor.xyz)) * step(vec3(nor.zzy), vec3(nor.xyz));
	vec3 dir = mm * vec3(0.0, 1.0, 2.0); // plane yz, zx, xy
	int ddir = int(max(dir.x, max(dir.y, dir.z)));
	// transform the triangle such that xy plane is the axis of maximum projection
	const mat3 swizzle = swizzleLUT[ddir];
	p0 = swizzle * p0;
	p1 = swizzle * p1;
	p2 = swizzle * p2;

	return ddir;
}

void test_swizzle_triangle(vec3 p0, vec3 p1, vec3 p2)
{
	gl_Position = model_view_proj_mat * vec4(p0, 1.0);
	EmitVertex();
	gl_Position = model_view_proj_mat * vec4(p1, 1.0);
	EmitVertex();
	gl_Position = model_view_proj_mat * vec4(p2, 1.0);
	EmitVertex();
	EndPrimitive();
}

// test whether the triangle and the unit box overlap in all three, mutually orthogonal, 2D main projections in 
// plane yz, zx, xy by evaluating "unnormalized" distance from the farthest box corner (b + c) to the edge ei
// <n_ei_zx, b_zx + c_zx - v_i_zx> * <n_ei_yz, b_yz + c_yz - v_i_yz> * <n_ei_xy, b_xy + c_xy - v_i_xy> >= 0
void voxelize_triangle(vec3 p0, vec3 p1, vec3 p2, int ddir)
{
	const mat3 unswizzle = unswizzleLUT[ddir];
	const vec3 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2; //figure 17/18 line 2-3
	vec3 n = cross(e0, e1), dir = step(0.f, n) * 2.f - 1.f; // 1 for 0 <= n
	// d0 = <n, c - p0>, d1 = <n, deltab - c - p0>
	float dot_n_p0 = dot(n, p0);
	float d0 = max(0.f, n.x) + max(0.f, n.y) + max(0.f, n.z) - dot_n_p0;
	float d1 = min(0.f, n.x) + min(0.f, n.y) + min(0.f, n.z) - dot_n_p0;
	// projected inward-facing edge normals
	vec2 n_e0_yz = dir.x * vec2(-e0.z, e0.y); //figure 17/18 line 5
	vec2 n_e1_yz = dir.x * vec2(-e1.z, e1.y);
	vec2 n_e2_yz = dir.x * vec2(-e2.z, e2.y);
	vec2 n_e0_zx = dir.y * vec2(-e0.x, e0.z); //figure 17/18 line 6
	vec2 n_e1_zx = dir.y * vec2(-e1.x, e1.z);
	vec2 n_e2_zx = dir.y * vec2(-e2.x, e2.z);
	vec2 n_e0_xy = dir.z * vec2(-e0.y, e0.x); //figure 17/18 line 4
	vec2 n_e1_xy = dir.z * vec2(-e1.y, e1.x);
	vec2 n_e2_xy = dir.z * vec2(-e2.y, e2.x);
	// d_ei_yz = <n_ei_yz, c_yz - v_i_yz>, d_ei_zx = <n_ei_zx, c_zx - v_i_zx>, d_ei_xy = <n_ei_xy, c_xy - v_i_xy>
	float d_e0_yz = max(0.f, n_e0_yz.x) + max(0.f, n_e0_yz.y) - dot(n_e0_yz, vec2(p0.yz)); //figure 17 line 8
	float d_e1_yz = max(0.f, n_e1_yz.x) + max(0.f, n_e1_yz.y) - dot(n_e1_yz, vec2(p1.yz));
	float d_e2_yz = max(0.f, n_e2_yz.x) + max(0.f, n_e2_yz.y) - dot(n_e2_yz, vec2(p2.yz));
	float d_e0_zx = max(0.f, n_e0_zx.x) + max(0.f, n_e0_zx.y) - dot(n_e0_zx, vec2(p0.zx)); //figure 18 line 9
	float d_e1_zx = max(0.f, n_e1_zx.x) + max(0.f, n_e1_zx.y) - dot(n_e1_zx, vec2(p1.zx));
	float d_e2_zx = max(0.f, n_e2_zx.x) + max(0.f, n_e2_zx.y) - dot(n_e2_zx, vec2(p2.zx));
	float d_e0_xy = max(0.f, n_e0_xy.x) + max(0.f, n_e0_xy.y) - dot(n_e0_xy, vec2(p0.xy)); //figure 17 line 7
	float d_e1_xy = max(0.f, n_e1_xy.x) + max(0.f, n_e1_xy.y) - dot(n_e1_xy, vec2(p1.xy));
	float d_e2_xy = max(0.f, n_e2_xy.x) + max(0.f, n_e2_xy.y) - dot(n_e2_xy, vec2(p2.xy));
	// range of indices of possible voxels in this triangle's AABB
	const vec3 bmin = max(vec3(0), floor(min(p0, min(p1, p2)))), bmax = min(vec3(size - 1), floor(max(p0, max(p1, p2))));
	vec3 b;
	for (b.x = bmin.x; b.x <= bmax.x; b.x++) // iterate over plane xy
	{
		for (b.y = bmin.y; b.y <= bmax.y; b.y++)
		{
			// <n_ei_xy, b_xy + c_xy - v_i_xy> = <n_ei_xy, b_xy> + d_ei_xy
			if ((dot(n_e0_xy, vec2(b.xy)) + d_e0_xy >= 0) &&
				(dot(n_e1_xy, vec2(b.xy)) + d_e1_xy >= 0) &&
				(dot(n_e2_xy, vec2(b.xy)) + d_e2_xy >= 0))
			{
				for (b.z = bmin.z; b.z <= bmax.z; b.z++)
				{
					// <n, b + c - p0><n, b + deltab - c - p0>
					// <n_ei_zx, b_zx + c_zx - v_i_zx> = <n_ei_zx, b_zx> + d_ei_zx
					// <n_ei_yz, b_yz + c_yz - v_i_yz> = <n_ei_yz, b_yz> + d_ei_yz
					float dot_n_b = dot(n, b);
					if (((dot_n_b + d0) * (dot_n_b + d1) <= 0) && 
						(dot(n_e0_zx, vec2(b.zx)) + d_e0_zx >= 0) &&
						(dot(n_e1_zx, vec2(b.zx)) + d_e1_zx >= 0) &&
						(dot(n_e2_zx, vec2(b.zx)) + d_e2_zx >= 0) &&
						(dot(n_e0_yz, vec2(b.yz)) + d_e0_yz >= 0) &&
						(dot(n_e1_yz, vec2(b.yz)) + d_e1_yz >= 0) &&
						(dot(n_e2_yz, vec2(b.yz)) + d_e2_yz >= 0))
					{
					#if LINKED_LIST
						vec3 p = unswizzle * b;
						insert_triangle(uvec3(p), triangle_sd(p + 0.5));
					#else
						imageStore(grid, ivec3(unswizzle * b), uvec4(1));
					#endif
					}
				}
			}
		}
	}
}

void main() {
#if LINKED_LIST
	vec3 p0 = vec3(gl_in[0].gl_Position), p1 = vec3(gl_in[2].gl_Position), 
		p2 = vec3(gl_in[4].gl_Position);
#else
	vec3 p0 = vec3(gl_in[0].gl_Position), p1 = vec3(gl_in[1].gl_Position), 
		p2 = vec3(gl_in[2].gl_Position);
#endif
	int ddir = swizzle_triangle(p0, p1, p2);
	//test_swizzle_triangle(p0, p1, p2);
	voxelize_triangle(p0, p1, p2, ddir);
}