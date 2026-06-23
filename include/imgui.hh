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

void draw_outliner (Scene& scene, float window_width)
{
    const float panel_width = 210.0f;
    const float margin = 10.0f;

    ImGui::SetNextWindowPos (ImVec2(window_width - panel_width - margin, margin), ImGuiCond_Always);
    ImGui::SetNextWindowSize (ImVec2(panel_width, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin ("Outliner");

    for (const auto& obj : scene.get_objects()) {
        bool selected = (scene.active == obj.get());
        if (ImGui::Selectable (obj->name.c_str(), selected))
            scene.active = obj.get();
    }

    ImGui::End ();
}

void draw_light_panel (Scene& scene, float window_width)
{
    const float panel_width = 210.0f;
    const float margin = 10.0f;
    const float outliner_height = 200.0f;
    const float gap = 10.0f;

    ImGui::SetNextWindowPos (ImVec2(window_width - panel_width - margin, margin + outliner_height + gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize (ImVec2(panel_width, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin ("Luci");

    for (Light* light : scene.get_lights()) {
        ImGui::PushID (light);
        ImGui::Text ("%s", light->name.c_str());

        bool changed = false;
        changed |= ImGui::ColorEdit3 ("Colore", &light->light_color[0]);
        changed |= ImGui::SliderFloat ("Intensità", &light->light_intensity, 0.0f, 5.0f);
        changed |= ImGui::ColorEdit3 ("Ambient", &light->light_ambient_val[0]);
        if (changed)
            light->update_color ();

        ImGui::Separator ();
        ImGui::PopID ();
    }

    ImGui::End ();
}

#endif