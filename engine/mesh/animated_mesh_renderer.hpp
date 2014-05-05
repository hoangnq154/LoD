/** @file animated_mesh_renderer.hpp
    @brief An animation loader using assimp
*/

#ifndef ENGINE_MESH_ANIMATED_MESH_RENDERER_HPP_
#define ENGINE_MESH_ANIMATED_MESH_RENDERER_HPP_

#include <functional>
#include "mesh_renderer.hpp"
#include "anim_state.hpp"
#include "skinning_data.hpp"
#include "anim_info.hpp"
#include "oglwrap/uniform.hpp"

namespace engine {

/// A class for loading and displaying animations.
class AnimatedMeshRenderer : public MeshRenderer {

  /// Stores data related to skin definition.
  SkinningData skinning_data_;

  /// The animations.
  AnimData anims_;

  /// Stores data to handle animation transitions.
  AnimMetaInfo anim_meta_info_;

  /// The current animation.
  AnimationState current_anim_;

  /// The name of the current animation
  std::string current_anim_name_;

  /// The last animation.
  AnimationState last_anim_;

public:
  /**
   * @brief Loads in the mesh and the skeleton for an asset, and prepares it
   *        for animation.
   *
   * @param filename   The name of the file.
   * @param flags      The assimp post-process flags to use while loading the mesh.
   */
  AnimatedMeshRenderer(const std::string& filename,
                       oglwrap::Bitfield<aiPostProcessSteps> flags);

private:
  /// It shouldn't be copyable.
  AnimatedMeshRenderer(const AnimatedMeshRenderer& src) = delete;

  /// It shouldn't be copyable.
  void operator=(const AnimatedMeshRenderer& rhs) = delete;

  /*         //=====:==-==-==:=====\\                                 //=====:==-==-==:=====\\
      <---<}>==~=~=~==--==--==~=~=~==<{>----- Skin definition -----<}>==~=~=~==--==--==~=~=~==<{>--->
             \\=====:==-==-==:=====//                                 \\=====:==-==-==:=====//          */

  /// Fills the bone_mapping with data.
  void mapBones();

  /**
   * @brief A recursive functions that should be started from the root node, and
   * it returns the first bone under it.
   *
   * @param node   The current root node.
   * @param anim   The animation to seek the root bone in.
   */
  const aiNodeAnim* getRootBone(const aiNode* node, const aiScene* anim);

  template <class Index_t>
  /**
   * @brief Creates bone attributes data.
   *
   * It is a template, as the type of boneIDs shouldn't be fix. Most of the times,
   * a skeleton won't contain more than 256 bones, but that doesn't mean boneIDs
   * should be forced to GLubyte, it works with shorts and even ints too. Although
   * I really doubt anyone would be using a skeleton with more than 65535 bones...
   */
  void loadBones();

  /**
   * @brief Creates the bone attributes data (the skinning.)
   *
   * It actually just calls the loadBones function with the appropriate template
   * parameter.
   */
  void createBonesData();

  template <class Index_t>
  /**
   * Shader plumbs the bone data.
   *
   * It is a template, as the type of boneIDs shouldn't be fix. Most of the times,
   * a skeleton won't contain more than 256 bones, but that doesn't mean boneIDs
   * should be forced to GLubyte, it works with shorts and even ints too. Although
   * I really doubt anyone would be using a skeleton with more than 65535 bones...
   *
   * @param idx_t          The oglwrap enum, naming the data type that should be
   *                       used.
   * @param boneIDs        Should be an array of attributes, that will be shader
   *                       plumbed for the boneIDs data.
   * @param bone_weights   Should be an array of attributes, that will be shader
   *                       plumbed for the bone_weights data.
   * @param integerIDs     If true, boneIDs are uploaded as integers
   *                       (#version 130+) else they are uploaded as floats */
  void shaderPlumbBones(oglwrap::IndexType idx_t,
                        oglwrap::LazyVertexAttribArray boneIDs,
                        oglwrap::LazyVertexAttribArray bone_weights,
                        bool integerWeights = true);

private:

  /**
   * @brief Returns the first node called \a name, who is under \a currentRoot
   * in the bone hierarchy.
   *
   * Note: this function is recursive
   *
   * @param currentRoot   The bone under which to search.
   * @param name          The name of the bone that is to be found.
   * @return the handle to the bone that is called name, or nullptr.
   */
  aiNode* findNode(aiNode* currentRoot, const std::string& name);

