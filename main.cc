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

class Scene
{
private:
    
    std::vector<std::unique_ptr<Object>> objects;
    GLuint FBO;
    GLuint pickingTexture;
    GLuint RBO;
    GLuint obj_id = 1;

public:

    Object* active = nullptr;

    Scene (std::string filename, Shaders& shaders)
    {
        initPickingBuffer(800, 800);
        add_obj(filename,shaders);
        add_light(shaders);
        active = objects.at(0).get();
    }

    void add_obj(std::string filename, Shaders& shaders)
    {
        objects.push_back(std::make_unique<Mesh>(filename, shaders, obj_id++));
    }

    void add_light(Shaders& shaders)
    {
        objects.push_back(std::make_unique<Light>(shaders, obj_id++));
    }

    const std::vector<std::unique_ptr<Object>>& get_objects() const {
        return objects;
    }

    GLuint pick(glm::vec2 pos, sf::Window& window, Shaders& shader, Camera&camera)
    {
        sf::Vector2u size = window.getSize();

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        camera.apply_vp(shader);

        GLint id_loc = glGetUniformLocation(shader.program, "object_id");
        GLint model_loc = glGetUniformLocation(shader.program, "m");

        for (const auto& obj : objects) {
            // Inviamo l'ID univoco di questa specifica mesh allo shader
            glUniform1ui(id_loc, obj->id);
            obj->draw_obj(model_loc);
        }

        unsigned int pickedID = 0;
        glReadPixels(pos.x, size.y - pos.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pickedID);

        std::cout << "DEBUG: pixel_id letto dal buffer è: " << pickedID << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return pickedID;
    }

    void draw(Shaders& shader){

        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

        GLint model_loc = glGetUniformLocation(shader.program, "m");

        for (const auto& obj : objects)
        {
            obj->draw_obj(model_loc);
        }
    }

    void resize_picking_buffer(int width, int height)
    {
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, width, height, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);

        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    }

private:
    void initPickingBuffer(int width, int height)
    {
        glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenTextures(1, &pickingTexture);
        glBindTexture(GL_TEXTURE_2D, pickingTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

        glGenRenderbuffers(1, &RBO);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

        resize_picking_buffer(width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

};



////////////////////
// SFML Callbacks //
////////////////////

void handle (const sf::Event::KeyPressed& key, Shaders& shaders, Camera& camera, Object& object, Scene& scene)
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
            object.translation(dx,-dy,camera.inv_v, camera.od, camera.fd);
        else if (object.mode == Object::TransformMode::Rotate)
            object.rotation(dx, dy, camera.inv_v, camera.od, camera.fd);
        else if (object.mode == Object::TransformMode::Scale)
            object.scale(dx,dy, camera.od, camera.fd);
    }
    else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)){
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
            camera.pan(dx,dy);
    }
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
        camera.drag (dx, dy);
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
    }
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

    Shaders shaders ("vertex.vert", "fragment.frag");

    shaders.use ();


    Shaders picking ("vertex.vert", "picking.frag");


    Camera camera (shaders);

    // the cube is always the first object loaded into the scene
    Scene scene ("data/cube.off", shaders);

    for (const auto& meshfile : meshfiles)
        scene.add_obj (meshfile, shaders);

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
                handle (resized, camera, scene);
            else if (const auto* key_pressed = event->getIf<sf::Event::KeyPressed> ())
                handle (*key_pressed, shaders, camera, *scene.active, scene);
            else if (const auto* mouse = event->getIf<sf::Event::MouseMoved> ())
                handle (mouse, camera, *scene.active);
            else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled> ())
                handle (wheel, camera);
            else if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
                handle(mouse, scene, picking, shaders, window, camera);
        }

        scene.draw (shaders);
        window.display ();
    }

    return 0;
}
