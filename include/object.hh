class Object{
  protected:
    glm::vec3 position = {0,0,0};
    glm::vec3 scale_factor = {1,1,1};
    glm::vec3 origin = {0,0,0};

    glm::mat4 R_m = glm::mat4(1.f);
    glm::mat4 M = glm::mat4(1.f);

    std::vector<float> points;
    std::vector<unsigned int> indices;

    GLuint vbo = 0;
    GLuint ebo = 0;
    GLuint vao = 0;

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

  public:
    Object(){};

    virtual ~Object(){
      clean();
    }

    GLuint id;

    bool moving = false;

    enum class TransformMode {None, Translate, Rotate, Scale};
    TransformMode mode = TransformMode::None;

    void translation(float dx, float dy, glm::mat4 inv)
    {
      glm::vec3 cam_right = glm::normalize(glm::vec3(inv[0]));
      glm::vec3 cam_up = glm::normalize(glm::vec3(inv[1]));

      float sensitivity = 0.005f;

      glm::vec3 displacement = (cam_right * dx + cam_up * dy) * sensitivity;

      position += displacement;

      compute_mm();
    }

    void rotation(float dx, float dy, glm::mat4 inv)
    {

      glm::vec3 cam_right = glm::normalize(glm::vec3(inv[0]));
      glm::vec3 cam_up = glm::normalize(glm::vec3(inv[1]));

      float sensitivity = 0.005f;

      glm::mat4 r_x = glm::rotate(glm::mat4(1.f), dx * sensitivity, cam_up);
      glm::mat4 r_y = glm::rotate(glm::mat4(1.f), dy * sensitivity, cam_right);

      R_m = r_x * r_y * R_m;
      
      compute_mm();
    }

    void scale (float dx, float dy){
      
      float sensitivity = 0.005f;

      float norma = glm::length(glm::vec2(dx,dy));

      float dir = (dx + dy) > 0 ? 1.f : -1.f;

      scale_factor += glm::vec3(norma * dir * sensitivity);

      compute_mm();
    }

    virtual void compute_mm(){
      glm::mat4 I = glm::mat4(1.f);

      M = glm::translate(I, position);
      
      M = M * R_m;
    
      M = glm::scale(M,scale_factor);

      M = glm::translate(M, -origin);
    }

    virtual void draw_obj(GLint model_loc)
      {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &M[0][0]);

        glBindVertexArray(vao);
        // draw all elements as described by indices
        glDrawElements(GL_TRIANGLES, indices.size (), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
      }

  private:
    void clean ()
    {
        glDeleteVertexArrays (1, &vao);
        glDeleteBuffers (1, &vbo);
        glDeleteBuffers (1, &ebo);
    }
};