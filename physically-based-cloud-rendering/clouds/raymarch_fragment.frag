#version 460 core

out vec4 fragment_colour;

in vec2 uvs;

uniform sampler2D weather_map;
uniform sampler2D blue_noise;

uniform sampler3D cloud_base;
uniform sampler3D cloud_erosion;
uniform sampler1D mie_texture;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_pos;

uniform int weather_map_visualization;
uniform int low_frequency_noise_visualization;
uniform int high_frequency_noise_visualization;
uniform float weather_map_scale;
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
uniform bool use_ambient;
uniform vec3 ambient_radiance;
uniform float turbidity;

const float pi = 3.141592653589793238462643383279502884197169;
const float one_over_pi = 1.0/pi;
const vec3 aabb_min = vec3(-30000, 1500, -30000);
const vec3 aabb_max = vec3(30000, 4000, 30000);
const vec2 weather_map_min = vec2(-30000, -30000);
const vec2 weather_map_max = vec2(30000, 30000);

const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

const vec3 sun_radiance = vec3(69000, 64000, 59000);

float hg(float costheta, float g) 
{
    return 0.25 * one_over_pi * (1 - pow(g, 2.0)) / pow((1 + pow(g, 2.0) - 2 * g * costheta), 1.5);
}

const float sun_angular_diameter_cos = 0.999956676946448443553574619906976478926848692873900859324F;

vec3 YxyToXYZ( in vec3 Yxy )
{
	float Y = Yxy.r;
	float x = Yxy.g;
	float y = Yxy.b;

	float X = x * ( Y / y );
	float Z = ( 1.0 - x - y ) * ( Y / y );

	return vec3(X,Y,Z);
}

vec3 XYZToRGB( in vec3 XYZ )
{
	// CIE/E
	mat3 M = mat3
	(
		 2.3706743, -0.9000405, -0.4706338,
		-0.5138850,  1.4253036,  0.0885814,
 		 0.0052982, -0.0146949,  1.0093968
	);

	return XYZ * M;
}


float saturatedDot( in vec3 a, in vec3 b )
{
	return max( dot( a, b ), 0.0 );   
}

vec3 YxyToRGB( in vec3 Yxy )
{
	vec3 XYZ = YxyToXYZ( Yxy );
	vec3 RGB = XYZToRGB( XYZ );
	return RGB;
}

void calculatePerezDistribution( in float t, out vec3 A, out vec3 B, out vec3 C, out vec3 D, out vec3 E )
{
	A = vec3(  0.1787 * t - 1.4630, -0.0193 * t - 0.2592, -0.0167 * t - 0.2608 );
	B = vec3( -0.3554 * t + 0.4275, -0.0665 * t + 0.0008, -0.0950 * t + 0.0092 );
	C = vec3( -0.0227 * t + 5.3251, -0.0004 * t + 0.2125, -0.0079 * t + 0.2102 );
	D = vec3(  0.1206 * t - 2.5771, -0.0641 * t - 0.8989, -0.0441 * t - 1.6537 );
	E = vec3( -0.0670 * t + 0.3703, -0.0033 * t + 0.0452, -0.0109 * t + 0.0529 );
}

vec3 calculateZenithLuminanceYxy( in float t, in float thetaS )
{
	float chi  	 	= ( 4.0 / 9.0 - t / 120.0 ) * ( pi - 2.0 * thetaS );
	float Yz   	 	= ( 4.0453 * t - 4.9710 ) * tan( chi ) - 0.2155 * t + 2.4192;

	float theta2 	= thetaS * thetaS;
    float theta3 	= theta2 * thetaS;
    float T 	 	= t;
    float T2 	 	= t * t;

	float xz =
      ( 0.00165 * theta3 - 0.00375 * theta2 + 0.00209 * thetaS + 0.0)     * T2 +
      (-0.02903 * theta3 + 0.06377 * theta2 - 0.03202 * thetaS + 0.00394) * T +
      ( 0.11693 * theta3 - 0.21196 * theta2 + 0.06052 * thetaS + 0.25886);

    float yz =
      ( 0.00275 * theta3 - 0.00610 * theta2 + 0.00317 * thetaS + 0.0)     * T2 +
      (-0.04214 * theta3 + 0.08970 * theta2 - 0.04153 * thetaS + 0.00516) * T +
      ( 0.15346 * theta3 - 0.26756 * theta2 + 0.06670 * thetaS + 0.26688);

	return vec3( Yz, xz, yz );
}

vec3 calculatePerezLuminanceYxy( in float theta, in float gamma, in vec3 A, in vec3 B, in vec3 C, in vec3 D, in vec3 E )
{
	return ( 1.0 + A * exp( B / cos( theta ) ) ) * ( 1.0 + C * exp( D * gamma ) + E * cos( gamma ) * cos( gamma ) );
}

