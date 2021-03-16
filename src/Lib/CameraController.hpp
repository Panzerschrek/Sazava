#pragma once
#include "Mat.hpp"
#include "SystemEvent.hpp"


namespace SZV
{

class CameraController final
{
public:
	CameraController(float aspect);

	void Update(float time_delta_s, const InputState& input_state);

	// Returns rotation + aspect
	m_Mat4 CalculateViewMatrix() const;
	m_Mat4 CalculateFullViewMatrix() const;
	m_Vec3 GetCameraPosition() const;

private:
	const float aspect_;

	m_Vec3 pos_= m_Vec3(0.0f, 0.0f, 0.0f);
	float azimuth_= 0.0f;
	float elevation_= 0.0f;
};

} // namespace SZV
