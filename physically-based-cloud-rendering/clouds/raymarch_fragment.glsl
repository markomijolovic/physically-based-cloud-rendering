#version 460 core

out vec4 fragment_colour;

in vec2 uvs;

uniform sampler2D full_screen;
uniform sampler2D weather_map;

uniform sampler3D cloud_base;
uniform sampler3D cloud_erosion;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_pos;

uniform float weather_map_visualization;
uniform float low_frequency_noise_visualization;
uniform float high_frequency_noise_visualization;
uniform float noise_scale;
uniform float scattering_factor;
uniform float extinction_factor;

const float pi = 3.14159265;
const float one_over_pi = 0.3183099;
const vec3 aabb_min = vec3(-30000, 1500, -30000);
const vec3 aabb_max = vec3(30000, 4000, 30000);
const vec3 sun_pos = vec3(30000, 100000, 30000);

float remap(float original_value , float original_min , float original_max , float new_min , float new_max) 
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float get_density_height_gradient_for_point(vec3 point, vec3 weather_data, float relative_height)
{
    float cloudt = weather_data.z;

    float cumulus = max(0.0, remap(relative_height, 0.01, 0.3, 0.0, 1.0) * remap(relative_height, 0.6, 0.95, 1.0, 0.0));
    float stratocumulus = max(0.0, remap(relative_height, 0.0, 0.25, 0.0, 1.0) * remap(relative_height,  0.3, 0.65, 1.0, 0.0)); 
    float stratus = max(0.0, remap(relative_height, 0, 0.1, 0.0, 1.0) * remap(relative_height, 0.2, 0.3, 1.0, 0.0)); 

    float a = mix(stratus, stratocumulus, clamp(cloudt * 2.0, 0.0, 1.0));

    float b = mix(stratocumulus, stratus, clamp((cloudt - 0.5) * 2.0, 0.0, 1.0));
    return mix(a, b, cloudt);
}

