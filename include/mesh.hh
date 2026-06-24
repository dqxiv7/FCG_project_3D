class Mesh : public Object{
private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::uvec3> triangles;

    GLint material_diffuse_loc;   // rgb
    GLint material_ambient_loc;   // rgb
    GLint material_specular_loc;  // rgb
    GLint material_shininess_loc; // scalar

public:

    glm::vec3 material_diffuse = {0.8, 0.7, 0.6};   // rgb
    glm::vec3 material_ambient = {0.5, 0.5, 0.8};   // rgb
    glm::vec3 material_specular = {1.0, 1.0, 1.0};  // rgb
    float material_shininess = 3.0; // scalar

    Mesh(const std::string& filename, Shaders& shaders, GLuint pick_id) {

        id = pick_id;
        name = std::filesystem::path(filename).stem().string();
        
        std::ifstream file (filename);
 
        if (!file.is_open ()) {
            fprintf (stderr, "Error: Failed to open file: %s\n", filename.c_str ());
            exit (1);
        }

        std::string line;

        // Read OFF header
        std::getline (file, line);
        std::erase (line, '\r'); std::erase (line, '\n');
        if (line != "OFF") {
            fprintf (stderr, "%s\n", line.c_str());
            fprintf (stderr, "Error: Invalid OFF file: missing OFF header\n");
            exit (1);
        }

        // Skip comments and empty lines
        while (std::getline (file, line)) {
            std::erase (line, '\r'); std::erase (line, '\n');
            if (line.empty () || line[0] == '#') {
                continue;
            }
            break;
        }

        // Parse header: vnum fnum ednum
        std::istringstream headerStream (line);
        unsigned int vnum, fnum, ednum;
        if (!(headerStream >> vnum >> fnum >> ednum)) {
            fprintf (stderr, "Error: Invalid OFF header format\n");
            exit (1);
        }

        vertices.reserve (vnum);
        normals.reserve (vnum);
        triangles.reserve (fnum);

        // Read vertices, initialize normals
        for (unsigned int i = 0; i < vnum; ++i) {
            float x, y, z;
            if (!(file >> x >> y >> z)) {
                fprintf (stderr, "Error: Failed to read vertex data at index %u\n", i);
                exit (1);
            }
            vertices.emplace_back (x, y, z);
            normals.emplace_back (0.0, 0.0, 0.0);
        }

        // Read faces
        for (unsigned int i = 0; i < fnum; ++i) {
            unsigned int vcount;

            if (!(file >> vcount)) {
                fprintf (stderr, "Error: Failed to read face count at face %u\n", i);
                exit (1);
            }

            if (vcount == 3) {
                glm::uvec3 triangle;

                if (!(file >> triangle[0] >> triangle[1] >> triangle[2])) {
                    fprintf (stderr, "Error: Failed to read triangle indices at face %u\n", i);
                    exit (1);
                }
                triangles.push_back (triangle);
            }
            else {
                fprintf (stderr, "Error: Face %u is not a triangle\n", i);
                exit (1);
            }
        }

        file.close();

        rescale ();
        compute_normals ();
        compute_mm();

        material_diffuse_loc = glGetUniformLocation (shaders.program, "material.diffuse");
        material_ambient_loc = glGetUniformLocation (shaders.program, "material.ambient");
        material_specular_loc = glGetUniformLocation (shaders.program, "material.specular");
        material_shininess_loc = glGetUniformLocation (shaders.program, "material.shininess");

        update_materials();

        pack4gpu();
        send_arrays_2a3f();
    }

    void update_materials()
    {
        glUniform3fv (material_diffuse_loc, 1, &material_diffuse[0]);
        glUniform3fv (material_ambient_loc, 1, &material_ambient[0]);
        glUniform3fv (material_specular_loc, 1, &material_specular[0]);
        glUniform1fv (material_shininess_loc, 1, &material_shininess);
    }

    void draw_obj(GLint model_loc, GLint normal_matrix_loc) override
    {
        update_materials();
        Object::draw_obj(model_loc, normal_matrix_loc);
    }

private:

    void rescale ()
    {
        if (vertices.empty ())
            return;

        // Find bounding box
        glm::vec3 min_bounds = vertices[0];
        glm::vec3 max_bounds = vertices[0];

        for (const auto& vertex : vertices) {
            // glm/glsl min and max work component-wise
            min_bounds = glm::min (min_bounds, vertex);
            max_bounds = glm::max (max_bounds, vertex);
        }

        // Calculate centers and extents along x, y, and z
        origin = (min_bounds + max_bounds) * 0.5f;
        // glm::vec3 extents = (max_bounds - min_bounds) * 0.5f;

        // Find the maximum extent to preserve proportions
        //float max_extent = std::max ({extents.x, extents.y, extents.z});
        float max_extent = glm::distance (max_bounds, min_bounds) * 0.5;

        scale_factor = glm::vec3(1.f / max_extent);
    }

    void compute_normals ()
    {
        for (auto t : triangles) {
            // get the coordinates of this triangle's vertices
            glm::vec3 v0 = vertices[t[0]];
            glm::vec3 v1 = vertices[t[1]];
            glm::vec3 v2 = vertices[t[2]];
            
            // compute this triangle's normal
            glm::vec3 n = glm::cross (v1 - v0, v2 - v0);

            // accumulate the triangle normal in its vertices
            normals[t[0]] += n;
            normals[t[1]] += n;
            normals[t[2]] += n;
        }

        // normalize the normals
        for (auto& n : normals)
            n = glm::normalize (n);
        
    }

    void pack4gpu()
    {
        points = {};
        // fill up flat points
        for (size_t i = 0; i < vertices.size(); i++) {
            const auto& v = vertices[i];
            const auto& n = normals[i];

            // coords
            points.push_back (v.x);
            points.push_back (v.y);
            points.push_back (v.z);
            // normals
            points.push_back (n.x);
            points.push_back (n.y);
            points.push_back (n.z);
        }

        indices = {};

        for (auto t : triangles)
            for (unsigned i = 0; i < 3; i++)
                indices.push_back (t[i]);
    }
};
