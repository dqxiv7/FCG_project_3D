class Scene
{
private:
    
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<Light*> lights;

    GLuint PFBO;
    GLuint pickingTexture;
    GLuint RBO;

    GLuint obj_id = 1;

public:

    // deve combaciare con #define MAX_LIGHTS in fragment.frag — l'array
    // di luci nello shader ha questa dimensione fissa.
    static constexpr size_t MAX_LIGHTS = 4;

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
        if (lights.size() >= MAX_LIGHTS)
            return;

        auto light = std::make_unique<Light>(shaders, obj_id++, (int)lights.size());
        lights.push_back(light.get());
        objects.push_back(std::move(light));

        GLint num_lights_loc = glGetUniformLocation(shaders.program, "num_lights");
        glUniform1i(num_lights_loc, (GLint)lights.size());
    }

    const std::vector<std::unique_ptr<Object>>& get_objects() const {
        return objects;
    }

    void set_active(Object* obj)
    {
        if (active) {
            active->moving = false;
            active->mode = Object::TransformMode::None;
            active->trackball_active = false;
        }
        active = obj;
    }
    
    void remove_active(Shaders& shaders)
    {
        if (!active)
            return;

        Light* light = dynamic_cast<Light*>(active);
        if (light) {
            for (size_t i = 0; i < lights.size(); i++) {
                if (lights[i] == light) {
                    lights.erase(lights.begin() + i);
                    break;
                }
            }
            for (size_t i = 0; i < lights.size(); i++) {
                lights[i]->locations(shaders, (int)i);
                lights[i]->update_color();
                lights[i]->compute_mm();
            }

            GLint num_lights_loc = glGetUniformLocation(shaders.program, "num_lights");
            glUniform1i(num_lights_loc, (GLint)lights.size());
        }

        for (size_t i = 0; i < objects.size(); i++) {
            if (objects[i].get() == active) {
                objects.erase(objects.begin() + i);
                break;
            }
        }

        active = nullptr;
    }

    const std::vector<Light*>& get_lights() const {
        return lights;
    }

    GLuint pick(glm::vec2 pos, sf::Window& window, Shaders& shader, Camera&camera)
    {
        sf::Vector2u size = window.getSize();

        glBindFramebuffer(GL_FRAMEBUFFER, PFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        camera.apply_vp(shader);

        GLint id_loc = glGetUniformLocation(shader.program, "object_id");
        GLint model_loc = glGetUniformLocation(shader.program, "m");
        GLint normal_matrix_loc = glGetUniformLocation(shader.program, "normal_matrix");

        for (const auto& obj : objects) {
            // Inviamo l'ID univoco di questa specifica mesh allo shader
            glUniform1ui(id_loc, obj->id);
            obj->draw_obj(model_loc, normal_matrix_loc);
        }

        unsigned int pickedID = 0;
        glReadPixels(pos.x, size.y - pos.y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &pickedID);

        std::cout << "DEBUG: pixel_id letto dal buffer è: " << pickedID << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return pickedID;
    }

    void draw(Shaders& shader){

        glClearColor (0.2f, 0.2f, 0.2f, 1.0f);
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLint model_loc = glGetUniformLocation(shader.program, "m");
        GLint normal_matrix_loc = glGetUniformLocation(shader.program, "normal_matrix");

        for (const auto& obj : objects)
        {
            obj->draw_obj(model_loc, normal_matrix_loc);
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
        glGenFramebuffers(1, &PFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, PFBO);

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