class Light : public Mesh
{
  public:
    glm::vec3 light_direct_val = {1.0, 1.0, 1.0};   // rgb
    glm::vec3 light_ambient_val = {0.1, 0.1, 0.1};  // rgb

  private:
    // lights and materials
    GLint light_direct_pos_loc;   // xyz
    GLint light_direct_val_loc;   // rgb
    GLint light_ambient_val_loc;  // rgb
    GLint unlit_loc;
    GLuint color_program;

  public:
    Light (Shaders& shaders, GLuint pick_id)
      : Mesh ("data/sphere.off", shaders, pick_id)
    {
        scale_factor *= 0.05f;
        position = {0.6, 0.6, 0.0};

        locations (shaders);
        parameters();
        compute_mm();
    }

    void locations (Shaders& shaders)
    {
        light_direct_pos_loc = glGetUniformLocation (shaders.program, "light.direct_pos");
        light_direct_val_loc = glGetUniformLocation (shaders.program, "light.direct_val");
        light_ambient_val_loc = glGetUniformLocation (shaders.program, "light.ambient_val");
        unlit_loc = glGetUniformLocation (shaders.program, "unlit");
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

    void draw_obj (GLint model_loc) override
    {
        GLint current_program;
        glGetIntegerv (GL_CURRENT_PROGRAM, &current_program);
        bool color_pass = (GLuint)current_program == color_program;

        if (color_pass)
            glUniform1i (unlit_loc, 1);

        Object::draw_obj (model_loc);

        if (color_pass)
            glUniform1i (unlit_loc, 0);
    }
};
