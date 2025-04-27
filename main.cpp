#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <vector>

#include "shader.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Error callback for GLFW
void errorCallback(int error, const char *description)
{
    std::cerr << "GLFW Error: " << description << std::endl;
}


//--------[ DLL ]--------------------------------------------
#include <dlfcn.h>
#include <sys/stat.h>
#include <string>

typedef struct
{
    void* game_code_handle;
    time_t dll_last_write_time;
    void (*clear_color)(float *, float *, float *, float *);
} GameCode;

time_t get_last_write_time(const char *filename)
{
    time_t last_write_time = 0;
    struct stat stat_data = {};
    if (stat(filename, &stat_data) == 0)
    {
        last_write_time = stat_data.st_mtime;
    }
    return (last_write_time);
}

void load_game_code(GameCode *game_code)
{
    void *lib_handle = dlopen("libgame.dylib", RTLD_NOW);
    if (!lib_handle)
    {
        printf("[%s:%d] Unable to load library: %s\n", __FILE__,__LINE__, dlerror());
        exit(EXIT_FAILURE);
    }
    game_code->game_code_handle = lib_handle;
    game_code->clear_color = (void (*)(float*, float*, float*, float*))dlsym(lib_handle, "clear_color");
    if (!game_code->clear_color)
    {
        printf("[%s:%d] Unable to get symbol: %s\n", __FILE__,__LINE__, dlerror());
        exit(EXIT_FAILURE);
    }
    game_code->dll_last_write_time = get_last_write_time("libgame.dylib");
}

struct Quad
{    
    Shader shader;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> textures;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    bool dynamic = false;

    void upload_vertices()
    {
        // Create Vertex Array Object
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Create Vertex Buffer Object
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertices.size() * sizeof(vertices.front()), vertices.data(), dynamic?GL_DYNAMIC_DRAW:GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indices.size() * sizeof(indices.front()), indices.data(), dynamic?GL_DYNAMIC_DRAW:GL_STATIC_DRAW);

