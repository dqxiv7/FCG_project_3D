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
        settings.antiAliasingLevel = 4;
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

class Scene
{
private:
    
    std::vector<std::unique_ptr<Object>> objects;

public:
    Scene (std::string filename, Shaders& shaders) 
    {
        add_obj(filename,shaders);
    }

    void add_obj(std::string filename, Shaders& shaders)
    {
        objects.push_back(std::make_unique<Mesh>(filename, shaders));
    }

    Object& get_obj()
    {
        return *objects.at(0);
    }

    void draw(){

        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

        for (const auto& obj : objects)
        {
            obj->draw_obj();
        }
    }

private:
    // send to the gpu the mesh arrays:
    // - the mesh vertices, 2 attributes, 3 floats each
    // - the mesh indices

};



////////////////////
// SFML Callbacks //
////////////////////

void handle (const sf::Event::KeyPressed& key, Shaders& shaders, Camera& camera, Object& object)
{

    switch (key.scancode) {
    case sf::Keyboard::Scancode::Space:
        shaders.reload ("00_vertex.vert", "00_fragment.frag");
        shaders.use ();
        return;
    case sf::Keyboard::Scancode::N:
        camera.view_normal ();
        return;
    case sf::Keyboard::Scancode::T:
        camera.view_tele ();
        return;
    case sf::Keyboard::Scancode::W:
        camera.view_wide ();
        return;
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

void handle (const sf::Event::MouseMoved* mouse, Camera& camera, Object& object)
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

    if (object.moving){
        if (object.mode == Object::TransformMode::Translate)
            object.translation(dx,-dy,camera.inv_v);
        else if (object.mode == Object::TransformMode::Rotate)
            object.rotation(dx, dy, camera.inv_v);
        else if (object.mode == Object::TransformMode::Scale)
            object.scale(dx,dy);
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)){
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
            camera.pan(dx,dy);
    }
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        camera.drag (dx, dy);
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt))
        camera.dolly(dy);
}

void handle (const sf::Event::MouseWheelScrolled* wheel, Camera& camera){
    camera.zoom(wheel->delta);
}

//////////
// Main //
//////////

int main (int argc, char* argv[])
{
    // mandatory command line argument: mesh file to open
    std::string meshfile = "";
    if (argc > 1)
        meshfile = argv[1];
    else {
        meshfile = "data/cube.off";
    }

    //// Startup ////

    Setup setup;
    sf::Window& window = *setup.window;

    Shaders shaders ("00_vertex.vert", "00_fragment.frag");
    shaders.use ();

    Camera camera (shaders);
    Scene scene (meshfile, shaders);

    scene.add_obj("data/dragon.off",shaders);

    glEnable (GL_CULL_FACE);
    glCullFace (GL_BACK);

    glEnable (GL_DEPTH_TEST);


    //// Main Loop ////

    bool running = true;
    while (running)
    {
        while (const std::optional event = window.pollEvent ())
        {
            if (event->is<sf::Event::Closed> ())
                running = false;
            else if (const auto* resized = event->getIf<sf::Event::Resized> ())
                glViewport (0, 0, resized->size.x, resized->size.y);
            else if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed> ())
                handle (*key_pressed, shaders, camera, scene.get_obj());
            else if (const auto* mouse = event->getIf<sf::Event::MouseMoved> ())
                handle (mouse, camera, scene.get_obj());
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled> ())
                handle (wheel, camera);
        }

        scene.draw ();
        window.display ();
    }

    return 0;
}
