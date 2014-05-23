// Copyright (c) 2014, Tamas Csala

#include "skybox.h"

using namespace oglwrap;
using gl = oglwrap::Context;

constexpr float day_duration = 256.0f;
constexpr float day_start = day_duration/16;

Skybox::Skybox()
  : time_(day_start)
  , vs_("skybox.vert")
  , fs_("skybox.frag")
  , uProjectionMatrix_(prog_, "uProjectionMatrix")
  , uCameraMatrix_(prog_, "uCameraMatrix")
  , uSunData_(prog_, "uSunData")
  , sky_fs_("sky.frag")
  , env_map(env_map_)
  , sky_fs(sky_fs_) {

  prog_ << vs_ << fs_ << sky_fs_;
  prog_.link().use();

  UniformSampler(prog_, "uEnvMap").set(0);

  prog_.validate();

  cube_.setupPositions(prog_ | "aPosition");

  env_map_.active(0);
  {
    env_map_.bind();
    env_map_.minFilter(MinFilter::Linear);
    env_map_.magFilter(MagFilter::Linear);
    env_map_.wrapS(WrapMode::ClampToEdge);
    env_map_.wrapT(WrapMode::ClampToEdge);
    env_map_.wrapP(WrapMode::ClampToEdge);

    for (int i = 0; i < 6; i++) {
      // The cloud map is not in srgb
      env_map_.loadTexture(i, "textures/skybox_" + std::to_string(i) + ".png", "RGBA");
    }
  }
}

void Skybox::update(float time) {
  time_ = time + day_start;
}

glm::vec3 Skybox::getSunPos() const {
  return glm::vec3(0.f, 1.f, 0.f) * float(1e10 * sin(time_ * 2 * M_PI / day_duration)) +
         glm::vec3(0.f, 0.f, -1.f) * float(1e10 * cos(time_ * 2 * M_PI / day_duration));
}

glm::vec4 Skybox::getSunData() const {

  float daytime = fmod(time_, day_duration) / day_duration;

  static bool day = true; // day/night
  static float lastSwitch = 0.0f;
  if (0.7f < daytime && daytime < 0.8f && 0.5f < (time_ - lastSwitch) / day_duration) {
    day = !day;
    lastSwitch = time_;
  }

  static float dayLerp = 1.0f;
  // The transition happens between the daytimes of 0.7 and 0.8
  const float day_Night_Transition_Lenght = 0.1f * day_duration;

  static float lastTime = time_;
  float timeDiff = time_ - lastTime;
  lastTime = time_;

  if (day && dayLerp < 1.0f) {
    dayLerp += timeDiff / day_Night_Transition_Lenght;
    if (dayLerp > 1.0f) {
      dayLerp = 1.0f;
    }
  } else if (!day && 0.0f < dayLerp) {
    dayLerp -= timeDiff / day_Night_Transition_Lenght;
    if (dayLerp < 0.0f) {
      dayLerp = 0.0f;
    }
  }

  return glm::vec4(getSunPos(), dayLerp);
}

void Skybox::render(float time, const engine::Camera& cam) {

  // We don't need the camMat's translation part for the skybox
  const float* f = glm::value_ptr(cam.matrix());
  glm::mat3 camRot = glm::mat3(
    f[0], f[1], f[2],
    f[4], f[5], f[6],
    f[8], f[9], f[10]
  );

  prog_.use();
  uCameraMatrix_ = camRot;
  uProjectionMatrix_ = cam.projectionMatrix();
  uSunData_ = getSunData();

  env_map_.active(0);
  env_map_.bind();
  gl::DepthMask(false);

  cube_.render();

  gl::DepthMask(true);
  env_map_.active(0);
  env_map_.unbind();
}