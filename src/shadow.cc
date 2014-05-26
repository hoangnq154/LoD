// Copyright (c) 2014, Tamas Csala

#include "shadow.h"
#include "oglwrap/context.h"
#include "oglwrap/smart_enums.h"

namespace gl = oglwrap;
using glEnum = oglwrap::SmartEnums;

Shadow::Shadow(int shadow_map_size, int atlas_x_size, int atlas_y_size)
    : size_(shadow_map_size)
    , xsize_(atlas_x_size)
    , ysize_(atlas_y_size)
    , curr_depth_(0)
    , max_depth_(xsize_*ysize_)
    , cp_matrices_(max_depth_)  {

  // Setup the texture array that will serve as storage.
  tex_.bind();
  tex_.upload(
    glEnum::DepthComponent,
    size_*xsize_, size_*ysize_,
    glEnum::DepthComponent,
    glEnum::Float, nullptr
  );
  tex_.minFilter(glEnum::Linear);
  tex_.magFilter(glEnum::Linear);
  tex_.wrapS(glEnum::ClampToBorder);
  tex_.wrapT(glEnum::ClampToBorder);
  tex_.borderColor(glm::vec4(1.0f));
  tex_.compareFunc(glEnum::Lequal);
  tex_.compareMode(glEnum::CompareRefToTexture);

  // Setup the FBO
  fbo_.bind();
  fbo_.attachTexture(glEnum::DepthAttachment, tex_, 0);
  // No color output in the bound framebuffer, only depth.
  gl::DrawBuffer(glEnum::None);
  fbo_.validate();
  fbo_.unbind();
}

void Shadow::screenResized(size_t width, size_t height) {
  w_ = width;
  h_ = height;
}

glm::dmat4 Shadow::projMat(double size) const {
  return glm::ortho<double>(-size, size, -size, size, 0, 2*size);
}

glm::dmat4 Shadow::camMat(glm::dvec3 lightSrcPos, glm::dvec4 targetBSphere) const {
  return glm::lookAt(
    glm::dvec3(targetBSphere) + glm::normalize(lightSrcPos) * targetBSphere.w,
    glm::dvec3(targetBSphere),
    glm::dvec3(0, 1, 0)
  );
}

glm::mat4 Shadow::modelCamProjMat(glm::dvec3 lightSrcPos,
                                  glm::dvec4 targetBSphere,
                                  glm::dmat4 modelMatrix,
                                  glm::dmat4 worldTransform) {
  // [-1, 1] -> [0, 1] convert
  glm::dmat4 biasMatrix(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
  );

  glm::dmat4 projMatrix = projMat(targetBSphere.w);
  glm::dvec4 offseted_targetBSphere =
    glm::dvec4(
      glm::dvec3(modelMatrix * glm::dvec4(glm::dvec3(targetBSphere), 1)),
      targetBSphere.w
    );

  glm::dmat4 pc = projMatrix * camMat(lightSrcPos, offseted_targetBSphere);

  cp_matrices_[curr_depth_] = biasMatrix * pc;

  return static_cast<glm::mat4>(pc * modelMatrix * worldTransform);
}

const std::vector<glm::mat4>& Shadow::shadowCPs() const {
  return cp_matrices_;
}

const oglwrap::Texture2D& Shadow::shadowTex() const {
  return tex_;
}

void Shadow::begin() {
  fbo_.bind();
  curr_depth_ = 0;

  // Clear the shadowmap atlas
  gl::Viewport(size_*xsize_, size_*ysize_);
  gl::Clear().Depth();

  // Setup the 0th shadowmap
  gl::Viewport(0, 0, size_, size_);
}

void Shadow::setViewPort() {
  size_t x = curr_depth_ / xsize_, y = curr_depth_ % xsize_;
  gl::Viewport(x*size_, y*size_, size_, size_);
}

void Shadow::push() {
  if (curr_depth_ < max_depth_) {
    ++curr_depth_;
    setViewPort();
  }
}

size_t Shadow::getDepth() const {
  return curr_depth_;
}

size_t Shadow::getMaxDepth() const {
  return max_depth_;
}

void Shadow::end() {
  fbo_.unbind();
  gl::Viewport(w_, h_);
}
