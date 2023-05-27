#include "EntityElement.h"

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"
/////////////////////////////////////////////// TASK I /////////////////////////////////////////////////////////////////
#include <tinyfiledialogs/tinyfiledialogs.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<EditorScene::EntityElement> EditorScene::EntityElement::new_default(const SceneContext& scene_context, ElementRef parent) {
    auto rendered_entity = EntityRenderer::Entity::create(
        scene_context.model_loader.load_from_file<EntityRenderer::VertexData>("cube.obj"),
        EntityRenderer::InstanceData{
            glm::mat4{}, // Set via update_instance_data()
            EntityRenderer::EntityMaterial{
                {1.0f, 1.0f, 1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f, 1.0f},
                512.0f,
            }
        },
        EntityRenderer::RenderData{
            scene_context.texture_loader.default_white_texture(),
            scene_context.texture_loader.default_white_texture()
        }
    );

    auto new_entity = std::make_unique<EntityElement>(
        parent,
        "New Entity",
        glm::vec3{0.0f},
        glm::vec3{0.0f},
        glm::vec3{1.0f},
        rendered_entity
    );

    new_entity->update_instance_data();
    return new_entity;
}

std::unique_ptr<EditorScene::EntityElement> EditorScene::EntityElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto new_entity = new_default(scene_context, parent);

    new_entity->update_local_transform_from_json(j);
    new_entity->update_material_from_json(j);

    new_entity->rendered_entity->model = scene_context.model_loader.load_from_file<EntityRenderer::VertexData>(j["model"]);
    new_entity->rendered_entity->render_data.diffuse_texture = texture_from_json(scene_context, j["diffuse_texture"]);
    new_entity->rendered_entity->render_data.specular_map_texture = texture_from_json(scene_context, j["specular_map_texture"]);

    new_entity->update_instance_data();
    return new_entity;
}

json EditorScene::EntityElement::into_json() const {
    if (!rendered_entity->model->get_filename().has_value()) {
        return {
            {"error", Formatter() << "Entity [" << name << "]'s model does not have a filename so can not be exported, and has been skipped."}
        };
    }

    return {
        local_transform_into_json(),
        material_into_json(),
        {"model", rendered_entity->model->get_filename().value()},
        {"diffuse_texture", texture_to_json(rendered_entity->render_data.diffuse_texture)},
        {"specular_map_texture", texture_to_json(rendered_entity->render_data.specular_map_texture)},
    };
}

void EditorScene::EntityElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Entity");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    add_local_transform_imgui_edit_section(render_scene, scene_context);
    add_material_imgui_edit_section(render_scene, scene_context);

    ImGui::Text("Model & Textures");
    scene_context.model_loader.add_imgui_model_selector("Model Selection", rendered_entity->model);
    scene_context.texture_loader.add_imgui_texture_selector("Diffuse Texture", rendered_entity->render_data.diffuse_texture);
    scene_context.texture_loader.add_imgui_texture_selector("Specular Map", rendered_entity->render_data.specular_map_texture, false);
    ImGui::Spacing();

    /////////////////////////////////////////////// TASK I /////////////////////////////////////////////////////////////
    // Add a button for model upload
    if (ImGui::Button("Upload Model")) {
        const char* filter = "*.model"; // Replace with your model file extension
        const auto init_path = (std::filesystem::current_path() / "models").string(); // Default model directory

#ifdef __APPLE__
        // Apparently the file filter doesn't work properly on Mac?
        // Feel free to re-enable if you want to try it, but I have disabled it on Mac for now.

        const char* path = tinyfd_openFileDialog("Open Model", init_path.c_str(), 0, nullptr, nullptr, false);
#else
        const char* path = tinyfd_openFileDialog("Open Model", init_path.c_str(), 1, &filter, "Model Files", false);
#endif

        if (path != nullptr) {
            try {
                // Add to res/models directory
                std::filesystem::path source_path(path);
                std::filesystem::path target_path = std::filesystem::current_path() / "res/models" / source_path.filename();

                // Copy the file
                std::filesystem::copy(source_path, target_path, std::filesystem::copy_options::overwrite_existing);

                // Extract the filename of the model
                std::string model_file_name = source_path.filename().string();

                // Load the model directly after upload
                auto model = scene_context.model_loader.load_from_file<EntityRenderer::VertexData>(model_file_name);
                // Assign the model to the rendered_entity
                rendered_entity->model = model;
                std::cout << "Model loaded successfully." << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Error while trying to add model file:" << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }
    }

    // Add a button for texture map upload
    if (ImGui::Button("Upload Texture Map")) {
        const char* filter = "*.png";
        const auto init_path = (std::filesystem::current_path() / "textures").string(); // Default model directory

#ifdef __APPLE__
        // Apparently the file filter doesn't work properly on Mac?
        // Feel free to re-enable if you want to try it, but I have disabled it on Mac for now.

        const char* path = tinyfd_openFileDialog("Open Texture Map", init_path.c_str(), 0, nullptr, nullptr, false);
#else
        const char* path = tinyfd_openFileDialog("Open Texture Map", init_path.c_str(), 1, &filter, "Texture Files", false);
#endif

        if (path != nullptr) {
            try {
                // Add to res/textures directory
                std::filesystem::path source_path(path);
                std::filesystem::path target_path = std::filesystem::current_path() / "res/textures" / source_path.filename();

                // Copy the file
                std::filesystem::copy(source_path, target_path, std::filesystem::copy_options::overwrite_existing);

                // Extract the filename of the texture
                std::string texture_file_name = source_path.filename().string();

                // Load the model directly after upload
                auto texture = scene_context.texture_loader.load_from_file(texture_file_name, false, false);

                // Assign the model to the rendered_entity
                rendered_entity->render_data.diffuse_texture = texture;
                std::cout << "Texture loaded successfully." << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Error while trying to add texture map:" << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void EditorScene::EntityElement::update_instance_data() {
    transform = calc_model_matrix();

    if (!EditorScene::is_null(parent)) {
        // Post multiply by transform so that local transformations are applied first
        transform = (*parent)->transform * transform;
    }

    rendered_entity->instance_data.model_matrix = transform;
    rendered_entity->instance_data.material = material;
}

const char* EditorScene::EntityElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
