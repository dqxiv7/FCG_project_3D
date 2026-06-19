class Object{
  protected:
    glm::vec3 position = {0,0,0};
    glm::vec3 rotation = {0,0,0};
    glm::vec3 scale = {1,1,1};
    glm::vec3 origin = {0,0,0};

    std::vector<float> points;
    std::vector<unsigned int> indices;

    GLint model;

    GLuint vbo;
    GLuint ebo;
    GLuint vao;

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

    ~Object(){
      clean();
    }

  void compute_mm(){
    glm::mat4 I = glm::mat4(1.f);

    glm::mat4 M = glm::translate(I, position);
    
    M = glm::rotate(M, glm::radians(rotation.x), glm::vec3(1.f,0.f,0.f));
    M = glm::rotate(M, glm::radians(rotation.y), glm::vec3(0.f,1.f,0.f));
    M = glm::rotate(M, glm::radians(rotation.z), glm::vec3(0.f,0.f,1.f));
  
    M = glm::scale(M,scale);

    M = glm::translate(M, -origin);

    glUniformMatrix4fv(model, 1, GL_FALSE, &M[0][0]);
  }

  void draw_obj()
    {
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
    }
};