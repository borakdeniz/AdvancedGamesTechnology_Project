#include "example_layer.h"
#include "platform/opengl/gl_shader.h"

#include "pickup.h"
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp>
#include "engine/events/key_event.h"
#include "engine/utils/track.h"


template<typename T, typename O>
void render_object(glm::vec3& object_location, O& m_object, const T& mesh_shader)
{
	glm::mat4 object_transform(1.0f);
	object_transform = glm::translate(object_transform, object_location);
	object_transform = glm::rotate(object_transform, m_object->rotation_amount(), m_object->rotation_axis());
	object_transform = glm::scale(object_transform, m_object->scale());
	engine::renderer::submit(mesh_shader, object_transform, m_object);
}

example_layer::example_layer() 
    :m_2d_camera(-1.6f, 1.6f, -0.9f, 0.9f), 
    m_3d_camera((float)engine::application::window().width(), (float)engine::application::window().height())


{
    // Hide the mouse and lock it inside the window
    //engine::input::anchor_mouse(true);
    engine::application::window().hide_mouse_cursor();

	// Initialise audio and play background music
	m_audio_manager = engine::audio_manager::instance();
	m_audio_manager->init();
	m_audio_manager->load_sound("assets/audio/bounce.wav", engine::sound_type::spatialised, "bounce"); // Royalty free sound from freesound.org
	m_audio_manager->load_sound("assets/audio/DST-impuretechnology.mp3", engine::sound_type::track, "music");  // Royalty free music from http://www.nosoapradio.us/
	m_audio_manager->play("music");
	m_audio_manager->pause("music");


	// Initialise the shaders, materials and lights
	auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
	auto text_shader = engine::renderer::shaders_library()->get("text_2D");

	m_directionalLight.Color = glm::vec3(1.0f, 1.0f, 1.0f);
	m_directionalLight.AmbientIntensity = 0.25f;
	m_directionalLight.DiffuseIntensity = 0.6f;
	m_directionalLight.Direction = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));

	// set color texture unit
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->bind();
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gColorMap", 0);
	m_directionalLight.submit(mesh_shader);
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gMatSpecularIntensity", 1.f);
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gSpecularPower", 10.f);
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("transparency", 1.0f);

	std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->bind();
	std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("projection",
		glm::ortho(0.f, (float)engine::application::window().width(), 0.f,
		(float)engine::application::window().height()));


	m_material = engine::material::create(1.0f, glm::vec3(1.0f, 0.1f, 0.07f),
		glm::vec3(1.0f, 0.1f, 0.07f), glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);

	m_mannequin_material = engine::material::create(1.0f, glm::vec3(0.5f, 0.5f, 0.5f),
		glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);

	// For materials, the parameters correspond to shininess, ambient colour, diffuse colour, specular colour and transparency. In that order.
	m_tetrahedron_material = engine::material::create(32.0f,
		glm::vec3(1.0f, 0.5f, 0.0f),
		glm::vec3(1.0f, 0.5f, 0.0f),
		glm::vec3(0.5f, 0.5f, 0.5f),
		0.3f);

	// Skybox texture from http://www.vwall.it/wp-content/plugins/canvasio3dpro/inc/resource/cubeMaps/
	m_skybox = engine::skybox::create(50.f,
		{ engine::texture_2d::create("assets/textures/skybox/SkyboxFront.bmp", true),
		  engine::texture_2d::create("assets/textures/skybox/SkyboxRight.bmp", true),
		  engine::texture_2d::create("assets/textures/skybox/SkyboxBack.bmp", true),
		  engine::texture_2d::create("assets/textures/skybox/SkyboxLeft.bmp", true),
		  engine::texture_2d::create("assets/textures/skybox/SkyboxTop.bmp", true),
		  engine::texture_2d::create("assets/textures/skybox/SkyboxBottom.bmp", true)
		});

	engine::ref<engine::skinned_mesh> m_skinned_mesh = engine::skinned_mesh::create("assets/models/animated/mannequin/free3Dmodel.dae");
	m_skinned_mesh->LoadAnimationFile("assets/models/animated/mannequin/walking.dae");
	m_skinned_mesh->LoadAnimationFile("assets/models/animated/mannequin/idle.dae");
	m_skinned_mesh->LoadAnimationFile("assets/models/animated/mannequin/jump.dae");
	m_skinned_mesh->LoadAnimationFile("assets/models/animated/mannequin/standard_run.dae");
	m_skinned_mesh->switch_root_movement(false);

	engine::game_object_properties mannequin_props;
	mannequin_props.animated_mesh = m_skinned_mesh;
	mannequin_props.scale = glm::vec3(1.f/ glm::max(m_skinned_mesh->size().x, glm::max(m_skinned_mesh->size().y, m_skinned_mesh->size().z)));
	mannequin_props.position = glm::vec3(3.0f, 0.5f, -5.0f);
	mannequin_props.type = 0;
	mannequin_props.bounding_shape = m_skinned_mesh->size() / 2.f * mannequin_props.scale.x;
	m_mannequin = engine::game_object::create(mannequin_props);
	m_player.initialise(m_mannequin);


	// Load the terrain texture and create a terrain mesh. Create a terrain object. Set its properties
	std::vector<engine::ref<engine::texture_2d>> terrain_textures = { engine::texture_2d::create("assets/textures/terrain.bmp", false) };
	engine::ref<engine::terrain> terrain_shape = engine::terrain::create(100.f, 0.5f, 100.f);
	engine::game_object_properties terrain_props;
	terrain_props.meshes = { terrain_shape->mesh() };
	terrain_props.textures = terrain_textures;
	terrain_props.is_static = true;
	terrain_props.type = 0;
	terrain_props.bounding_shape = glm::vec3(100.f, 0.5f, 100.f);
	terrain_props.restitution = 0.92f;
	m_terrain = engine::game_object::create(terrain_props);

	// Load the cow model. Create a cow object. Set its properties
	engine::ref <engine::model> cow_model = engine::model::create("assets/models/static/cow4.3ds");
	engine::game_object_properties cow_props;
	cow_props.meshes = cow_model->meshes();
	cow_props.textures = cow_model->textures();
	float cow_scale = 1.f / glm::max(cow_model->size().x, glm::max(cow_model->size().y, cow_model->size().z));
	cow_props.position = { -4.f,0.5f, -5.f };
	cow_props.scale = glm::vec3(cow_scale);
	cow_props.bounding_shape = cow_model->size() / 2.f * cow_scale;
	m_cow = engine::game_object::create(cow_props);

	// Load the tree model. Create a tree object. Set its properties
	engine::ref <engine::model> tree_model = engine::model::create("assets/models/static/elm.3ds");
	engine::game_object_properties tree_props;
	tree_props.meshes = tree_model->meshes();
	tree_props.textures = tree_model->textures();
	float tree_scale = 3.f / glm::max(tree_model->size().x, glm::max(tree_model->size().y, tree_model->size().z));
	tree_props.position = { 4.f, 0.5f, -5.f };
	tree_props.bounding_shape = tree_model->size() / 2.f * tree_scale;
	tree_props.scale = glm::vec3(tree_scale);
	m_tree = engine::game_object::create(tree_props);

	// Load the Jeep model. Create a jeep object. Set its properties
	engine::ref <engine::model> jeep_model = engine::model::create("assets/models/static/jeep1.3ds");
	engine::game_object_properties jeep_props;
	jeep_props.meshes = jeep_model->meshes();
	jeep_props.textures = jeep_model->textures();
	float jeep_scale = 0.125;
	jeep_props.position = { -5.f,0.5f, 0.f };
	jeep_props.scale = glm::vec3(jeep_scale);
	jeep_props.bounding_shape = jeep_model->size() / 2.f * jeep_scale;
	m_jeep = engine::game_object::create(jeep_props);

	engine::ref<engine::sphere> sphere_shape = engine::sphere::create(10, 20, 0.5f);
	engine::game_object_properties sphere_props;
	sphere_props.position = { 0.f, 5.f, -5.f };
	sphere_props.meshes = { sphere_shape->mesh() };
	sphere_props.type = 1;
	sphere_props.bounding_shape = glm::vec3(0.5f);
	sphere_props.restitution = 0.92f;
	sphere_props.mass = 0.000001f;
	m_ball = engine::game_object::create(sphere_props);

	std::vector<glm::vec3> tetrahedron_vertices;
	tetrahedron_vertices.push_back(glm::vec3(0.f, 10.f, 0.f));		//0
	tetrahedron_vertices.push_back(glm::vec3(0.f, 0.f, 10.f));		//1
	tetrahedron_vertices.push_back(glm::vec3(-10.f, 0.f, -10.f));	//2
	tetrahedron_vertices.push_back(glm::vec3(10.f, 0.f, -10.f));	//3

	engine::ref<engine::tetrahedron> tetrahedron_shape = engine::tetrahedron::create(tetrahedron_vertices);
	engine::game_object_properties tetrahedron_props;
	tetrahedron_props.position = { 0.f, 0.5f, -20.f };
	tetrahedron_props.meshes = { tetrahedron_shape->mesh() };
	std::vector<engine::ref<engine::texture_2d>> tetrahedron_textures = { engine::texture_2d::create("assets/textures/stone.bmp", false) };
	tetrahedron_props.textures = tetrahedron_textures;
	m_tetrahedron = engine::game_object::create(tetrahedron_props);


	m_game_objects.push_back(m_terrain);
	m_game_objects.push_back(m_ball);
	//m_game_objects.push_back(m_cow);
	//m_game_objects.push_back(m_tree);
	//m_game_objects.push_back(m_pickup);
	m_physics_manager = engine::bullet_manager::create(m_game_objects);

	m_text_manager = engine::text_manager::create();

	m_skinned_mesh->switch_animation(1);

	// Adding a pick up
	// Medkit texture from https://www.textures.com/download/manmadeboxes0007/105116
	engine::ref<engine::cuboid> pickup_shape = engine::cuboid::create(glm::vec3(0.5f), false);
	engine::ref<engine::texture_2d> pickup_texture = engine::texture_2d::create("assets/textures/medkit.jpg", true);
	engine::game_object_properties pickup_props;
	pickup_props.position = { 5.f, 1.f, 5.f };
	pickup_props.meshes = { pickup_shape->mesh() };
	pickup_props.textures = { pickup_texture };
	m_pickup = pickup::create(pickup_props);
	m_pickup->init();
}