  /**
   * @brief Marks all of a bone's child external recursively.
   *
   * @param parent              A pointer to the parent ExternalBone struct.
   * @param node                The current node.
   * @param should_be_external  Should be false if called from outside,
   *                            true if called recursively.
   */
  ExternalBone markChildExternal(ExternalBone* parent, aiNode* node,
                                 bool should_be_external = false);

public:

  /**
   * @brief Marks a bone to be modified from outside.
   *
   * @param bone_name - The name of the bone.
   * @return A structure, which through the bone, and all of its child can be moved.
   */
  ExternalBoneTree markBoneExternal(const std::string& bone_name);

  /**
   * @brief Returns the number of bones this scene has.
   *
   * May change the currently active VAO and ArrayBuffer at the first call.
   */
  size_t getNumBones();

  /**
   * @brief Returns the size that boneIds and BoneWeights attribute arrays
   *        should be.
   *
   * May change the currently active VAO and ArrayBuffer at the first call.
   */
  size_t getBoneAttribNum();

  /**
   * @brief Loads in bone weight and id information to the given array of
   *        attribute arrays.
   *
   * Uploads the bone weight and id to an array of attribute arrays, and sets
   * it up for use. For example if you specified "in vec4 boneIds[3]" you have
   * to give "prog | boneIds".
   *
   * Calling this function changes the currently active VAO and ArrayBuffer.
   *
   * @param boneIDs        The array of attributes array to use as destination
   *                       for bone IDs.
   * @param bone_weights   The array of attributes array to use as destination
   *                       for bone weights.
   * @param integerIDs     If true, boneIDs are uploaded as integers
   *                       (#version 130+) else they are uploaded as floats */
  void setupBones(oglwrap::LazyVertexAttribArray boneIDs,
                  oglwrap::LazyVertexAttribArray bone_weights,
                  bool integerIDs = true);

  /*         //=====:==-==-==:=====\\                           //=====:==-==-==:=====\\
      <---<}>==~=~=~==--==--==~=~=~==<{>----- Animation -----<}>==~=~=~==--==--==~=~=~==<{>--->
             \\=====:==-==-==:=====//                           \\=====:==-==-==:=====//          */

private:

  /**
   * @brief Returns the index of the currently active translation keyframe for
   *        the given animation and time.
   *
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for a keyframe.
   */
  unsigned findPosition(float anim_time, const aiNodeAnim* node_anim);

  /**
   * @brief Returns the index of the currently active rotation keyframe for
   *        the given animation and time.
   *
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for a keyframe.
   */
  unsigned findRotation(float anim_time, const aiNodeAnim* node_anim);

  /**
   * @brief Returns the index of the currently active scaling keyframe for
   *        the given animation and time.
   *
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for a keyframe.
   */
  unsigned findScaling(float anim_time, const aiNodeAnim* node_anim);

  /**
   * @brief Returns a linearly interpolated value between the previous and next
   *        translation keyframes.
   *
   * @param out         Returns the result here.
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for the keyframes.
   */
  void calcInterpolatedPosition(aiVector3D& out, float anim_time,
                                const aiNodeAnim* node_anim);

  /**
   * @brief Returns a spherically interpolated value (always choosing the shorter
   * path) between the previous and next rotation keyframes.
   *
   * @param out         Returns the result here.
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for the keyframes.
   */
  void calcInterpolatedRotation(aiQuaternion& out, float anim_time,
                                const aiNodeAnim* node_anim);

  /**
   * @brief Returns a linearly interpolated value between the previous and next
   *        scaling keyframes.
   *
   * @param out         Returns the result here.
   * @param anim_time   The time elapsed since the start of this animation.
   * @param node_anim   The animation node, in which the function should search
   *                    for the keyframes.
   */
  void calcInterpolatedScaling(aiVector3D& out, float anim_time,
                               const aiNodeAnim* node_anim);

  /**
   * @brief Returns the animation node in the given animation, referenced by
   *        its name.
   *
   * Returns nullptr if it doesn't find a node with that name,
   * which usually means that it's not a bone.
   *
   * @param animation - The animation, this function should search in.
   * @param node_name - The name of the bone to search.
   */
  const aiNodeAnim* findNodeAnim(const aiAnimation* animation,
                                 const std::string node_name);

