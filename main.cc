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
    // data to be drawn
    std::vector<float> points;
    std::vector<unsigned int> indices;
    GLuint vbo;
    GLuint ebo;
    GLuint vao;

public:
    Scene (std::string filename) { load (filename); }
    ~Scene () { clean (); }

    void load (std::string filename)
    {
        Mesh mesh (filename);
        mesh.pack4gpu (points, indices);
        send_arrays_2a3f ();
    }

    void clean ()
    {
        glDeleteVertexArrays (1, &vao);
        glDeleteBuffers (1, &vbo);
    }

    // // when data will be dynamically loaded, reloading will be useful
    // void reload ()
    // {
    //     clean ();
    //     load ();
    // }

    void draw ()
    {
        // clear the buffers
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw all elements as described by indices
        glDrawElements(GL_TRIANGLES, indices.size (), GL_UNSIGNED_INT, 0);
    }

private:
    // send to the gpu the mesh arrays:
    // - the mesh vertices, 2 attributes, 3 floats each
    // - the mesh indices
    void send_arrays_2a3f ()
    {
        // we want just one buffer, and we retrieve the name OpenGL assigns to it.
        glGenBuffers (1, &vbo);
        // bind it as the current VBO
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        // transfer data from CPU RAM to GPU RAM.
        glBufferData (GL_ARRAY_BUFFER,
                      points.size () * sizeof (float),
                      points.data (),
                      GL_STATIC_DRAW);

        // we want just one buffer container, and we retrieve the name OpenGL assigns to it.
        glGenVertexArrays (1, &vao);
        // bind it as the current vao.
        glBindVertexArray (vao);

        // Attribute 0: position (x, y, z)
        glVertexAttribPointer (0,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               6 * sizeof(float),
                               (void*)0);
        glEnableVertexAttribArray (0);

        // Attribute 1: 3 generic floats (u, v, w)
        glVertexAttribPointer (1,
                               3,
                               GL_FLOAT,
                               GL_FALSE,
                               6 * sizeof(float),
                               (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray (1);

        glGenBuffers(1, &ebo); 
        // MUST be bound after the VAO's binding!
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size () * sizeof (unsigned int),
                     indices.data (),
                     GL_STATIC_DRAW);
    }
};



////////////////////
// SFML Callbacks //
////////////////////

void handle (const sf::Event::KeyPressed& key, Shaders& shaders, Camera& camera)
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
    default:
        return;
    }
}

void handle (const sf::Event::MouseMoved* mouse, Camera& camera)
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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)){
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
    Scene scene (meshfile);

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
                handle (*key_pressed, shaders, camera);
            else if (const auto* mouse = event->getIf<sf::Event::MouseMoved> ())
                handle (mouse, camera);
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled> ())
                handle (wheel, camera);
        }

        scene.draw ();
        window.display ();
    }

    return 0;
}
