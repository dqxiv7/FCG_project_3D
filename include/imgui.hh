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

#endif