vec3 calculateSkyLuminanceRGB( in vec3 s, in vec3 e, in float t )
{
	vec3 A, B, C, D, E;
	calculatePerezDistribution( t, A, B, C, D, E );

	float thetaS = acos( saturatedDot( s, vec3(0,1,0) ) );
	float thetaE = acos( saturatedDot( e, vec3(0,1,0) ) );
	float gammaE = acos( saturatedDot( s, e )		   );

	vec3 Yz = calculateZenithLuminanceYxy( t, thetaS );

	vec3 fThetaGamma = calculatePerezLuminanceYxy( thetaE, gammaE, A, B, C, D, E );
	vec3 fZeroThetaS = calculatePerezLuminanceYxy( 0.0,    thetaS, A, B, C, D, E );

	vec3 Yp = Yz * ( fThetaGamma / fZeroThetaS );

	return YxyToRGB( Yp );
}

vec3 ambient = 683.0F*(calculateSkyLuminanceRGB(-sun_direction, vec3(0, 1, 0), turbidity) + calculateSkyLuminanceRGB(-sun_direction, vec3(1, 0, 0), turbidity) + calculateSkyLuminanceRGB(-sun_direction, vec3(-1, 0, 0), turbidity) + calculateSkyLuminanceRGB(-sun_direction, vec3(0, 0, 1), turbidity) + calculateSkyLuminanceRGB(-sun_direction, vec3(0, 0, -1), turbidity))/5;

float remap(float original_value , float original_min , float original_max , float new_min , float new_max) 
{
    return new_min + (((original_value - original_min) / (original_max - original_min)) * (new_max - new_min));
}

float get_height_relative_to_cloud_type(float relative_height, float cloud_type)
{
    float stratocumulus = 0.25; 
    float cumulus_and_stratus = 0.0;

    if (cloud_type == 1.0 || cloud_type == 0.0)
    {
        return relative_height - cumulus_and_stratus;
    }

    return clamp(relative_height - stratocumulus, 0, 1);
}

float get_height_coverage(float relative_height, float cloud_type)
{
    float cumulus_and_stratus = 0.0;
    float stratocumulus = 0.25; 
    float stratus = 0.0; 

    if (cloud_type == 0.5)
    {
        float test = relative_height - stratocumulus;
        if (test < 0) return 0;
        return clamp(remap(relative_height - stratocumulus, 0, 0.25, 1.25, 1), 1,1.25);
    }
    else 
    {
        return clamp(remap(relative_height - cumulus_and_stratus, 0, 0.25, 1.25, 1), 1,1.25);
    }
}

float get_height_gradient(float relative_height, float cloud_type)
{
    if (cloud_type == 1.0)
    {
        return clamp(remap(relative_height, 0.0, 0.15, 0.0, 1.0),0, 1) * clamp(remap(relative_height, 0.5, 0.9, 1.0, 0.0), 0, 1);
    }
    if (cloud_type == 0.5)
    {
        return clamp(remap(relative_height, 0.25, 0.3, 0.0, 1.0), 0, 1) * clamp(remap(relative_height,  0.55, 0.6, 1.0, 0.0), 0, 1); 
    }
    else 
    {
        return clamp(remap(relative_height, 0.0, 0.05, 0.0, 1.0), 0, 1) * clamp(remap(relative_height, 0.2, 0.3, 1.0, 0.0), 0, 1); 
    }
}

float get_coverage(float relative_height, vec3 weather_data)
{
    float cloudt = weather_data.z;
    float cloud_coverage_x = weather_data.x;
    float cloud_coverage_y = weather_data.y;

    float cloud_coverage = mix(cloud_coverage_x, cloud_coverage_y, global_cloud_coverage);
    float retval = cloud_coverage*get_height_coverage(relative_height, cloudt);

    return retval;
}


