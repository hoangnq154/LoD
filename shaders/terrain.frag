// Copyright (c) 2014, Tamas Csala

#version 120

// This might be overwritten by the c++ code.
#define SHADOW_MAP_NUM 32

varying vec3  w_vNormal;
varying vec3  c_vPos, w_vPos;
varying vec2  vTexcoord;
varying float vInvalid;
varying mat3  vNormalMatrix;

uniform mat4 uCameraMatrix;
uniform sampler2D uGrassMap0, uGrassMap1, uGrassNormalMap;
uniform sampler2DShadow uShadowMap;
uniform vec3 uScales;

uniform mat4 uShadowCP[SHADOW_MAP_NUM];
uniform int uNumUsedShadowMaps;
uniform ivec2 uShadowAtlasSize;

// External functions
vec3 AmbientDirection();
float SunPower();
float AmbientPower();
vec3 AmbientColor();
float isDay();

float kFogMin = max(uScales.x, uScales.z) * 128.0;
float kFogMax = max(uScales.x, uScales.z) * 2048.0;

const float kSpecularShininess = 20.0;

// We love #version 120
int min(int a, int b) {
  return a > b ? b : a;
}

// -------======{[ Shadow ]}======-------

// The maximum potion of light that should be subtracted
// if the object is in shadow. For ex. 0.8 means, object in
// shadow is 20% as bright as a lit one.
const float kMaxShadow = 0.8;

vec2 GetShadowAtlasOffset(int i) {
  return vec2(i / uShadowAtlasSize.x, mod(i, uShadowAtlasSize.y));
}

vec2 AtlasLookup(vec2 tc, int i) {
  return (tc + GetShadowAtlasOffset(i)) / uShadowAtlasSize;
}

bool isValid(vec2 tc) {
  return 0 <= tc.x && tc.x <= 1 && 0 <= tc.y && tc.y <= 1;
}

float Visibility() {
  float bias = 0.01;
  float visibility = 1.0;
  int num_shadow_casters = min(uNumUsedShadowMaps, SHADOW_MAP_NUM);

  // For every shadow casters
  for (int i = 0; i < num_shadow_casters; ++i) {
    vec4 shadowCoord = uShadowCP[i] * vec4(w_vPos, 1.0);

    if (!isValid(shadowCoord.xy)) {
      continue;
    }

    visibility -= kMaxShadow * (1 - shadow2D(
      uShadowMap,
      vec3(
        AtlasLookup(shadowCoord.xy, i),
        (shadowCoord.z - bias) / shadowCoord.w
      )
    ).r);
  }

  return max(visibility, 0.0);
}

void main() {
  if (vInvalid != 0.0) {
    discard;
  }

  // TexCoord
  vec2 grass_texcoord = vTexcoord * 200;

  // Normals
  vec3 w_normal = normalize(w_vNormal);
  vec3 normal_offset = texture2D(uGrassNormalMap, grass_texcoord).rgb;
  vec3 w_final_normal = normalize(vNormalMatrix * normal_offset);
  vec3 c_normal = (uCameraMatrix * vec4(w_final_normal, 0.0)).xyz;

  // Lighting directions
  vec3 c_light_dir = normalize((uCameraMatrix * vec4(AmbientDirection(), 0)).xyz);
  vec3 c_view_direction = normalize(-(uCameraMatrix * vec4(w_vPos, 1)).xyz);

  // Lighting values
  float diffuse_power = dot(c_normal, c_light_dir);
  float specular_power;
  if (diffuse_power <= 0.0) {
    diffuse_power = 0;
    specular_power = 0;
  } else {
    vec3 L = c_light_dir, V = c_view_direction;
    vec3 H = normalize(L + V), N = c_normal;
    specular_power = pow(max(dot(H, N), 0), kSpecularShininess);
  }

  // Colors
  vec3 grass0_color = texture2D(uGrassMap0, grass_texcoord).rgb;
  vec3 grass1_color = texture2D(uGrassMap1, grass_texcoord).rgb;
  float height_factor = clamp(sqrt((w_vPos.y - 15 * uScales.y) / 40), 0, 1);
  vec3 grass_color = mix(grass0_color, grass1_color, height_factor);

  float length_from_camera = length(c_vPos);

  const float ambient_occlusion = 0.2f;

  vec3 final_color = grass_color * AmbientColor() *
    (Visibility()*SunPower()*(specular_power + diffuse_power + ambient_occlusion) + AmbientPower());

  // Fog
  vec3 fog_color = vec3(mix(-1.6f, 0.8f, isDay()));
  vec3 fog = AmbientColor() * fog_color * (0.005 + SunPower());
  float alpha = clamp((length_from_camera - kFogMin) / (kFogMax - kFogMin), 0, 1) / 4;

  gl_FragColor = vec4(mix(pow(final_color, vec3(0.7)), fog, alpha), 1);
}