        // Set vertex attribute pointers
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // UV attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // Unbind VAO
        glBindVertexArray(0);
    }
    void upload_texture(const char *texture_file)
    {    
        unsigned int texture_id;
        stbi_set_flip_vertically_on_load(true);
        int img_width, img_height, img_nr_channels;
        uint8_t *data = stbi_load(texture_file, &img_width, &img_height, &img_nr_channels, 0);
        
        if (!data) {
            std::cerr << "Failed to load texture: " << texture_file << std::endl;
            return;
        }
    
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
    
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Choose the correct format based on channels
        GLint format;
        if (img_nr_channels == 1)
            format = GL_RED;
        else if (img_nr_channels == 3)
            format = GL_RGB;
        else if (img_nr_channels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported image format" << std::endl;
            stbi_image_free(data);
            return;
        }
    
        glTexImage2D(GL_TEXTURE_2D, 0, format, img_width, img_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    
        stbi_image_free(data);
    
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Get the texture unit index
        auto texture_unit = textures.size();
        
        // Activate the correct texture unit
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        std::string texture_name = "texture_" + std::to_string(texture_unit);
        shader.set_int(texture_name, (int)texture_unit);
        textures.push_back(texture_id);
    }
 
    void draw()
    {
        shader.use();
        glBindVertexArray(VAO);
        if (dynamic)
        { 
            glBindVertexArray(VBO);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vertices.size() * sizeof(vertices.front()), vertices.data(), GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
 
        unsigned int tex_pos = GL_TEXTURE0;
        for (auto texture: textures)
        {
            glActiveTexture(tex_pos++);
            glBindTexture(GL_TEXTURE_2D, texture);
        }       
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // Unbind VAO
        glBindVertexArray(0);
 
    }
};

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set error callback
    glfwSetErrorCallback(errorCallback);

    // Configure GLFW to use OpenGL 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a window
    GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL 4.1 Colored Triangle", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Verify OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;


    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Quad flag = {.shader = Shader("../res/shaders/text1.vert", "../res/shaders/multi_text_quad1.frag"),
                 .vertices = {
                     /* pos */  0.5f,  0.5f, 0.0f, /* color */ 1.0f, 0.0f, 0.0f, /* uv */ 1, 1,   // top right
                     /* pos */  0.5f, -0.3f, 0.0f, /* color */ 1.0f, 1.0f, 0.0f, /* uv */ 1, 0,  // bottom right
                     /* pos */ -0.5f, -0.3f, 0.0f, /* color */ 0.0f, 0.0f, 1.0f, /* uv */ 0, 0, // bottom left
                     /* pos */ -0.5f,  0.5f, 0.0f, /* color */ 0.0f, 1.0f, 1.0f, /* uv */ 0, 1,  // top left
                 },
                 .indices = {
                     0, 1, 3, // first triangle
                     1, 2, 3, // second triangle
                 }};
    flag.upload_vertices();
    flag.upload_texture("../res/textures/us.png");



    float plane_x = -1.;
    float plane_y = .7;
    Quad plane = {.shader = Shader("../res/shaders/text1.vert", "../res/shaders/text1.frag"),
                  .vertices = {
                      /* pos */ plane_x + 0.3f, plane_y + 0.3f, 0.0f, /* color */ 1.0f, 0.0f, 0.0f, /* uv */ 0, 0, // top right
                      /* pos */ plane_x + 0.3f, plane_y + 0.0f, 0.0f, /* color */ 0.0f, 1.0f, 0.0f, /* uv */ 0, 1, // bottom right
                      /* pos */ plane_x + 0.0f, plane_y + 0.0f, 0.0f, /* color */ 0.0f, 0.0f, 1.0f, /* uv */ 1, 1, // bottom left
                      /* pos */ plane_x + 0.0f, plane_y + 0.3f, 0.0f, /* color */ 0.5f, 0.5f, 0.0f, /* uv */ 1, 0, // top left
                  },
                  .indices = {
                      0, 1, 3, // first triangle
                      1, 2, 3, // second triangle
                  },
                  .dynamic = true
                };
    plane.upload_vertices();
    plane.upload_texture("../res/textures/chat_gpt_plane.png");

    // Main rendering loop
    float r = 0.2f, g = 0.3f, b = 0.3f, a = 1.0f;

    GameCode game_code = {};
    load_game_code(&game_code);

    // Plane movement variables
    float dx = 0.0f, dy = -0.005f;
    int direction = 0; // 0: down, 1: right, 2: up, 3: left

    auto patrol_plane = [&]() -> void
    {
        // Update all vertices
        for (int i = 0; i < 4; i++)
        {
            plane.vertices[i * 8] += dx;
            plane.vertices[i * 8 + 1] += dy;
        }

        // Calculate center point for rotation
        float cx = (plane.vertices[0] + plane.vertices[8] + plane.vertices[16]) / 3.0f;
        float cy = (plane.vertices[1] + plane.vertices[9] + plane.vertices[17]) / 3.0f;

        // Check boundaries and change direction
        // swap UVs for turning
        if (cy <= -0.9f && direction == 0)
        {
            // Bottom edge - turn right
            direction = 1;
            dy = 0.0f;
            dx = 0.005f;

            plane.vertices[8 * 0 + 6 + 0] = 0;
            plane.vertices[8 * 0 + 6 + 1] = 1;
            plane.vertices[8 * 1 + 6 + 0] = 1;
            plane.vertices[8 * 1 + 6 + 1] = 1;
            plane.vertices[8 * 2 + 6 + 0] = 1;
            plane.vertices[8 * 2 + 6 + 1] = 0;
            plane.vertices[8 * 3 + 6 + 0] = 0;
            plane.vertices[8 * 3 + 6 + 1] = 0;
        }
        else if (cx >= .9f && direction == 1)
        {
            // Right edge - turn up
            direction = 2;
            dx = 0.0f;
            dy = 0.005f;

            plane.vertices[8 * 0 + 6 + 0] = 0;
            plane.vertices[8 * 0 + 6 + 1] = 1;
            plane.vertices[8 * 1 + 6 + 0] = 0;
            plane.vertices[8 * 1 + 6 + 1] = 0;
            plane.vertices[8 * 2 + 6 + 0] = 1;
            plane.vertices[8 * 2 + 6 + 1] = 0;
            plane.vertices[8 * 3 + 6 + 0] = 1;
            plane.vertices[8 * 3 + 6 + 1] = 1;
        }
        else if (cy >= .8 && direction == 2)
        {
            // Top edge - turn left
            direction = 3;
            dy = 0.0f;
            dx = -0.005f;

            plane.vertices[8 * 0 + 6 + 0] = 1;
            plane.vertices[8 * 0 + 6 + 1] = 0;
            plane.vertices[8 * 1 + 6 + 0] = 0;
            plane.vertices[8 * 1 + 6 + 1] = 0;
            plane.vertices[8 * 2 + 6 + 0] = 0;
            plane.vertices[8 * 2 + 6 + 1] = 1;
            plane.vertices[8 * 3 + 6 + 0] = 1;
            plane.vertices[8 * 3 + 6 + 1] = 1;
        }
        else if (cx <= -0.8f && direction == 3)
        {
            // Left edge - turn down
            direction = 0;
            dx = 0.0f;
            dy = -0.005f;

            plane.vertices[8 * 0 + 6 + 0] = 0;
            plane.vertices[8 * 0 + 6 + 1] = 0;
            plane.vertices[8 * 1 + 6 + 0] = 0;
            plane.vertices[8 * 1 + 6 + 1] = 1;
            plane.vertices[8 * 2 + 6 + 0] = 1;
            plane.vertices[8 * 2 + 6 + 1] = 1;
            plane.vertices[8 * 3 + 6 + 0] = 1;
            plane.vertices[8 * 3 + 6 + 1] = 0;
        }
    };

    while (!glfwWindowShouldClose(window))
    {

        auto dll_write_time = get_last_write_time("libgame.dylib");
        if (dll_write_time > game_code.dll_last_write_time)
        {
            dlclose(game_code.game_code_handle);
            load_game_code(&game_code);
        }
        game_code.clear_color(&r, &g, &b, &a);

        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        // Rendering
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        flag.draw();
        patrol_plane();
        plane.draw();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}