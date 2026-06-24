#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

#include <iostream>
#include <cstdlib>

class Camera
{
private:
    struct aspect{
        float width;
        float height;
    };

    GLint vp_loc;

    fcg::Trackball trackball;
    bool dragging = false;

    bool ortographic = false;

    const float normal_fd = 6.0;
    const float tele_fd = 100.0;
    const float wide_fd = 1.5;

    // how far from the origin the scene is allowed to extend
    // (near/far clip and lateral FOV are sized around this, independently of zoom)
    const float world_radius = 5.0;

    float pan_x = 0.f;
    float pan_y = 0.f;

    GLint camera_pos_loc;  // xyz
    glm::vec3 camera_pos = {0.0, 0.0, 0.0};   // xyz

    GLint ambient_light_loc;

public:

    glm::vec3 ambient_light = {0.1f, 0.1f, 0.1f};

    glm::mat4 inv_v;
    glm::mat4 pr;
    glm::mat4 vp;
    aspect aspect = {800,800};

    float fd; // focal distance
    float od; // object distance

    Camera (Shaders& shaders)
    {
        vp_loc = glGetUniformLocation (shaders.program, "vp");
        camera_pos_loc = glGetUniformLocation (shaders.program, "camera_pos");

        ambient_light_loc = glGetUniformLocation (shaders.program, "ambient_light");
        update_ambient ();

        view_normal ();
    }

    void update_ambient ()
    {
        glUniform3fv (ambient_light_loc, 1, &ambient_light[0]);
    }

    void drag (float x, float y)
    {
        ortographic = false;
        if (!dragging) {
            trackball.start (x, y);
            dragging = true;
        } else {
            trackball.move (x, y);
        }
        update ();
    }

    void drag_stop ()
    {
        dragging = false;
        trackball.stop ();
    }

    void zoom (float dy)
    {
        float ratio = fd / 100.0;
        fd += dy * ratio;
        if (fd < 0.1)
            fd = 0.1;
        //std::cout << "F:" << fd << std::endl;
        update ();
    }

    void pan (float dx, float dy)
    {
        pan_x += dx * 0.005 * (od/fd);
        pan_y -= dy * 0.005 * (od/fd);
        update();
    }

    void dolly (float dy)
    {
        float ratio = od / 10.0;
        od -= dy * ratio; // note: we go in the opposite direction of zoooming
        if (od < 0.5)
            od = 0.5;
        //std::cout << "D:" << od << std::endl;        
        update ();
    }

    void view_tele ()
    {
        fd = tele_fd;
        od = tele_fd;
        update ();
    }

    void view_normal ()
    {
        fd = normal_fd;
        od = normal_fd;
        update ();
    }

    void view_wide ()
    {
        fd = wide_fd;
        od = wide_fd;
        update ();
    }

    void apply_vp (Shaders& shader)
    {
        GLint loc = glGetUniformLocation (shader.program, "vp");
        glUniformMatrix4fv (loc, 1, GL_FALSE, &vp[0][0]);
    }

    void resize (float x, float y)
    {
        aspect.width = x;
        aspect.height = y;
        update();
    }

    void view_front()
    {
        glm::mat4 look_at = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0));
        trackball.set_rotation(glm::toQuat(glm::mat3(look_at)));
        ortographic = true;
        update();
    }

    void view_top()
    {
        glm::mat4 look_at = glm::lookAt(glm::vec3(0,1,0), glm::vec3(0,0,0), glm::vec3(0,0,-1));
        trackball.set_rotation(glm::toQuat(glm::mat3(look_at)));
        ortographic = true;
        update();
    }
    
    void view_right()
    {
        glm::mat4 look_at = glm::lookAt(glm::vec3(1,0,0), glm::vec3(0,0,0), glm::vec3(0,1,0));
        trackball.set_rotation(glm::toQuat(glm::mat3(look_at)));
        ortographic = true;
        update();
    }

private:

    void update ()
    {
        float ratio = aspect.height / aspect.width;

        float ncp = od - world_radius; // distance near clip plane
        if (ncp < 0.1)
            ncp = 0.1;
        float fcp = od + world_radius; // distance far clip plane

        // fd controls zoom (lens), world_radius controls how much of the
        // world is framed at a given zoom level — the two are independent.
        float lens = fd / world_radius;

        // tieni il trackball sincronizzato con la finestra e con il FOV
        // attuale, così la rotazione "sente" sempre coerente con lo zoom.
        trackball.set_window_size ((int) aspect.width, (int) aspect.height);
        trackball.set_view (od, 1.0f / lens);

        glm::mat4 r = trackball.rotation_matrix ();

        // prepare translation matrix
        glm::mat4 t = glm::mat4(
                                 1.0, 0.0, 0.0, 0.0, // 1st column
                                 0.0, 1.0, 0.0, 0.0, // 2nd column
                                 0.0, 0.0, 1.0, 0.0, // 3rd column
                                 pan_x, pan_y, -od, 1.0  // translate object along the Z axis
                                 );

        if (ortographic){
            float half_height = od / lens;
            float half_width  = half_height / ratio;
            float coeff_x = 1.0 / half_width;
            float coeff_y = 1.0 / half_height;

            float a_ortho = -2.0 / (fcp - ncp);
            float b_ortho = -(fcp + ncp) / (fcp - ncp);

            pr = glm::mat4(
            coeff_x, 0.0, 0.0,     0.0,
            0.0, coeff_y, 0.0,     0.0,
            0.0, 0.0,    a_ortho,  0.0,   // qui 0, non -1
            0.0, 0.0,    b_ortho,  1.0    // qui 1, non 0
            );
        }
        else{
            // prepare projection matrix
            float a = (fcp + ncp) / (ncp - fcp);       // coefficient 3rd col
            float b = 2.0 * fcp * ncp / (ncp - fcp);   // coefficient 4th col

            pr = glm::mat4(
                            lens * ratio,  0.0, 0.0,  0.0,    // 1st column
                            0.0,  lens, 0.0,  0.0,    // 2nd column
                            0.0, 0.0,   a, -1.0,    // 3rd column
                            0.0, 0.0,   b,  0.0     // 4th column
                            );
        }

        // Compute VP matrix and update it
        glm::mat4 v = t * r;
        vp = pr * v;
        glUniformMatrix4fv (vp_loc, 1, GL_FALSE, &vp[0][0]);

        inv_v = glm::inverse (v);

        glm::vec4 cp4 = {0.0, 0.0, 0.0, 1.0};
        cp4 = inv_v * cp4;
        glm::vec3 cp3 = {cp4.x, cp4.y, cp4.z};
        glUniform3fv(camera_pos_loc, 1, &cp3[0]);
    }
};