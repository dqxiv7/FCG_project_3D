class Light : public Mesh
{
  public:
    glm::vec3 light_color = {1.0f, 1.0f, 1.0f};
    float light_intensity = 1.0f;
    glm::vec3 light_direct_val = {1.0, 1.0, 1.0};   // rgb
    glm::vec3 light_ambient_val = {0.1, 0.1, 0.1};  // rgb

  private:
    // lights and materials
    GLint light_direct_pos_loc;   // xyz
    GLint light_direct_val_loc;   // rgb
    GLint light_ambient_val_loc;  // rgb
    GLint unlit_loc;
    GLint unlit_color_loc;
    GLint unlit_alpha_loc;
    GLuint color_program;

    glm::vec3 gizmo_color = {1.0f, 0.65f, 0.2f};

  public:
    Light (Shaders& shaders, GLuint pick_id, int index)
      : Mesh ("data/sphere.off", shaders, pick_id)
    {
        name = "Light " + std::to_string(index + 1);

        scale_factor *= 0.05f;

        float angle = glm::radians (90.0f * index);
        position = {0.8f * cos (angle), 0.6f, 0.8f * sin (angle)};

        locations (shaders, index);
        update_color();
        compute_mm();
    }

    void update_color()
    {
        light_direct_val = light_color * light_intensity;
        parameters();
    }

    void locations (Shaders& shaders, int index)
    {
        std::string prefix = "lights[" + std::to_string (index) + "].";
        light_direct_pos_loc = glGetUniformLocation (shaders.program, (prefix + "direct_pos").c_str());
        light_direct_val_loc = glGetUniformLocation (shaders.program, (prefix + "direct_val").c_str());
        light_ambient_val_loc = glGetUniformLocation (shaders.program, (prefix + "ambient_val").c_str());
        unlit_loc = glGetUniformLocation (shaders.program, "unlit");
        unlit_color_loc = glGetUniformLocation (shaders.program, "unlit_color");
        unlit_alpha_loc = glGetUniformLocation (shaders.program, "unlit_alpha");
        color_program = shaders.program;
    }

    void parameters ()
    {
        glUniform3fv (light_direct_val_loc, 1, &light_direct_val[0]);
        glUniform3fv (light_ambient_val_loc, 1, &light_ambient_val[0]);
    }

    void compute_mm () override
    {
        Object::compute_mm();

        glm::vec3 ldp3 = glm::vec3 (M[3]);
        glUniform3fv (light_direct_pos_loc, 1, &ldp3[0]);
    }

    void draw_obj (GLint model_loc, GLint normal_matrix_loc) override
    {
        GLint current_program;
        glGetIntegerv (GL_CURRENT_PROGRAM, &current_program);
        bool color_pass = (GLuint)current_program == color_program;

        if (color_pass) {
            glUniform1i (unlit_loc, 1);
            glUniform3fv (unlit_color_loc, 1, &gizmo_color[0]);
            glUniform1f (unlit_alpha_loc, 1.0f);
        }

        Object::draw_obj (model_loc, normal_matrix_loc);

        if (color_pass)
            glUniform1i (unlit_loc, 0);
    }
};
