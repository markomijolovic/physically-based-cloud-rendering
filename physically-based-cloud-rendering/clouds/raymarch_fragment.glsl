#version 460 core

out vec4 fragment_colour;

in vec2 uvs;

uniform sampler2D full_screen;
uniform sampler2D weather_map;
uniform sampler2D blue_noise;

uniform sampler3D cloud_base;
uniform sampler3D cloud_erosion;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_pos;

uniform int weather_map_visualization;
uniform int low_frequency_noise_visualization;
uniform int high_frequency_noise_visualization;
uniform float low_freq_noise_scale;
uniform float high_freq_noise_scale;
uniform float scattering_factor;
uniform float extinction_factor;
uniform float sun_intensity;
uniform float high_freq_noise_factor;
uniform int multiple_scattering_approximation;
uniform int N;
uniform float a;
uniform float b;
uniform float c;
uniform int primary_ray_steps;
uniform int secondary_ray_steps;
uniform float time;
uniform float cloud_speed;
uniform vec3 wind_direction;
uniform vec3 sun_direction;
uniform float global_cloud_coverage;
uniform float anvil_bias;
uniform int use_blue_noise;

const float pi = 3.141592653589793238462643383279502884197169;
const float one_over_pi = 1.0/pi;
const vec3 aabb_min = vec3(-30000, 1500, -30000);
const vec3 aabb_max = vec3(30000, 4000, 30000);
const vec2 weather_map_min = vec2(-30000, -30000);
const vec2 weather_map_max = vec2(30000, 30000);

const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;
const vec3 sun_radiance = vec3(25311.6, 26361.3, 24232.3);

// ----------------------------------------------------------------------------
// Rayleigh and Mie scattering atmosphere system
//
// implementation of the techniques described here:
// http://www.scratchapixel.com/old/lessons/3d-advanced-lessons/simulating-the-colors-of-the-sky/atmospheric-scattering/
// ----------------------------------------------------------------------------

struct ray_t {
	vec3 origin;
	vec3 direction;
};

struct sphere_t {
	vec3 origin;
	float radius;
	int material;
};

