#pragma once
#include <engine.h>
#include "glm/gtx/rotate_vector.hpp"


class player
{
public:
	player();
	~player();

	void initialise(engine::ref<engine::game_object> object);
	void on_update(const engine::timestep& time_step);

	engine::ref<engine::game_object> object() const { return m_object; }

	void turn(float angle);
	void update_camera(engine::perspective_camera& camera);

	void jump();

private:
	float m_speed{ 0.f };
	float m_timer;

	engine::ref< engine::game_object> m_object;
};
