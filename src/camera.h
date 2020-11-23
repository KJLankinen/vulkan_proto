#pragma once

#include "headers.h"

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct Camera {
    const Renderer &m_renderer;

    const float m_defaultSpeed = 0.05f;
    const float m_highSpeed = 0.01f;
    glm::vec2 m_velocity = glm::vec2(0.0f);
    float m_speed = m_defaultSpeed;
    float m_mouseSpeed = 0.005f;

    float m_near = 0.1f;
    float m_far = 100.0f;
    float m_fov = 70.0f;

    glm::vec2 m_yawPitch = glm::vec2(0.0f);
    glm::vec2 m_dxdy = glm::vec2(0.0f);

    glm::vec3 m_position = glm::vec3(0.0f, -5.0f, 0.0f);

    glm::vec3 m_up = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_direction = glm::vec3(0.0f, 1.0f, 0.0f);

    Camera(Renderer &renderer);
    ~Camera();
    glm::mat4 getLookAt();
    void update();
    Logger &getLogger();
};
} // namespace vulkan_proto
