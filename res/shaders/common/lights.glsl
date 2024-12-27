#ifndef NUM_PL
#define NUM_PL 0
#endif
////////////////////////////////////////////////// TASK H //////////////////////////////////////////////////////////////
#ifndef NUM_DL
#define NUM_DL 0
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Material Properties
struct Material {
    vec3 diffuse_tint;
    vec3 specular_tint;
    vec3 ambient_tint;
    float shininess;
};

// Light Data
struct LightCalculatioData {
    vec3 ws_frag_position;
    vec3 ws_view_dir;
    vec3 ws_normal;
};

struct PointLightData {
    vec3 position;
    vec3 colour;
};
////////////////////////////////////////////////// TASK H //////////////////////////////////////////////////////////////
struct DirectionalLightData {
    vec3 direction;
    vec3 colour;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculations
const float ambient_factor = 0.002f;
////////////////////////////////////////////////// TASK F //////////////////////////////////////////////////////////////
const vec3 attenuation_params = vec3(0.1, 0.1, 0.3); // Hardcoded attenuation coefficients a, b, c
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Point Lights
void point_light_calculation(PointLightData point_light, LightCalculatioData calculation_data, float shininess, inout vec3 total_diffuse, inout vec3 total_specular, inout vec3 total_ambient) {
    vec3 ws_light_offset = point_light.position - calculation_data.ws_frag_position;

    ////////////////////////////////////////////////// TASK F //////////////////////////////////////////////////////////////
    float distance = length(ws_light_offset);

    // Attenuation
    float attenuation = 1.0 / (attenuation_params.x + attenuation_params.y * distance + attenuation_params.z * distance * distance);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Ambient
    ////////////////////////////////////////////////// TASK F //////////////////////////////////////////////////////////////
    vec3 ambient_component = attenuation * ambient_factor * point_light.colour;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Diffuse
    vec3 ws_light_dir = normalize(ws_light_offset);
    float diffuse_factor = max(dot(ws_light_dir, calculation_data.ws_normal), 0.0f);
    ////////////////////////////////////////////////// TASK F //////////////////////////////////////////////////////////////
    vec3 diffuse_component = attenuation * diffuse_factor * point_light.colour;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Specular
    vec3 ws_halfway_dir = normalize(ws_light_dir + calculation_data.ws_view_dir);
    float specular_factor = pow(max(dot(calculation_data.ws_normal, ws_halfway_dir), 0.0f), shininess);
    ////////////////////////////////////////////////// TASK F //////////////////////////////////////////////////////////////
    vec3 specular_component = attenuation * specular_factor * point_light.colour;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    total_diffuse += diffuse_component;
    total_specular += specular_component;
    total_ambient += ambient_component;
}

////////////////////////////////////////////////// TASK H //////////////////////////////////////////////////////////////
// Directional Lights
void directional_light_calculation(DirectionalLightData directional_light, LightCalculatioData calculation_data, float shininess, inout vec3 total_diffuse, inout vec3 total_specular, inout vec3 total_ambient) {
    vec3 ws_light_dir = normalize(-directional_light.direction);  // Direction to the light is the negative of the light's direction

    // Ambient
    vec3 ambient_component = ambient_factor * directional_light.colour;

    // Diffuse
    float diffuse_factor = max(dot(ws_light_dir, calculation_data.ws_normal), 0.0f);
    vec3 diffuse_component = diffuse_factor * directional_light.colour;

    // Specular
    vec3 ws_halfway_dir = normalize(ws_light_dir + calculation_data.ws_view_dir);
    float specular_factor = pow(max(dot(calculation_data.ws_normal, ws_halfway_dir), 0.0f), shininess);
    vec3 specular_component = specular_factor * directional_light.colour;

    total_diffuse += diffuse_component;
    total_specular += specular_component;
    total_ambient += ambient_component;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Total Calculation
struct LightingResult {
    vec3 total_diffuse;
    vec3 total_specular;
    vec3 total_ambient;
};

LightingResult total_light_calculation(LightCalculatioData light_calculation_data, Material material
        #if NUM_PL > 0
        ,PointLightData point_lights[NUM_PL]
        #endif
        /////////////////////////////////////////////////// TASK H /////////////////////////////////////////////////////
        #if NUM_DL > 0
        ,DirectionalLightData directional_lights[NUM_DL]
        #endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ) {

    vec3 total_diffuse = vec3(0.0f);
    vec3 total_specular = vec3(0.0f);
    vec3 total_ambient = vec3(0.0f);

    #if NUM_PL > 0
    for (int i = 0; i < NUM_PL; i++) {
        point_light_calculation(point_lights[i], light_calculation_data, material.shininess, total_diffuse, total_specular, total_ambient);
    }
    #endif

    #if NUM_PL > 0
    total_ambient /= float(NUM_PL);
    #endif
    /////////////////////////////////////////////////// TASK H /////////////////////////////////////////////////////
    #if NUM_DL > 0
    for (int i = 0; i < NUM_DL; i++) {
        directional_light_calculation(directional_lights[i], light_calculation_data, material.shininess, total_diffuse, total_specular, total_ambient);
    }
    #endif
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    total_diffuse *= material.diffuse_tint;
    total_specular *= material.specular_tint;
    total_ambient *= material.ambient_tint;

    return LightingResult(total_diffuse, total_specular, total_ambient);
}

vec3 resolve_textured_light_calculation(LightingResult result, sampler2D diffuse_texture, sampler2D specular_map, vec2 texture_coordinate) {
    vec3 texture_colour = texture(diffuse_texture, texture_coordinate).rgb;
    vec3 specular_map_sample = texture(specular_map, texture_coordinate).rgb;

    vec3 textured_diffuse = result.total_diffuse * texture_colour;
    vec3 sampled_specular = result.total_specular * specular_map_sample;
    vec3 textured_ambient = result.total_ambient * texture_colour;

    // Mix the diffuse and ambient so that there is no ambient in bright scenes
    return max(textured_diffuse, textured_ambient) + sampled_specular;
}