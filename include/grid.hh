class Grid
{
  private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertex_count = 0;

    GLint model_loc;
    GLint unlit_loc;
    GLint unlit_color_loc;
    GLint unlit_alpha_loc;

    glm::vec3 color = {0.4f, 0.4f, 0.4f};
    float alpha = 0.4f;

  public:
    Grid (Shaders& shaders, float extent = 5.0f, float spacing = 1.0f)
    {
        std::vector<float> verts;

        for (float v = -extent; v <= extent + 1e-4f; v += spacing)
        {
            // linea parallela all'asse X, a profondità Z = v
            verts.insert (verts.end(), { -extent, 0.0f, v,  extent, 0.0f, v });
            // linea parallela all'asse Z, a X = v
            verts.insert (verts.end(), { v, 0.0f, -extent,  v, 0.0f, extent });
        }

        vertex_count = (GLsizei) (verts.size () / 3);

        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (GL_ARRAY_BUFFER, verts.size () * sizeof (float), verts.data (), GL_STATIC_DRAW);

        glGenVertexArrays (1, &vao);
        glBindVertexArray (vao);
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void*) 0);
        glEnableVertexAttribArray (0);
        glBindVertexArray (0);

        model_loc = glGetUniformLocation (shaders.program, "m");
        unlit_loc = glGetUniformLocation (shaders.program, "unlit");
        unlit_color_loc = glGetUniformLocation (shaders.program, "unlit_color");
        unlit_alpha_loc = glGetUniformLocation (shaders.program, "unlit_alpha");
    }

    void draw ()
    {
        glm::mat4 identity (1.0f);
        glUniformMatrix4fv (model_loc, 1, GL_FALSE, &identity[0][0]);

        glUniform1i (unlit_loc, 1);
        glUniform3fv (unlit_color_loc, 1, &color[0]);
        glUniform1f (unlit_alpha_loc, alpha);

        glBindVertexArray (vao);
        glDrawArrays (GL_LINES, 0, vertex_count);
        glBindVertexArray (0);

        glUniform1i (unlit_loc, 0);
    }
};

class Axes
{
  private:
    GLuint vao = 0;
    GLuint vbo = 0;

    GLint model_loc;
    GLint unlit_loc;
    GLint unlit_color_loc;
    GLint unlit_alpha_loc;

    float alpha = 0.4f;

  public:
    Axes (Shaders& shaders, float length = 5.0f)
    {
        float verts[] = {
            -length, 0.0f, 0.0f,   length, 0.0f, 0.0f,  // X
            0.0f, 0.0f, -length,   0.0f, 0.0f, length   // Z
        };

        glGenBuffers (1, &vbo);
        glBindBuffer (GL_ARRAY_BUFFER, vbo);
        glBufferData (GL_ARRAY_BUFFER, sizeof (verts), verts, GL_STATIC_DRAW);

        glGenVertexArrays (1, &vao);
        glBindVertexArray (vao);
        glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (float), (void*) 0);
        glEnableVertexAttribArray (0);
        glBindVertexArray (0);

        model_loc = glGetUniformLocation (shaders.program, "m");
        unlit_loc = glGetUniformLocation (shaders.program, "unlit");
        unlit_color_loc = glGetUniformLocation (shaders.program, "unlit_color");
        unlit_alpha_loc = glGetUniformLocation (shaders.program, "unlit_alpha");
    }

    void draw ()
    {
        glm::mat4 identity (1.0f);
        glUniformMatrix4fv (model_loc, 1, GL_FALSE, &identity[0][0]);
        glUniform1i (unlit_loc, 1);
        glUniform1f (unlit_alpha_loc, alpha);

        glBindVertexArray (vao);

        glm::vec3 red  = {1.0f, 0.1f, 0.1f};
        glm::vec3 green = {0.1f, 1.0f, 0.1f};

        glUniform3fv (unlit_color_loc, 1, &red[0]);
        glDrawArrays (GL_LINES, 0, 2);

        glUniform3fv (unlit_color_loc, 1, &green[0]);
        glDrawArrays (GL_LINES, 2, 2);

        glBindVertexArray (0);
        glUniform1i (unlit_loc, 0);
    }
};
