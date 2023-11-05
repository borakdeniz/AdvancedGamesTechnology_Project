#include "/Github/GameDevelopmentTechnologies_Project/AGT_TEMPLATE/game/src/pickup.h"

pickup::pickup(const engine::game_object_properties props) : engine::game_object(props)
{}

pickup::~pickup()
{}

void pickup::init()
{
	m_is_active = true;
}

void pickup::update(glm::vec3 c, float dt)
{
	set_rotation_amount(rotation_amount() + dt * 1.5f);

	glm::vec3 d = position() - c;
	if (glm::length(d) < 1.0f)
		m_is_active = false;
}

engine::ref<pickup> pickup::create(const engine::game_object_properties& props)
{
	return std::make_shared<pickup>(props);
}