bool isect_sphere(ray_t ray, sphere_t sphere, inout float t0, inout float t1)
{
	vec3 rc = sphere.origin - ray.origin;
	float radius2 = sphere.radius * sphere.radius;
	float tca = dot(rc, ray.direction);
	float d2 = dot(rc, rc) - tca * tca;
	if (d2 > radius2) return false;
	float thc = sqrt(radius2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;

	return true;
}

// scattering coefficients at sea level (m)
const vec3 betaR = vec3(5.5e-6, 13.0e-6, 22.4e-6);  // Rayleigh 
const vec3 betaM = vec3(21e-6);                     // Mie

// scale height (m)
// thickness of the atmosphere if its density were uniform
const float hR = 7994.0; // Rayleigh
const float hM = 1200.0; // Mie

float rayleigh_phase_func(float mu)
{
	return
			3. * (1. + mu*mu)
	/ //------------------------
				(16. * pi);
}

float hg(float costheta, float g) 
{
    return 0.25 * one_over_pi * (1 - pow(g, 2.0)) / pow((1 + pow(g, 2.0) - 2 * g * costheta), 1.5);
}

const float earth_radius = 6360e3;      // (m)
const float atmosphere_radius = 6420e3; // (m)
const float sun_angular_diameter_cos = 0.999956676946448443553574619906976478926848692873900859324F;

const sphere_t atmosphere = 
{
	vec3(0, 0, 0), atmosphere_radius, 0
};

const int num_samples = 16;
const int num_samples_light = 8;

bool get_sun_light( ray_t ray,
	                inout float optical_depthR,
	                inout float optical_depthM)
{
	float t0, t1;
	isect_sphere(ray, atmosphere, t0, t1);

	float march_pos = 0.;
	float march_step = t1 / float(num_samples_light);

	for (int i = 0; i < num_samples_light; i++) {
		vec3 s =
			ray.origin +
			ray.direction * (march_pos + 0.5 * march_step);
		float height = length(s) - earth_radius;
		if (height < 0.)
			return false;

		optical_depthR += exp(-height / hR) * march_step;
		optical_depthM += exp(-height / hM) * march_step;

		march_pos += march_step;
	}

	return true;
}

vec3 get_incident_light(ray_t ray)
{
	// "pierce" the atmosphere with the viewing ray
	float t0, t1;
	if (!isect_sphere(
		ray, atmosphere, t0, t1)) {
		return vec3(0);
	}

	float march_step = t1 / float(num_samples);

	// cosine of angle between view and light directions
	float mu = dot(ray.direction, -sun_direction);

	// Rayleigh and Mie phase functions
	// A black box indicating how light is interacting with the material
	// Similar to BRDF except
	// * it usually considers a single angle
	//   (the phase angle between 2 directions)
	// * integrates to 1 over the entire sphere of directions
	float phaseR = rayleigh_phase_func(mu);
	float phaseM = hg(mu, 0.76);

	// optical depth (or "average density")
	// represents the accumulated extinction coefficients
	// along the path, multiplied by the length of that path
	float optical_depthR = 0.;
	float optical_depthM = 0.;

	vec3 sumR = vec3(0);
	vec3 sumM = vec3(0);
	float march_pos = 0.;

	for (int i = 0; i < num_samples; i++) {
		vec3 s =
			ray.origin +
			ray.direction * (march_pos + 0.5 * march_step);
		float height = length(s) - earth_radius;

		// integrate the height scale
		float hr = exp(-height / hR) * march_step;
		float hm = exp(-height / hM) * march_step;
		optical_depthR += hr;
		optical_depthM += hm;

		// gather the sunlight
		ray_t light_ray = {s,
			-sun_direction
		};
		float optical_depth_lightR = 0.;
		float optical_depth_lightM = 0.;
		bool overground = get_sun_light(
			light_ray,
			optical_depth_lightR,
			optical_depth_lightM);

		if (overground) {
			vec3 tau =
				betaR * (optical_depthR + optical_depth_lightR) +
				betaM  * (optical_depthM + optical_depth_lightM);
			vec3 attenuation = exp(-tau);

			sumR += hr * attenuation;
			sumM += hm * attenuation;
		}

		march_pos += march_step;
	}

    vec3 ret = sun_radiance *
		(sumR * phaseR * betaR +
		sumM * phaseM * betaM);

    float sundisk = smoothstep(sunAngularDiameterCos,sunAngularDiameterCos+0.00002,mu);
    ret += sundisk*sun_radiance;    // todo: extinction

	return ret;		
}

vec3 tone_map(vec3 x) 
{
    return x/(x+1);
}

float remap(float original_value , float original_min , float original_max , float new_min , float new_max) 
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float get_density_height_gradient_for_point(vec3 point, vec3 weather_data, float relative_height)
{
    float cloudt = weather_data.z;

    float cumulus = clamp(remap(relative_height, 0.0, 0.1, 0.0, 1.0),0, 1) * clamp(remap(relative_height, 0.75, 1.0, 1.0, 0.0), 0, 1);
    float stratocumulus = clamp(remap(relative_height, 0.05, 0.1, 0.0, 1.0), 0, 1) * clamp(remap(relative_height,  0.3, 0.6, 1.0, 0.0), 0, 1); 
    float stratus = clamp(remap(relative_height, 0.0, 0.01, 0.0, 1.0), 0, 1) * clamp(remap(relative_height, 0.15, 0.3, 1.0, 0.0), 0, 1); 

    if (cloudt > 0.5)
    {
        return mix(stratocumulus, cumulus, clamp((cloudt - 0.5) * 2.0, 0.0, 1.0));
    }
    else 
    {
        return mix(stratus, stratocumulus, clamp(cloudt * 2.0, 0.0, 1.0));
    }
}

float sample_cloud_density(vec3 samplepoint, vec3 weather_data, float relative_height, bool ischeap)
{
    samplepoint += (wind_direction + vec3(0.0, 0.1, 0.0))*time*cloud_speed;

    vec4 low_frequency_noises = texture(cloud_base, samplepoint/low_freq_noise_scale);
    float low_freq_FBM = low_frequency_noises.y * 0.625 + 
                         low_frequency_noises.z * 0.250 +
                         low_frequency_noises.w * 0.125;

    float base_cloud = remap(low_frequency_noises.x, 1.0 - low_freq_FBM, 1.0, 0.0, 1.0);
    base_cloud = clamp(remap(base_cloud, 1.0 - global_cloud_coverage, 1, 0, 1), 0, 1);

    float density_height_gradient = get_density_height_gradient_for_point(samplepoint, weather_data, relative_height);
    base_cloud = base_cloud * density_height_gradient;

    float cloud_coverage = weather_data.y;
    cloud_coverage = pow(cloud_coverage, clamp(remap(relative_height, 0.6, 1.0, 1.0, mix(1.0, 0.5, anvil_bias)), 0, 1));
    if (weather_map_visualization == 1.0)
    {
        return cloud_coverage;   
    }
    float base_cloud_with_coverage = clamp(remap(base_cloud, 1.0 - cloud_coverage, 1.0, 0.0, 1.0), 0, 1);
    
    base_cloud_with_coverage *= cloud_coverage;

    float final_cloud = base_cloud_with_coverage;
    if (low_frequency_noise_visualization == 1.0)
    {
        return final_cloud;
    }

    if(!ischeap && base_cloud_with_coverage > 0.0)
    {
        // todo: curl noise?
        vec4 high_frequency_noises = texture(cloud_erosion, samplepoint/high_freq_noise_scale);

        float high_freq_FBM =     (high_frequency_noises.x * 0.625)
                                + (high_frequency_noises.y * 0.250)
                                + (high_frequency_noises.z * 0.125);

        float high_freq_noise_modifier = mix(high_freq_FBM, 1.0 - high_freq_FBM, clamp(relative_height * 10.0, 0.0, 1.0));
        final_cloud = clamp(remap(base_cloud_with_coverage, high_freq_noise_modifier * high_freq_noise_factor, 1.0, 0.0, 1.0), 0.0, 1.0); 
    }

    return final_cloud;
}

// no intersection means vec.x > vec.y (really tNear > tFar)
vec2 intersect_aabb(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax)
{
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

float phase(vec3 a, vec3 b)
{
	float costheta = dot(a, b);
    return 0.8*hg(costheta, 0.8) +0.2*hg(costheta, -0.5);
}

float phase_ms(vec3 a, vec3 b, int i)
{
	float costheta = dot(a, b);
    return 0.8*hg(costheta, 0.8*pow(c, i)) +0.2*hg(costheta, -0.5*pow(c, i));
}

vec3 ray_march_to_sun(vec3 start_point, vec3 end_point, vec3 prev_dir)
{
    float transmittance = 1.0;

    vec3 dir = normalize(end_point - start_point);
    float step_size = length(end_point - start_point)/(secondary_ray_steps + 1);

    float ph = phase(-dir, -prev_dir);


    for (int i = 0; i < secondary_ray_steps; i++)
    {
        start_point += dir*step_size;
        float relative_height = (start_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);
        vec4 weather_data = texture(weather_map, (start_point.xz - aabb_min.x)/(aabb_max.x - aabb_min.x));
        float cloud_density = sample_cloud_density(start_point, weather_data.xyz, relative_height, false);
        transmittance *= exp(-cloud_density*extinction_factor*step_size);
    }

    return transmittance * ph * sun_intensity*sun_radiance;
}

vec3 ray_march_to_sun_ms(vec3 start_point, vec3 end_point, vec3 prev_dir)
{
    float transmittance = 1.0;

    vec3 dir = normalize(end_point - start_point);
    float step_size = length(end_point - start_point)/(secondary_ray_steps + 1);

    for (int i = 0; i < secondary_ray_steps; i++)
    {
        start_point += dir*step_size;
        float relative_height = (start_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);
        vec4 weather_data = texture(weather_map, (start_point.xz - aabb_min.x)/(aabb_max.x - aabb_min.x));
        float cloud_density = sample_cloud_density(start_point, weather_data.xyz, relative_height, false);
        transmittance *= exp(-cloud_density*extinction_factor*step_size);
    }

    float retval = 0;

    for (int i = 0; i < N; i++)
    {
        float ph = phase_ms(-dir, -prev_dir, i);
        retval += pow(b, i)*ph*pow(transmittance, pow(a,i));
    }

    return retval*sun_radiance;
}

vec4 ray_march(vec3 start_point, vec3 end_point)
{  
    float transmittance = 1.0; 
    vec3 colour = vec3(0);

    vec3 dir = normalize(end_point - start_point);

    float len = length(end_point - start_point); 

    float step_size = len/(primary_ray_steps+1);

    if (use_blue_noise)
    {
        vec2 sample_uvs = uvs*(vec2(1280,720)/vec2(512,512));
        vec3 noise = texture(blue_noise, sample_uvs).rgb;
        start_point += noise*step_size;
    }

    // start marching from the beginning
    vec3 current_point = start_point;
    for (int i = 0; i < primary_ray_steps; i++)
    {
        // march forwards
        current_point += dir*step_size;

        // compute relative height in cloud layer
        float relative_height = (current_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);

        // sample extinction and scattering coefficient for current position based on cloud density
        vec4 weather_data = texture(weather_map, (current_point.xz - weather_map_min.x)/(weather_map_max.x - weather_map_min.x));
        float cloud_density = sample_cloud_density(current_point, weather_data.xyz, relative_height, false);

        float extinction_coefficient = extinction_factor*cloud_density;
        float scattering_coefficient = scattering_factor*cloud_density;

        // compute radiance from sun
        vec3 dir_to_sun = -sun_direction;
        vec2 inter = intersect_aabb(current_point, dir_to_sun, aabb_min, aabb_max);
        vec3 end_point_to_sun = inter.y*dir_to_sun + current_point;

        vec3 radiance;
        if (multiple_scattering_approximation)
        {
             radiance =  ray_march_to_sun_ms(current_point, end_point_to_sun, dir);
        }
        else 
        {
             radiance =  ray_march_to_sun(current_point, end_point_to_sun, dir);
        }

        // compute current  and extinction contribution
        float current_transmittance = exp(-extinction_coefficient*step_size);
        vec3 current_scattering = scattering_coefficient*(radiance - radiance*current_transmittance)/max(extinction_coefficient,  0.0000001);

        // accumulate scattering and extinction
        colour += transmittance*current_scattering;

        transmittance *= current_transmittance;
    }

    return vec4(colour, transmittance);
}

void main()
{
    vec3 colour = vec3(0.53, 0.81, 0.92);

    // calculate ray in world space
    float x = uvs.x*2.0 - 1.0;
    float y = uvs.y*2.0 - 1.0;
    float z = -1.0;
    vec4 ray_clip = vec4(x, y, z, 1.0);
    vec4 ray_eye = inverse(projection)*ray_clip;
    ray_eye = vec4(ray_eye.xy, z, 0.0);
    vec3 ray_world = normalize((inverse(view)*ray_eye).xyz);
   
    ray_t ray = {vec3(0, earth_radius, 0), ray_world};

    colour = get_incident_light(ray);

    if (dot(ray_world, -sun_direction) > 0.9998)
        colour += sun_radiance;


    // calculate intersection with cloud layer
    vec2 res = intersect_aabb(camera_pos, ray_world, aabb_min, aabb_max);

    if (res.y > 0 && res.y > res.x)
    {
        // interesection exists
        // calculate start and end points for the ray march
        res.x = max(res.x, 0);
        vec3 start_point = res.x * ray_world + camera_pos;
        vec3 end_point = res.y * ray_world + camera_pos;
        
        if (length(end_point - start_point) > primary_ray_steps) 
        {

            vec4 rm = ray_march(start_point, end_point);

            // combine with source colour
            colour = colour.rgb*rm.a + rm.rgb;
        }
    }

    fragment_colour = vec4(colour, 1.0);
}