float sample_cloud_density(vec3 samplepoint, vec3 weather_data, float relative_height)
{
    samplepoint += (wind_direction)*time*cloud_speed;

    vec4 low_frequency_noises = texture(cloud_base, samplepoint/low_freq_noise_scale);
    float low_freq_FBM = low_frequency_noises.y * 0.625 + 
                         low_frequency_noises.z * 0.250 +
                         low_frequency_noises.w * 0.125;

    float base_cloud = clamp(remap(low_frequency_noises.x, -(1-low_freq_FBM), 1.0, 0.0, 1.0), 0, 1);
    base_cloud *= get_height_gradient(relative_height, weather_data.z);

    float coverage = get_coverage(relative_height, weather_data);
    float anvil_factor = clamp(remap(relative_height, 0.6, 1.0, 1.0, mix(1.0, 0.1, anvil_bias)), 0.1, 1.0);
    coverage = pow(coverage, anvil_factor);

    float base_cloud_with_coverage = clamp(remap(base_cloud,  1 - coverage, 1, 0, 1), 0 , 1);
    base_cloud_with_coverage *=  coverage;

    float final_cloud = base_cloud_with_coverage;
    if (low_frequency_noise_visualization == 1.0)
    {
        return final_cloud;
    }

    if(final_cloud > 0.0)
    {
        // todo: curl noise?
        vec4 high_frequency_noises = texture(cloud_erosion, samplepoint/high_freq_noise_scale);
        float high_freq_FBM =     (high_frequency_noises.x * 0.625)
                                + (high_frequency_noises.y * 0.250)
                                + (high_frequency_noises.z * 0.125);

        float high_freq_noise_modifier = mix(high_freq_FBM,  1 - high_freq_FBM, clamp(get_height_relative_to_cloud_type(relative_height, weather_data.b)* 10.0, 0.0, 1.0));
        final_cloud = clamp(remap(final_cloud, high_freq_noise_modifier * high_freq_noise_factor, 1.0, 0.0, 1.0), 0.0, 1.0); 
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

vec3 phase(vec3 a, vec3 b)
{
	float costheta = dot(a, b);
    return vec3(0.8*hg(costheta, 0.8) +0.2*hg(costheta, -0.5));
}

vec3 phase_ms(vec3 a, vec3 b, int i)
{
	float costheta = dot(a, b);
    float theta = acos(costheta);
    if (i == 0)
    {
        return texture1D(mie_texture, theta/pi).rgb;
    }
    else 
    {
        return vec3(0.8*hg(costheta, 0.8*pow(c, i)) +0.2*hg(costheta, -0.5*pow(c, i)));
    }
}

vec3 ray_march_to_sun(vec3 start_point, vec3 end_point, vec3 prev_dir)
{
    float transmittance = 1.0;

    vec3 dir = normalize(end_point - start_point);
    float step_size = length(end_point - start_point)/(secondary_ray_steps + 1);

    vec3 ph = phase(-dir, -prev_dir);


    for (int i = 0; i < secondary_ray_steps; i++)
    {
        start_point += dir*step_size;
        float relative_height = (start_point.y - aabb_min.y)/(aabb_max.y - aabb_min.y);
        vec4 weather_data = texture(weather_map, (start_point.xz)/(weather_map_scale));
        float cloud_density = sample_cloud_density(start_point, weather_data.xyz, relative_height);
        transmittance *= exp(-cloud_density*extinction_factor*step_size);
    }

    if (use_ambient)
    {
        return transmittance*(sun_radiance* ph + ambient_radiance);
    }
    else 
    {
        return transmittance * ph*sun_radiance;
    }
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
        vec4 weather_data = texture(weather_map, (start_point.xz)/(weather_map_scale));
        float cloud_density = sample_cloud_density(start_point, weather_data.xyz, relative_height);
        transmittance *= exp(-cloud_density*extinction_factor*step_size);
    }

    vec3 retval = vec3(0);

    for (int i = 0; i < N; i++)
    {
        vec3 ph = phase_ms(-dir, -prev_dir, i);
        if (use_ambient)
        {
            retval += pow(b, i)*pow(transmittance, pow(a,i))*(ph*sun_radiance + ambient_radiance);
        }
        else 
        {
            retval += pow(b, i)*pow(transmittance, pow(a,i))*(ph*sun_radiance);
        }
    }

    return retval;
}

vec4 ray_march(vec3 start_point, vec3 end_point)
{  
    float transmittance = 1.0; 
    vec3 colour = vec3(0);

    vec3 dir = normalize(end_point - start_point);

    float len = length(end_point - start_point); 

    float step_size = len/(primary_ray_steps+1);

    if (use_blue_noise == 1.0)
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
        vec4 weather_data = texture(weather_map, (current_point.xz)/(weather_map_scale));
        float cloud_density = sample_cloud_density(current_point, weather_data.xyz, relative_height);

        float extinction_coefficient = extinction_factor*cloud_density;
        float scattering_coefficient = scattering_factor*cloud_density;

        // compute radiance from sun
        vec3 dir_to_sun = -sun_direction;

        vec2 inter = intersect_aabb(current_point, dir_to_sun, aabb_min, aabb_max);
        vec3 end_point_to_sun = inter.y*dir_to_sun + current_point;

        vec3 radiance;
        if (multiple_scattering_approximation == 1.0)
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
    // calculate ray in world space
    float x = uvs.x*2.0 - 1.0;
    float y = uvs.y*2.0 - 1.0;
    float z = -1.0;
    vec4 ray_clip = vec4(x, y, z, 1.0);
    vec4 ray_eye = inverse(projection)*ray_clip;
    ray_eye = vec4(ray_eye.xy, z, 0.0);
    vec3 ray_world = normalize((inverse(view)*ray_eye).xyz);

    vec3 colour = calculateSkyLuminanceRGB( -sun_direction, ray_world, turbidity );

    float sundisk = smoothstep(sunAngularDiameterCos,sunAngularDiameterCos+0.0002,dot(ray_world, -sun_direction));
    colour += sundisk*10*sun_radiance/683.0F;

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
            colour = colour.rgb*rm.a + rm.rgb/683.0F;
        }
    }

    fragment_colour = vec4(colour, 1.0);
}
