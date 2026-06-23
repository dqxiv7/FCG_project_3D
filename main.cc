#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"
#include <SFML/Window.hpp>

#include "include.hh"


/////////////////////////////
// Window and OpenGL setup //
/////////////////////////////

class Setup
{
public:
    sf::Window* window;

    Setup ()
    {
        sf::ContextSettings settings;
        settings.depthBits = 32;
        settings.stencilBits = 8;
        settings.antiAliasingLevel = 0;
        settings.attributeFlags = sf::ContextSettings::Attribute::Core;
        settings.majorVersion = 4;
        settings.minorVersion = 1;

        window = new sf::Window (
                                 sf::VideoMode({800, 800}),
                                 "SFML + OpenGL",
                                 sf::Style::Default,
                                 sf::State::Windowed,
                                 settings
                                 );
        window->setVerticalSyncEnabled (true);

        if (!window->setActive (true)) {
            std::cerr << "Failure: error during SFML OpenGL Activation." << std::endl;
            exit (1);
        }
        sf::ContextSettings gotten = window->getSettings ();

        std::cout << "depth bits: " << gotten.depthBits << std::endl;
        std::cout << "stencil bits: " << gotten.stencilBits << std::endl;
        std::cout << "antialiasing level: " << gotten.antiAliasingLevel << std::endl;
        std::cout << "SFML GL version: " << gotten.majorVersion << "." << gotten.minorVersion << std::endl;

        int version = gladLoadGL (sf::Context::getFunction);
        if (!version) {
            std::cerr << "Failure: error during glad loading." << std::endl;
            exit (1);
        }
        std::cout << "GLAD GL version: " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;
    }

    ~Setup ()
    {
        delete window;
    }
};

////////////////////
// SFML Callbacks //
////////////////////
void handle (const sf::Event::KeyPressed& key, Shaders& shaders, Camera& camera)
{
    switch (key.scancode) {
     case sf::Keyboard::Scancode::N:
        camera.view_normal ();
        return;
    case sf::Keyboard::Scancode::T:
        camera.view_tele ();
        return;
    case sf::Keyboard::Scancode::W:
        camera.view_wide ();
        return;
    default:
        return;
    }
}


void handle (const sf::Event::KeyPressed& key, Object& object, Scene& scene)
{

    switch (key.scancode) {
    case sf::Keyboard::Scancode::G:
        if (!object.moving){
            object.moving = true;
            object.mode = Object::TransformMode::Translate;
        }
        else{ 
            object.moving = false;
            object.mode = Object::TransformMode::None;            
        }
        return;
    case sf::Keyboard::Scancode::R:
        if (!object.moving){
            object.moving = true;
            object.mode = Object::TransformMode::Rotate;
        }
        else{
            object.moving = false;
            object.mode = Object::TransformMode::None;
            object.trackball_active = false;
        }
        return;
    case sf::Keyboard::Scancode::S:
        if (!object.moving){
            object.moving = true;
            object.mode = Object::TransformMode::Scale;
        }
        else{ 
            object.moving = false;
            object.mode = Object::TransformMode::None;            
        }
    default:
        return;
    }
}

void handle (const sf::Event::MouseMoved* mouse, Camera& camera, Object* object)
{
    float x = mouse->position.x;
    float y = mouse->position.y;
    static float prev_x = 0;
    static float prev_y = 0;

    float dx = x - prev_x;
    float dy = y - prev_y;
    // std::cout <<"dx: "<<dx<<" | dy: "<<dy<<"\n";
    prev_x = x;
    prev_y = y;

    if (object && object->moving){
        if (object->mode == Object::TransformMode::Translate)
            object->translation(dx,-dy,camera.inv_v, camera.od, camera.fd);
        else if (object->mode == Object::TransformMode::Rotate)
            object->rotation(x, y, camera.inv_v, camera.aspect.width, camera.aspect.height);
        else if (object->mode == Object::TransformMode::Scale)
            object->scale(dx,dy, camera.od, camera.fd);
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)){
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
            camera.pan(dx,dy);
    }
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        camera.drag (x, y);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
        camera.zoom(dy);
}

void handle (const sf::Event::MouseWheelScrolled* wheel, Camera& camera)
{
    camera.dolly(wheel->delta);
}

void handle (const sf::Event::MouseButtonPressed* mouse, Scene& scene, Shaders& picking, Shaders& shaders, sf::Window& window, Camera& camera)
{
    if (mouse->button == sf::Mouse::Button::Left){
        sf::Vector2i pixel_tmp = sf::Mouse::getPosition(window);
        glm::vec2 pixel_pos = glm::vec2(pixel_tmp.x, pixel_tmp.y);

        GLuint id = scene.pick(pixel_pos, window, picking, camera);
        std::cout << "ID cliccato: " << id << std::endl;
        shaders.use();

        if (id > 0)
        {
            const auto& objs = scene.get_objects();
            for (const auto& obj : objs){
                if (obj->id == id)
                    scene.active = obj.get();
            }
        }
        else if (id == 0) scene.active = nullptr;
    }
}

void handle (const sf::Event::MouseButtonReleased* mouse, Camera& camera)
{
    if (mouse->button == sf::Mouse::Button::Middle)
        camera.drag_stop ();
}

void handle (const sf::Event::Resized* resized, Camera& camera, Scene& scene)
{
    glViewport (0, 0, resized->size.x, resized->size.y);
    camera.resize(resized->size.x, resized->size.y);
    scene.resize_picking_buffer(resized->size.x, resized->size.y);
}

//////////
// Main //
//////////

int main (int argc, char* argv[])
{
    // optional command line arguments: extra mesh files to open
    std::vector<std::string> meshfiles;
    for (int i = 1; i < argc; ++i)
        meshfiles.push_back (argv[i]);

    //// Startup ////

    Setup setup;
    sf::Window& window = *setup.window;

    Gui gui (window, 800, 800);

    Shaders shaders ("vertex.vert", "fragment.frag");

    shaders.use ();

    Shaders picking ("vertex.vert", "picking.frag");

    Camera camera (shaders);

    // the cube is always the first object loaded into the scene
    Scene scene ("data/cube.off", shaders);
    scene.add_light(shaders);

    for (const auto& meshfile : meshfiles)
        scene.add_obj (meshfile, shaders);

    Grid grid (shaders);
    Axes axes (shaders);

    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //// Main Loop ////

    sf::Clock clock;
    bool running = true;
    while (running)
    {
        while (const std::optional event = window.pollEvent ())
        {
            gui.process_event (window, *event);

            if (event->is<sf::Event::Closed> ())
                running = false;
            else if (const auto* resized = event->getIf<sf::Event::Resized> ())
                handle (resized, camera, scene);
            else if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed> ())
            {
                if (scene.active)
                    handle (*key_pressed, *scene.active, scene);
                handle(*key_pressed, shaders, camera);
            }
            else if (const auto* mouse = event->getIf<sf::Event::MouseMoved> ())
                handle (mouse, camera, scene.active);
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled> ())
                handle (wheel, camera);
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
                handle(mouse, scene, picking, shaders, window, camera);
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonReleased>())
                handle(mouse, camera);
        }

        sf::Time elapsed = clock.restart ();
        gui.new_frame (window, elapsed);

        float window_width = (float) window.getSize().x;
        draw_view_panel (camera);
        draw_outliner (scene, window_width);
        draw_light_panel (scene, window_width);

        scene.draw (shaders);
        grid.draw ();
        axes.draw ();
        gui.render ();
        window.display ();
    }

    return 0;
}
