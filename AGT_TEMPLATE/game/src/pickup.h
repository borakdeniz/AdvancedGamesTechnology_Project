#pragma once
#include <engine.h>


class pickup : public engine::game_object
{
public:
	pickup(const engine::game_object_properties props);

	~pickup();

	void init();

	void update(glm::vec3 c, float dt);

	bool active() { return m_is_active; }

	static engine::ref<pickup> create(const engine::game_object_properties& props);

private:

	bool m_is_active; // Indicates if pickup is active
};

