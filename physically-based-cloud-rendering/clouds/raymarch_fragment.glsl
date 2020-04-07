#version 460 core

out vec4 fragment_colour;

in vec2 uvs;

uniform sampler2D full_screen;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_pos;

vec3 aabb_min = vec3(-1, 2, -1);
vec3 aabb_max = vec3(1, 4, 1);

// intersects if retval.y > max(retval.x, 0.0)
vec2 intersect_aabb(vec3 ray_origin, vec3 ray_dir, vec3 box_min, vec3 box_max) 
{
	vec3 t_min = (box_min - ray_origin) / ray_dir;
	vec3 t_max = (box_max - ray_origin) / ray_dir;
	vec3 t1 = min(t_min, t_max);
	vec3 t2 = max(t_min, t_max);
	float t_near = max(max(t1.x, t1.y), t1.z);
	float t_far = min(min(t2.x, t2.y), t2.z);
	return vec2(t_near, t_far);
}

void main()
{
	fragment_colour = texture(full_screen, uvs);

	// calculate ray in world space
	float x = uvs.x*2.0 - 1.0;
	float y = uvs.y*2.0 - 1.0;
	float z = -1.0;

	vec4 ray_clip = vec4(x, y, z, 1.0);
	vec4 ray_eye = inverse(projection)*ray_clip;
	ray_eye = vec4(ray_eye.xy, z, 0.0);

	vec3 ray_world = normalize((inverse(view)*ray_eye).xyz);

	vec2 res = intersect_aabb(camera_pos, ray_world, aabb_min, aabb_max);
	if (res.y > max(0.0, res.x))
	{
		// hit	
		fragment_colour = vec4(1.0, 0.0, 0.0, 1.0);
	}
}
