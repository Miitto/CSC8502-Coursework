#pragma once

#include <engine/camera.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <vector>

class CameraTrack {
public:
  struct Keyframe {
    float time;
    glm::vec3 position;
    glm::quat orientation;
  };

  struct Effect {
    float startTime;
    float endTime = 0.0f;
    std::function<void(float time)> action;
    bool triggered = false;
  };

  CameraTrack() = default;
  CameraTrack(std::vector<Keyframe>&& kfs) : keyframes(std::move(kfs)) {}

  void addKeyframe(float time, const glm::vec3& position,
                   const engine::Camera::Rotation& rotation) {
    float pitch = rotation.x;
    if (pitch < -89.9f) {
      pitch = -89.9f;
    } else if (pitch > 89.9f) {
      pitch = 89.9f;
    }
    float yaw = rotation.y;
    if (rotation.y < 0.0f) {
      yaw += 360.0f;
    } else if (rotation.y >= 360.0f) {
      yaw -= 360.0f;
    }

    glm::vec3 eulerRadians = glm::radians(glm::vec3(pitch, yaw, 0.0f));
    glm::quat orientation = glm::quat(eulerRadians);
    Keyframe kf{time, position, orientation};
    addKeyframe(kf);
  }

  void addKeyframe(Keyframe kf) {
    size_t index = 0;
    for (; index < keyframes.size(); ++index) {
      if (keyframes[index].time > kf.time) {
        break;
      }
    }

    keyframes.insert(keyframes.begin() + index, kf);
  }

  void addEffect(float startTime, std::function<void(float)> action) {
    effects.push_back(Effect{startTime, startTime, action});
  }

  void addEffect(float startTime, float endTime,
                 std::function<void(float)> action) {
    effects.push_back(Effect{startTime, endTime, action});
  }

  void update(float deltaTime) {
    _time += deltaTime;

    if (_time > keyframes.back().time) {
      _time = 0.0f;
    }

    for (Effect& effect : effects) {
      if (effect.startTime <= _time && !effect.triggered) {
        effect.action(_time - effect.startTime);
        if (effect.endTime <= _time)
          effect.triggered = true;
      }
    }
  }

  glm::vec3 position() const {
    if (keyframes.empty()) {
      return glm::vec3(0.0f);
    }

    if (_time <= keyframes.front().time) {
      return keyframes.front().position;
    }

    if (_time >= keyframes.back().time) {
      return keyframes.back().position;
    }

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
      const Keyframe& kf1 = keyframes[i];
      const Keyframe& kf2 = keyframes[i + 1];
      if (_time >= kf1.time && _time <= kf2.time) {
        float t = (_time - kf1.time) / (kf2.time - kf1.time);
        float cubicT = t < 0.5f ? 4.0f * t * t * t
                                : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;

        return glm::mix(kf1.position, kf2.position, cubicT);
      }
    }

    return keyframes.back().position;
  }

  glm::quat rotation() const {
    if (keyframes.empty()) {
      return glm::quat(glm::vec3(0.0));
    }

    if (_time <= keyframes.front().time) {
      return keyframes.front().orientation;
    }
    if (_time >= keyframes.back().time) {
      return keyframes.back().orientation;
    }

    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
      const Keyframe& kf1 = keyframes[i];
      const Keyframe& kf2 = keyframes[i + 1];
      if (_time >= kf1.time && _time <= kf2.time) {
        float t = (_time - kf1.time) / (kf2.time - kf1.time);

        float cubicT = t < 0.5f ? 4.0f * t * t * t
                                : 1.0f - powf(-2.0f * t + 2.0f, 3) / 2.0f;

        glm::quat interpOrientation =
            glm::slerp(kf1.orientation, kf2.orientation, cubicT);
        return interpOrientation;
      }
    }

    return keyframes.back().orientation;
  }

protected:
  std::vector<Keyframe> keyframes;
  std::vector<Effect> effects;
  float _time = 0.0f;
};