example_layer::~example_layer() {}

void example_layer::on_update(const engine::timestep& time_step) 
{
	m_3d_camera.on_update(time_step);

	m_physics_manager->dynamics_world_update(m_game_objects, double(time_step));

	m_audio_manager->update_with_camera(m_3d_camera);

	m_pickup->update(m_3d_camera.position(), time_step);

	m_player.on_update(time_step);
	m_player.update_camera(m_3d_camera);

	check_bounce();
} 

void example_layer::on_render() 
{
    engine::render_command::clear_color({0.2f, 0.3f, 0.3f, 1.0f}); 
    engine::render_command::clear();

	// Set up  shader. (renders textures and materials)
	const auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
	engine::renderer::begin_scene(m_3d_camera, mesh_shader);

	// Set up some of the scene's parameters in the shader
	std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gEyeWorldPos", m_3d_camera.position());

	// Position the skybox centred on the player and render it
	glm::mat4 skybox_tranform(1.0f);
	skybox_tranform = glm::translate(skybox_tranform, m_3d_camera.position());
	for (const auto& texture : m_skybox->textures())
	{
		texture->bind();
	}
	engine::renderer::submit(mesh_shader, m_skybox, skybox_tranform);

	engine::renderer::submit(mesh_shader, m_terrain);

	// Rendering a forest
	float x = 0.f, y = 0.5f, z = 2.f; // Tree location parameters

	glm::vec3 tree_location = glm::vec3(x, y, z);
	render_object(tree_location, m_tree, mesh_shader);



	glm::vec3 cow_location = glm::vec3(0.f, 0.5f, 0.f);
	glm::vec3 c = m_3d_camera.position();
	glm::vec3 v = c - cow_location;

	float theta = atan2(v.x, v.z);

	// Calculate the rotation axis
	glm::vec3 target(0.0f, 0.0f, 1.0f);
	glm::vec3 rotation_axis = glm::cross(target, v);

	// Calculate the rotation angle in degrees
	float dot_product = glm::dot(target, v);
	float magnitudes = glm::length(target) * glm::length(v);
	float rotation_angle = glm::degrees(acos(dot_product / magnitudes));

	// Reset the cow's transform matrix to identity to avoid cumulative rotations
	glm::mat4 cow_transform(1.0f);

	// Apply the rotation to orient the cow towards the camera
	cow_transform = glm::rotate(cow_transform, rotation_angle, rotation_axis);

	// Apply translation and scaling as needed
	cow_transform = glm::translate(cow_transform, cow_location);
	cow_transform = glm::scale(cow_transform, m_cow->scale());

	// Render the cow with the updated transform
	engine::renderer::submit(mesh_shader, cow_transform, m_cow);

	// Cow gone rogue, can't fix it

	glm::mat4 jeep_transform(1.0f);
	jeep_transform = glm::translate(jeep_transform, m_jeep->position());
	jeep_transform = glm::rotate(jeep_transform, -glm::pi<float>()/2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	jeep_transform = glm::scale(jeep_transform, m_jeep->scale());
	engine::renderer::submit(mesh_shader, jeep_transform, m_jeep);



	if (m_pickup->active()) {
		std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);
		m_pickup->textures().at(0)->bind();
		m_pickup->set_position(glm::vec3(-2.f, 1.5f, -2.f));
		glm::mat4 pickup_transform(1.0f);
		pickup_transform = glm::translate(pickup_transform, m_pickup->position());
		pickup_transform = glm::rotate(pickup_transform, m_pickup->rotation_amount(), m_pickup->rotation_axis());
		engine::renderer::submit(mesh_shader, m_pickup->meshes().at(0), pickup_transform);
		std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
	}


	
	m_material->submit(mesh_shader);
	engine::renderer::submit(mesh_shader, m_ball);

	m_mannequin_material->submit(mesh_shader);
	engine::renderer::submit(mesh_shader, m_player.object());

	m_tetrahedron_material->submit(mesh_shader);
	engine::renderer::submit(mesh_shader, m_tetrahedron);

    engine::renderer::end_scene();

	// Render text
	const auto text_shader = engine::renderer::shaders_library()->get("text_2D");
	m_text_manager->render_text(text_shader, "Orange Text", 10.f, (float)engine::application::window().height()-25.f, 0.5f, glm::vec4(1.f, 0.5f, 0.f, 1.f));
} 

void example_layer::on_event(engine::event& event) 
{ 
    if(event.event_type() == engine::event_type_e::key_pressed) 
    { 
        auto& e = dynamic_cast<engine::key_pressed_event&>(event); 
        if(e.key_code() == engine::key_codes::KEY_TAB) 
        { 
            engine::render_command::toggle_wireframe();
        }
    } 
}

void example_layer::check_bounce()
{
	if (m_prev_sphere_y_vel < 0.1f && m_ball->velocity().y > 0.1f)
		//m_audio_manager->play("bounce");
		m_audio_manager->play_spatialised_sound("bounce", m_3d_camera.position(), glm::vec3(m_ball->position().x, 0.f, m_ball->position().z));
	m_prev_sphere_y_vel = m_game_objects.at(1)->velocity().y;
}