  /**
   * @brief Recursive function that travels through the entire node hierarchy,
   *        and creates transformation values in world space.
   *
   * Bone transformations are stored relative to their parents. That's why it is
   * needed. Also note, that the translation of the root node on the XZ plane is
   * treated differently, that offset isn't baked into the animation, you can get
   * the offset with the offsetSinceLastFrame() function, and you have to
   * externally do the object's movement, as normally it will stay right where
   * it was at the start of the animation.
   *
   * @param anim_time          The current animation time.
   * @param node               The node (bone) whose, and whose child's
   *                           transformation should be updated. You should call
   *                           this function with the root node.
   * @param parent_transform   The transformation of the parent node. You should
   *                           call it with an identity matrix.
   */
  void updateBoneTree(float anim_time,
                      const aiNode* node,
                      const glm::mat4& parent_transform = glm::mat4());

  /**
   * @brief Does the same thing as readNodeHierarchy, but it is used to create
   *        transitions between animations, so it interpolates between four
   *        keyframes not two.
   *
   * @param prev_animation_time   The animation time of when, the last animation
   *                              was interrupted.
   * @param next_animation_time   The current animation time.
   * @param node                  The node (bone) whose, and whose child's
   *                              transformation should be updated. You should
   *                              call this function with the root node.
   * @param parent_transform      The transformation of the parent node. You
   *                              should call it with an identity matrix.
   */
  void updateBoneTreeInTransition(float prev_animation_time,
                                  float next_animation_time,
                                  float factor,
                                  const aiNode* node,
                                  const glm::mat4& parent_transform = glm::mat4());

public:

  /// Updates the bones' transformations.
  void updateBoneInfo(float time_in_seconds);

  /**
   * @brief Uploads the bones' transformations into the given uniform array.
   * @param bones - The uniform naming the bones array. It should be indexable.
   */
  void uploadBoneInfo(oglwrap::LazyUniform<glm::mat4>& bones);

  /**
   * @brief Updates the bones transformation and uploads them into the given
   *        uniforms.
   *
   * @param time_in_seconds  Expect a time value as a float, optimally since
   *                         the start of the program.
   * @param bones            The uniform naming the bones array. It should be
   *                         indexable.
   */
  void updateAndUploadBoneInfo(float time_in_seconds,
                               oglwrap::LazyUniform<glm::mat4>& bones);

  /*       //=====:==-==-==:=====\\                                   //=====:==-==-==:=====\\
    <---<}>==~=~=~==--==--==~=~=~==<{>----- Animation Control -----<}>==~=~=~==--==--==~=~=~==<{>--->
           \\=====:==-==-==:=====//                                   \\=====:==-==-==:=====//       */

public:
  /**
   * @brief A callback function, that is called everytime an animation ends
   *        The function should return the name of the new animation. If it
   *        returns a string that isn't a name of an animation, then the default
   *        animation will be played.
   *
   * It is typically created using std::bind, like this:
   *
   * using std::placeholders::_1;
   * auto callback = std::bind(&MyClass::animationEndedCallback, this, _1);
   *
   * Or, if you prefer you can use lambdas, to create the callback:
   *
   * auto callback2 = [this](const std::string& current_anim){
   *   return animationEndedCallback(current_anim);
   *  };
   *
   * @param current_anim  The name of the currently playing animation, that is
   *                      about to be changed.
   * @return The parameters of the new animation.
   */
  using AnimationEndedCallback =
    AnimParams(const std::string& current_anim);

private:
  /// The callback functor
  std::function<AnimationEndedCallback> anim_ended_callback_;

  /**
   * @brief The function that changes animations when they end.
   *
   * @param current_time  The current time.
   */
  void animationEnded(float current_time);

public:
  /**
   * @brief Sets the callback functor for choosing the next anim.
   *
   * Sets a callback functor that is called everytime an animation ends,
   * and is responsible for choosing the next animation.
   *
   * @param callback - The functor to use for the callbacks.
   */
  void setAnimationEndedCallback(std::function<AnimationEndedCallback> callback) {
    anim_ended_callback_ = callback;
  }

