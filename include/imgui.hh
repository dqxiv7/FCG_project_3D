#ifndef IMGUI_WRAPPER_HH
#define IMGUI_WRAPPER_HH

class Gui
{
public:
    Gui (sf::Window& window, unsigned int width, unsigned int height)
    {
        if (!ImGui::SFML::Init (window, {(float) width, (float) height})) {
            std::cerr << "Failure: could not init ImGui::SFML." << std::endl;
            exit (1);
        }
        ImGui_ImplOpenGL3_Init ("#version 410 core");
    }

    ~Gui ()
    {
        ImGui_ImplOpenGL3_Shutdown ();
        ImGui::SFML::Shutdown ();
    }

    void process_event (sf::Window& window, const sf::Event& event)
    {
        ImGui::SFML::ProcessEvent (window, event);
    }

    void new_frame (sf::Window& window, sf::Time elapsed)
    {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition (window);
        sf::Vector2u size = window.getSize ();

        ImGui_ImplOpenGL3_NewFrame ();
        ImGui::SFML::Update (mouse_pos, {(float) size.x, (float) size.y}, elapsed);
    }

    void render ()
    {
        ImGui::Render ();
        ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    }
};

void draw_view_panel (Camera& camera)
{
    ImGui::SetNextWindowPos (ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin ("View");

    if (ImGui::Button ("Top"))   camera.view_top ();
    if (ImGui::Button ("Front")) camera.view_front ();
    if (ImGui::Button ("Right")) camera.view_right ();

    ImGui::Separator ();

    if (ImGui::Button ("Normal")) camera.view_normal ();
    if (ImGui::Button ("Tele"))   camera.view_tele ();
    if (ImGui::Button ("Wide"))   camera.view_wide ();

    ImGui::End ();
}

// Contenuto del popup di aggiunta (Shift+A o bottone "+"): scansiona data/
// alla ricerca di file .off, più una voce per aggiungere una luce.
void draw_add_menu (Scene& scene, Shaders& shaders)
{
    if (ImGui::BeginPopup ("AddMenu")) {
        bool can_add_light = scene.get_lights().size() < Scene::MAX_LIGHTS;
        if (ImGui::MenuItem ("Luce", nullptr, false, can_add_light))
            scene.add_light (shaders);

        ImGui::Separator ();

        for (const auto& entry : std::filesystem::directory_iterator ("data")) {
            if (entry.path().extension() != ".off")
                continue;
            std::string label = entry.path().stem().string();
            if (ImGui::MenuItem (label.c_str()))
                scene.add_obj (entry.path().string(), shaders);
        }

        ImGui::EndPopup ();
    }
}

void draw_outliner (Scene& scene, Shaders& shaders, float window_width, bool& open_add_menu, bool& open_delete_confirm)
{
    const float panel_width = 210.0f;
    const float margin = 10.0f;

    ImGui::SetNextWindowPos (ImVec2(window_width - panel_width - margin, margin), ImGuiCond_Always);
    ImGui::SetNextWindowSize (ImVec2(panel_width, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin ("Outliner");

    // Apriamo i popup qui, nello stesso contesto (stessa finestra ImGui
    // "corrente") in cui li controlliamo con BeginPopup/BeginPopupModal —
    // chiamare OpenPopup da un altro contesto (es. l'handler della
    // tastiera) calcolerebbe un ID diverso e il popup non si vedrebbe.
    if (ImGui::Button ("+") || open_add_menu) {
        ImGui::OpenPopup ("AddMenu");
        open_add_menu = false;
    }

    ImGui::SameLine ();

    if (ImGui::Button ("-") || open_delete_confirm) {
        if (scene.active)
            ImGui::OpenPopup ("ConfirmDelete");
        open_delete_confirm = false;
    }

    draw_add_menu (scene, shaders);

    if (ImGui::BeginPopupModal ("ConfirmDelete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text ("Eliminare l'oggetto selezionato?");
        if (ImGui::Button ("Elimina")) {
            scene.remove_active (shaders);
            ImGui::CloseCurrentPopup ();
        }
        ImGui::SameLine ();
        if (ImGui::Button ("Annulla"))
            ImGui::CloseCurrentPopup ();
        ImGui::EndPopup ();
    }

    ImGui::Separator ();

    for (const auto& obj : scene.get_objects()) {
        bool selected = (scene.active == obj.get());
        if (ImGui::Selectable (obj->name.c_str(), selected))
            scene.set_active (obj.get());
    }

    ImGui::End ();
}

void draw_properties_panel (Scene& scene, Camera& camera, float window_width)
{
    const float panel_width = 210.0f;
    const float margin = 10.0f;
    const float outliner_height = 200.0f;
    const float gap = 10.0f;

    ImGui::SetNextWindowPos (ImVec2(window_width - panel_width - margin, margin + outliner_height + gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize (ImVec2(panel_width, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin ("Properties");

    if (ImGui::ColorEdit3 ("Ambient (scena)", &camera.ambient_light[0]))
        camera.update_ambient ();

    ImGui::Separator ();

    // Light deriva da Mesh: va controllata PRIMA, altrimenti il cast a
    // Mesh* la intercetterebbe sempre e mostreremmo i controlli sbagliati.
    if (Light* light = dynamic_cast<Light*> (scene.active)) {
        ImGui::Text ("Luce: %s", light->name.c_str());

        bool changed = false;
        changed |= ImGui::ColorEdit3 ("Colore", &light->light_color[0]);
        changed |= ImGui::SliderFloat ("Intensità", &light->light_intensity, 0.0f, 5.0f);
        if (changed)
            light->update_color ();
    }
    else if (Mesh* mesh = dynamic_cast<Mesh*> (scene.active)) {
        ImGui::Text ("Mesh: %s", mesh->name.c_str());

        // i valori sono pubblici e usati direttamente da Mesh::draw_obj()
        // ad ogni frame: non serve richiamare nulla dopo averli modificati.
        ImGui::ColorEdit3 ("Diffuse", &mesh->material_diffuse[0]);
        ImGui::ColorEdit3 ("Ambient", &mesh->material_ambient[0]);
        ImGui::ColorEdit3 ("Specular", &mesh->material_specular[0]);
        ImGui::SliderFloat ("Shininess", &mesh->material_shininess, 1.0f, 256.0f);
    }
    else {
        ImGui::Text ("Nessun oggetto selezionato");
    }

    ImGui::End ();
}

#endif