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

const float pi = 3.14159265;
const vec3 aabb_min = vec3(-30000, 1500, -30000);
const vec3 aabb_max = vec3(30000, 4000, 30000);

float get_gradient_height_factor(float height, int cloudtype)
{
    float timewidthup, starttimeup, starttimedown;

    if(cloudtype == 0)
    {
        // stratus
        timewidthup     = 0.08;
        starttimeup     = 0.08;
        starttimedown   = 0.2;
    }
    else if(cloudtype == 1)
    {
        // cumulus
        timewidthup     = 0.14;
        starttimeup     = 0.1;
        starttimedown   = 0.5;
    }
    else if(cloudtype == 2)
    {
        // cumulonimbus
        timewidthup     = 0.2;
        starttimeup     = 0.10;
        starttimedown   = 0.7;
    }

    float factor = 2.0 * pi/(2.0 * timewidthup);

    float density_gradient = 0.0;

    // gradient functions dependent on the cloud type
    if(height < starttimeup)
    {
        density_gradient = 0.0;
    }
    else if(height < starttimeup + timewidthup)
    {
        density_gradient = 0.5 * sin(factor * height - pi/2.0 - factor * starttimeup) + 0.5;
    }
    else if(height < starttimedown)
    {
        density_gradient = 1.0;
    }
    else if(height < starttimedown + timewidthup)
    {
        density_gradient = 0.5 * sin(factor * height - pi/2.0 - factor * (starttimedown + timewidthup)) + 0.5;
    }
    else
    {
        density_gradient = 0.0;
    }

    return density_gradient;
}

float get_density_height_gradient_for_point(vec3 point, vec3 weather_data, float relative_height){
    float cloudt = weather_data.z;
    int cloudtype = 1;  // cumulus
    if(cloudt < 0.1)
    {
        cloudtype = 0;  // stratus
    }
    else if(cloudt > 0.9)
    {
        cloudtype = 2;  // cumulonimbus
    }

    return get_gradient_height_factor(relative_height, cloudtype);
}

float remap(float original_value , float original_min , float original_max , float new_min , float new_max) 
{
	return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float sample_cloud_density(vec3 samplepoint, vec3 weather_data, float relative_height, bool ischeap){
    vec4 low_frequency_noises = texture(cloud_base, samplepoint);
    float low_freq_FBM = low_frequency_noises.y * 0.625 + 
                         low_frequency_noises.z * 0.250 +
                         low_frequency_noises.w * 0.125;

    float base_cloud = remap(low_frequency_noises.x, -(1.0 - low_freq_FBM), 1.0, 0.0, 1.0);

    float density_height_gradient = get_density_height_gradient_for_point(samplepoint, weather_data, relative_height);
    base_cloud = base_cloud * density_height_gradient;

    float cloud_coverage = weather_data.x;
    float base_cloud_with_coverage = remap(base_cloud, cloud_coverage, 1.0, 0.0, 1.0);
    base_cloud_with_coverage *= cloud_coverage;
    base_cloud_with_coverage = clamp(base_cloud_with_coverage, 0.0, 1.0);

    float final_cloud = base_cloud_with_coverage;
    if(!ischeap && base_cloud_with_coverage > 0.0)
    {
        // todo: curl noise?
        vec4 high_frequency_noises = texture(cloud_erosion, samplepoint);

        float high_freq_FBM =     (high_frequency_noises.x * 0.625)
                                + (high_frequency_noises.y * 0.250)
                                + (high_frequency_noises.z * 0.125);

        float high_freq_noise_modifier = mix(high_freq_FBM, 1.0 - high_freq_FBM, clamp(relative_height * 2.0, 0.0, 1.0));
        final_cloud = remap(base_cloud_with_coverage, high_freq_noise_modifier * 0.2, 1.0, 0.0, 1.0); 
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

vec4 ray_march(vec3 start_point, vec3 end_point)
{
    float density = 0;

    vec3 dir = normalize(end_point - start_point);
    float step_size = length(end_point - start_point)/128;

    for (int i = 0; i < 128; i++)
    {
        start_point += dir*step_size;
        float relative_height = (start_point.y - 1500)/2500;
            vec4 weather_data = texture(weather_map, (start_point.xz - 30000)/60000);
            float cloud_density = sample_cloud_density(start_point/10000, weather_data.xyz, relative_height, false);
            density += cloud_density;
    }

    return vec4(density);
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

    // calculate intersection with cloud layer
    vec2 res = intersect_aabb(camera_pos, ray_world, aabb_min, aabb_max);
    if (res.y < 0)
    {
        // intersection behind
    }
    else if (res.y > res.x)
    {
        // interesection
        res.x = max(res.x, 0);
        vec3 start_point = res.x * ray_world + camera_pos;
        vec3 end_point = res.y * ray_world + camera_pos;
        fragment_colour = ray_march(start_point, end_point)/32;
    }
    else 
    {
        // no intersection
    }
}