  /**
   * @brief Adds an external animation from a file.
   *
   * You should give this animation a name, you will be able to
   * reference it with this name in the future. You can also set
   * the default animation modifier flags for this animation.
   * These flags will be used everytime you change to this animation
   * without explicitly specifying new flags.
   *
   * @param filename    The name of the file, from where to load the animation.
   * @param anim_name   The name with you wanna reference this animation.
   * @param flags       You can specify animation modifiers, like repeat the
   *                    animation after it ends, play it backwards, etc...
   * @param speed       Sets the default speed of the animation. If it's 1, it
   *                    will be played with the its default speed. If it's
   *                    negative, it will be played backwards.
   */
  void addAnimation(const std::string& filename,
                    const std::string& anim_name,
                    oglwrap::Bitfield<AnimFlag> flags = AnimFlag::None,
                    float speed = 1.0f);

  /**
   * @brief Sets the default animation, that will be played if you don't set to
   *        play another one.
   *
   * @param anim_name                 The user-defined name of the animation
   *                                  that should be set to be default.
   * @param default_transition_time   The fading time that should be used when
   *                                  changing to the default animation.
   */
  void setDefaultAnimation(const std::string& anim_name,
                           float default_transition_time = 0.0f);

private:
  /**
   * @brief Changes the current animation to a specified one.
   *
   * @param anim_idx          The index of the new animation.
   * @param current_time      The current time in seconds, optimally since the
   *                          start of the program.
   * @param transition_time   The fading time to be used for the transition.
   * @param flags             A bitfield containing the animation specifier flags.
   * @param speed             Sets the speed of the animation. If it's 0, will
   *                          play with the speed specified at the addAnim. If
   *                          it's negative, it will be played backwards.
   */
  void changeAnimation(size_t anim_idx,
                       float current_time,
                       float transition_time,
                       oglwrap::Bitfield<AnimFlag> flags,
                       float speed = 1.0f);

public:
  /**
   * @brief Tries to change the current animation to a specified one.
   *
   * Only changes it if the current animation is interruptable,
   * it's not currently in a transition, and new animation is
   * not the same as the one currently playing.
   *
   * @param new_anim       The parameters of the new animation.
   * @param current_time   The current time in seconds, optimally since the
   *                       start of the program.
   */
  void setCurrentAnimation(AnimParams new_anim,
                           float current_time);

  /**
   * @brief Forces the current animation to a specified one.
   *
   * Only changes it if the new animation is not the same as the one
   * currently playing.
   *
   * @param new_anim       The parameters of the new animation.
   * @param current_time   The current time in seconds, optimally since the
   *                       start of the program.
   */
  void forceCurrentAnimation(AnimParams new_anim,
                             float current_time);

  /// Returns the currently running animation's name.
  std::string getCurrentAnimation() const {
    return current_anim_name_;
  }

  /// Returns the currently running animation's state.
  AnimationState getCurrentAnimState() const {
    return current_anim_;
  }

  /// Returns the currently running animation's AnimFlags.
  oglwrap::Bitfield<AnimFlag> getCurrentAnimFlags() const {
    return current_anim_.flags;
  }

  /// Returns if the currently running animation is interruptable.
  bool isInterrupable() const {
    return current_anim_.flags.test(AnimFlag::Interruptable);
  }

  /**
   * Tries to change the current animation to the default one.
   *
   * Only changes it if the current animation is interruptable,
   * it's not currently in a transition, and new animation is
   * not the same as the one currently playing. Will use the default
   * anim modifier flags for the default anim.
   *
   * @param current_time  The current time in seconds, optimally since the
   *                      start of the program.
   */
  void setAnimToDefault(float current_time);

  /**
   * @brief Forces the current animation to the default one.
   *
   * Only changes it if the new animation is not the same as the one currently
   * playing. Will use the default anim modifier flags for the default anim.
   *
   * @param current_time    The current time in seconds, optimally since the
   *                        start of the program.
   */
  void forceAnimToDefault(float current_time);

  /// Returns the name of the default animation
  std::string getDefaultAnim() const {
    return anims_[anim_meta_info_.default_idx].name;
  }

  /**
   * @brief Returns the offset of the root bone, since it was last queried.
   *
   * It should be queried every frame (hence the name),
   * but it works even if you only query every 10th frame,
   * just the animation will "lag", and will look bad.
   */
  glm::vec2 offsetSinceLastFrame();

}; // AnimatedMeshRenderer

} // namespace engine

#include "animated_mesh_renderer_general-inl.hpp"
#include "animated_mesh_renderer_skinning-inl.hpp"
#include "animated_mesh_renderer_animation-inl.hpp"
#include "animated_mesh_renderer_animation-control-inl.hpp"

#endif // ENGINE_MESH_ANIMATED_MESH_RENDERER_HPP_