float sample_cloud_density(vec3 samplepoint, vec3 weather_data, float relative_height, bool ischeap)
{
    vec4 low_frequency_noises = texture(cloud_base, samplepoint);
    float low_freq_FBM = low_frequency_noises.y * 0.625 + 
                         low_frequency_noises.z * 0.250 +
                         low_frequency_noises.w * 0.125;

    float base_cloud = remap(low_frequency_noises.x, -(1.0 - low_freq_FBM), 1.0, 0.0, 1.0);

    float density_height_gradient = get_density_height_gradient_for_point(samplepoint, weather_data, relative_height);
    base_cloud = base_cloud * density_height_gradient;
    
    float cloud_coverage = weather_data.x;
    if (weather_map_visualization == 1.0)
    {
        return cloud_coverage;   
    }
    float base_cloud_with_coverage = remap(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);
    base_cloud_with_coverage *= cloud_coverage;
    base_cloud_with_coverage = clamp(base_cloud_with_coverage, 0.0, 1.0);

    float final_cloud = base_cloud_with_coverage;
    if (low_frequency_noise_visualization == 1.0)
    {
        return final_cloud;
    }
    if(!ischeap && base_cloud_with_coverage > 0.0)
    {
        // todo: curl noise?
        vec4 high_frequency_noises = texture(cloud_erosion, samplepoint);

        float high_freq_FBM =     (high_frequency_noises.x * 0.625)
                                + (high_frequency_noises.y * 0.250)
                                + (high_frequency_noises.z * 0.125);

        float high_freq_noise_modifier = mix(high_freq_FBM, 1.0 - high_freq_FBM, clamp(relative_height * 10.0, 0.0, 1.0));
        final_cloud = remap(base_cloud_with_coverage, high_freq_noise_modifier * 0.1, 1.0, 0.0, 1.0); 
        final_cloud = clamp(final_cloud, 0.0, 1.0);
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

float hg(float costheta, float g) 
{
    return 0.25 * one_over_pi * (1 - pow(g, 2.0)) / pow((1 + pow(g, 2.0) - 2 * g * costheta), 1.5);
}

float phase(vec3 a, vec3 b)
{
	float costheta = dot(a, b);
	return hg(costheta, 0.8);
    //return 0.75*hg(costheta, 0.8) +0.25*hg(costheta, (2.0/3.0)*0.8);
}

float ray_march_to_sun(vec3 start_point, vec3 end_point, vec3 prev_dir)
{
    float density = 0;

    vec3 dir = normalize(end_point - start_point);
    float step_size = length(end_point - start_point)/17;

    float ph = phase(-dir, -prev_dir);

    for (int i = 0; i < 16; i++)
    {
        start_point += dir*step_size;
        float relative_height = (start_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);
        vec4 weather_data = texture(weather_map, (start_point.xz - aabb_min.x)/(aabb_max.x - aabb_min.x));
        float cloud_density = sample_cloud_density(start_point/noise_scale, weather_data.xyz, relative_height, false);
        density += cloud_density;
    }

    float scatter = exp(-density*extinction_factor);
    return scatter * ph;
}


vec4 ray_march(vec3 start_point, vec3 end_point)
{  
    float extinction = 1.0; 
    vec3 scattering = vec3(0);

    vec3 dir = normalize(end_point - start_point);

    float step_size = length(end_point - start_point)/129;

    // start marching from the end
    vec3 current_point = end_point;
    for (int i = 0; i < 128; i++)
    {
        // march backwards
        current_point -= dir*step_size;

        // compute relative height in cloud layer
        float relative_height = (current_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);

        // sample extinction and scattering coefficient for current position based on cloud density
        vec4 weather_data = texture(weather_map, (current_point.xz - aabb_min.x)/(aabb_max.x - aabb_min.x));
        float cloud_density = sample_cloud_density(current_point/noise_scale, weather_data.xyz, relative_height, false);

        float extinction_coefficient = extinction_factor*cloud_density;
        float scattering_coefficient = scattering_factor*cloud_density;

        // compute radiance from sun
        vec3 dir_to_sun = normalize(sun_pos - current_point);
        vec2 inter = intersect_aabb(current_point, dir_to_sun, aabb_min, aabb_max);
        vec3 end_point_to_sun = inter.y*dir_to_sun + current_point;
        float radiance = ray_march_to_sun(current_point, end_point_to_sun, dir);

        // compute current  and extinction contribution
        float current_scattering = step_size*radiance*scattering_coefficient;
        float current_extinction = exp(-extinction_coefficient*step_size);

        // accumulate scattering and extinction
        extinction *= current_extinction;
        scattering += current_extinction*current_scattering;
    }

    return vec4(scattering, extinction);
}

void main()
{
    vec4 source = texture(full_screen, uvs);
    if (source.g == 0)
    {
        // quick and dirty test lol
        source = vec4(0.53,0.81,0.92, 1);
    }

    // calculate ray in world space
    float x = uvs.x*2.0 - 1.0;
    float y = uvs.y*2.0 - 1.0;
    float z = -1.0;
    vec4 ray_clip = vec4(x, y, z, 1.0);
    vec4 ray_eye = inverse(projection)*ray_clip;
    ray_eye = vec4(ray_eye.xy, z, 0.0);
    vec3 ray_world = normalize((inverse(view)*ray_eye).xyz);

    // calculate intersection with cloud layer
    vec2 res = intersect_aabb(camera_pos, ray_world, aabb_min, aabb_max);

    if (res.y > 0 && res.y > res.x)
    {
        // interesection exists
        // calculate start and end points for the ray march
        res.x = max(res.x, 0);
        vec3 start_point = res.x * ray_world + camera_pos;
        vec3 end_point = res.y * ray_world + camera_pos;
        
        vec4 rm = ray_march(start_point, end_point);

        // combine with source colour
        fragment_colour = vec4(source.rgb*rm.a + rm.rgb, 1);
    }
    else 
    {
        // no intersection
        fragment_colour = source;
    }
}
