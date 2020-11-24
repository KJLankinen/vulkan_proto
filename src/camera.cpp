#include "camera.h"
#include "renderer.h"

namespace vulkan_proto {
    Camera::Camera(Renderer &renderer) : m_renderer(renderer) {}
    Camera::~Camera() {}

    glm::mat4 Camera::getLookAt() {
        return glm::lookAt(m_position, m_position + m_direction, m_up);
    }

    void Camera::update() {
        // Movement per unit time
        m_position += m_velocity.y * m_direction * m_speed +
                      m_velocity.x * m_right * m_speed;

        // Direction
        m_yawPitch += m_dxdy * m_mouseSpeed;

        if (m_yawPitch.y < -M_PI / 2.0f) {
            m_yawPitch.y = -M_PI / 2.0f + 0.001f;
        } else if (m_yawPitch.y > M_PI / 2.0f) {
            m_yawPitch.y = M_PI / 2.0f - 0.001f;
        }

        m_direction = glm::vec3(cos(m_yawPitch.y) * sin(m_yawPitch.x),
                                cos(m_yawPitch.y) * cos(m_yawPitch.x),
                                sin(-m_yawPitch.y));
        glm::vec3 tempVec = m_direction;
        tempVec.z *= -0.5f;
        tempVec.z += 0.5f;
        const float directionMultiplier =
            tempVec.z < m_direction.z ? -1.0f : 1.0f;

        m_right = directionMultiplier *
                  glm::normalize(glm::cross(m_direction, tempVec));
        m_up = glm::cross(m_right, m_direction);

        m_dxdy = glm::vec2(0.0f);
    }

    Logger &Camera::getLogger() { return m_renderer.getLogger(); }
}
