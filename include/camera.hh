#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

#include <iostream>
#include <cstdlib>

class Camera
{
private:
    GLint vp_loc;
    float phi_deg = 210.0;
    float theta_deg = 2.0;

    const float normal_fd = 4.0;
    const float tele_fd = 100.0;
    const float wide_fd = 1.5;

    float fd; // focal distance
    float od; // object distance

    float pan_x = 0.f;
    float pan_y = 0.f;

    GLint camera_pos_loc;  // xyz
    glm::vec3 camera_pos = {0.0, 0.0, 0.0};   // xyz

    // lights and materials
    GLint light_direct_pos_loc;   // xyz
    GLint light_direct_val_loc;   // rgb
    GLint light_ambient_val_loc;  // rgb
    GLint material_diffuse_loc;   // rgb
    GLint material_ambient_loc;   // rgb
    GLint material_specular_loc;  // rgb
    GLint material_shininess_loc; // scalar

    glm::vec3 light_direct_pos = {2.0, 2.0, 0.0};   // xyz
    glm::vec3 light_direct_val = {1.0, 0.8, 0.6};   // rgb
    glm::vec3 light_ambient_val = {0.5, 0.5, 1.0};  // rgb
    glm::vec3 material_diffuse = {0.7, 0.7, 0.7};   // rgb
    glm::vec3 material_ambient = {0.1, 0.1, 0.1};   // rgb
    glm::vec3 material_specular = {0.7, 0.7, 0.7};  // rgb
    float material_shininess = 3.0; // scalar

    
public:
    Camera (Shaders& shaders)
    {
        vp_loc = glGetUniformLocation (shaders.program, "vp");
        camera_pos_loc = glGetUniformLocation (shaders.program, "camera_pos");

        light_direct_pos_loc = glGetUniformLocation (shaders.program, "light.direct_pos");
        light_direct_val_loc = glGetUniformLocation (shaders.program, "light.direct_val");
        light_ambient_val_loc = glGetUniformLocation (shaders.program, "light.ambient_val");
        material_diffuse_loc = glGetUniformLocation (shaders.program, "material.diffuse");
        material_ambient_loc = glGetUniformLocation (shaders.program, "material.ambient");
        material_specular_loc = glGetUniformLocation (shaders.program, "material.specular");
        material_shininess_loc = glGetUniformLocation (shaders.program, "material.shininess");

        glUniform3fv (light_direct_val_loc, 1, &light_direct_val[0]);
        glUniform3fv (light_ambient_val_loc, 1, &light_ambient_val[0]);
        glUniform3fv (material_diffuse_loc, 1, &material_diffuse[0]);
        glUniform3fv (material_ambient_loc, 1, &material_ambient[0]);
        glUniform3fv (material_specular_loc, 1, &material_specular[0]);
        glUniform1fv (material_shininess_loc, 1, &material_shininess);

        view_normal ();
        update ();
    }

    void drag (float dx, float dy)
    {
        phi_deg += dx * 0.2;
        theta_deg += dy * 0.2;
        theta_deg = theta_deg > 90.0? 90.0 : theta_deg;
        theta_deg = theta_deg < -90.0? -90.0 : theta_deg;
        update ();
    }

    void zoom (float dy)
    {
        float ratio = fd / 10.0;
        fd += dy * ratio;
        if (fd < 0.1)
            fd = 0.1;
        //std::cout << "F:" << fd << std::endl;
        update ();
    }

    void pan (float dx, float dy)
    {
        pan_x += dx * 0.005;
        pan_y -= dy * 0.005;
        update();
    }

    void dolly (float dy)
    {
        float ratio = od / 100.0;
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

private:

    void update ()
    {    
        float ncp = od - 1.0; // distance near clip plane
        if (ncp < 0.1)
            ncp = 0.1;
        float fcp = od + 1.0; // distance far clip plane

        // prepare rotation matrices
        //// Convert degrees to radians, compute sin and cos
        float ps = glm::sin (glm::radians (phi_deg));
        float pc = glm::cos (glm::radians (phi_deg));
        glm::mat4 ry = glm::mat4(
                                  pc, 0.0, -ps, 0.0, // 1st column
                                 0.0, 1.0, 0.0, 0.0, // 2nd column
                                  ps, 0.0,  pc, 0.0, // 3rd column
                                 0.0, 0.0, 0.0, 1.0
                                 );
        //// Convert degrees to radians, compute sin and cos
        float ts = glm::sin (glm::radians (theta_deg));
        float tc = glm::cos (glm::radians (theta_deg));
        glm::mat4 rx = glm::mat4(
                                 1.0, 0.0, 0.0, 0.0, // 1st column
                                 0.0,  tc, ts,  0.0, // 2nd column
                                 0.0, -ts, tc,  0.0, // 3rd column
                                 0.0, 0.0, 0.0, 1.0
                                 );

        // prepare translation matrix
        glm::mat4 t = glm::mat4(
                                 1.0, 0.0, 0.0, 0.0, // 1st column
                                 0.0, 1.0, 0.0, 0.0, // 2nd column
                                 0.0, 0.0, 1.0, 0.0, // 3rd column
                                 pan_x, pan_y, -od, 1.0  // translate object along the Z axis
                                 );

        // prepare projection matrix
        float a = (fcp + ncp) / (ncp - fcp);       // coefficient 3rd col
        float b = 2.0 * fcp * ncp / (ncp - fcp);   // coefficient 4th col

        /*** NOTE *******************************************************
         **  We use fd directly as coefficient in the first two lines. **
         **  It works because our scene is in a unitary cube.          **
         **  If the image plane is centered about the view axis, with  **
         **  width 2r and height 2t in view space, the coefficients    **
         **  containing fd must be scaled accordingly.                 **
         ****************************************************************/
        glm::mat4 pr = glm::mat4(
                                 fd,  0.0, 0.0,  0.0,    // 1st column
                                 0.0,  fd, 0.0,  0.0,    // 2nd column
                                 0.0, 0.0,   a, -1.0,    // 3rd column
                                 0.0, 0.0,   b,  0.0     // 4th column
                                 );

        // Compute VP matrix and update it
        glm::mat4 v = t * rx * ry; 
        glm::mat4 vp; 
        vp = pr * v;
        glUniformMatrix4fv (vp_loc, 1, GL_FALSE, &vp[0][0]);

        glm::mat4 inv_v = glm::inverse (v);

        glm::vec4 cp4 = {0.0, 0.0, 0.0, 1.0};
        cp4 = inv_v * cp4;
        glm::vec3 cp3 = {cp4.x, cp4.y, cp4.z};
        glUniform3fv(camera_pos_loc, 1, &cp3[0]);

        glm::vec4 ldp4 = glm::vec4 (light_direct_pos, 1.0);
        ldp4 = inv_v  * ldp4;
        glm::vec3 ldp3 = {ldp4.x, ldp4.y, ldp4.z};
        glUniform3fv (light_direct_pos_loc, 1, &ldp3[0]);
    }
};