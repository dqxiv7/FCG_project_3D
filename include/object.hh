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
    std::string name;

    bool moving = false;

    enum class TransformMode {None, Translate, Rotate, Scale};
    TransformMode mode = TransformMode::None;

    fcg::Trackball trackball;
    bool trackball_active = false;

    void translation(float dx, float dy, glm::mat4 inv, float od, float fd)
    {
      glm::vec3 cam_right = glm::normalize(glm::vec3(inv[0]));
      glm::vec3 cam_up = glm::normalize(glm::vec3(inv[1]));

      float sensitivity = 0.005f * (od / fd);

      glm::vec3 displacement = (cam_right * dx + cam_up * dy) * sensitivity;

      position += displacement;

      compute_mm();
    }

    void rotation(float mouse_x, float mouse_y, glm::mat4 inv, float win_width, float win_height)
    {
      trackball.set_window_size ((int)win_width, (int)win_height);

      if (!trackball_active) {
          trackball.start (mouse_x, mouse_y);
          trackball_active = true;
      } else {
          trackball.move (mouse_x, mouse_y);
      }

      // La rotazione del trackball è calcolata in uno spazio locale fisso
      // (x schermo, y schermo, profondità virtuale). Per applicarla come
      // rotazione mondo coerente con la vista attuale della camera, la
      // portiamo nella base della camera (cam_right/up/forward, le colonne
      // di inv) e poi torniamo indietro: R_world = base * R_local * base^-1.
      glm::mat3 cam_basis = glm::mat3 (inv);
      glm::mat3 local_rotation = glm::mat3 (trackball.rotation_matrix ());
      glm::mat3 world_rotation = cam_basis * local_rotation * glm::transpose (cam_basis);

      R_m = glm::mat4 (world_rotation);

      compute_mm();
    }

    void scale (float dx, float dy, float od, float fd){
      
      float sensitivity = 0.005f * (od / fd);

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

    virtual void draw_obj(GLint model_loc, GLint normal_matrix_loc)
      {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, &M[0][0]);

        glm::mat3 normal_matrix = glm::transpose (glm::inverse (glm::mat3 (M)));
        glUniformMatrix3fv(normal_matrix_loc, 1, GL_FALSE, &normal_matrix[0][0]);

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