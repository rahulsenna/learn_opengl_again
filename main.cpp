#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

// Vertex shader source code
const char* vertexShaderSource = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vertexColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
#version 410 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

// Fragment shader source code
const char* secondFragmentShaderSource = R"(
    #version 410 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1,1,0, 1.0);
    }
    )";


// Error callback for GLFW
void errorCallback(int error, const char *description)
{
    std::cerr << "GLFW Error: " << description << std::endl;
}

// Function to check shader compilation/linking errors
void checkShaderErrors(GLuint shader, const char *type)
{
    GLint success;
    GLchar infoLog[1024];
    
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
        }
    }
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

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkShaderErrors(vertexShader, "VERTEX");

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderErrors(fragmentShader, "FRAGMENT");

    // Create shader program and link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShaderErrors(shaderProgram, "PROGRAM");

    // Compile 2nd fragment shader
    GLuint secondFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(secondFragmentShader, 1, &secondFragmentShaderSource, NULL);
    glCompileShader(secondFragmentShader);
    checkShaderErrors(secondFragmentShader, "FRAGMENT 2");

    // Create 2nd shader program and link shaders
    GLuint secondShaderProgram = glCreateProgram();
    glAttachShader(secondShaderProgram, vertexShader);
    glAttachShader(secondShaderProgram, secondFragmentShader);
    glLinkProgram(secondShaderProgram);
    checkShaderErrors(secondShaderProgram, "PROGRAM 2");


    // Delete shaders as they're linked into the program now and no longer needed
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(secondFragmentShader);

    // Vertex data for the triangle (position and color)

    float vertices[] = {
         0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f,   // top right
         0.5f, -0.3f,  0.0f,  0.0f,  1.0f, 0.0f,   // bottom right
        -0.5f, -0.3f,  0.0f,  0.0f,  0.0f, 1.0f,   // bottom left
        -0.5f,  0.5f,  0.0f,  .5f,    .5f, 0.0f,   // top left
        
      
        -0.7f,  -0.6f,   0.0f,  1.0f,  0.0f, 0.0f,   // bottom right
        -0.45f, -0.99f,  0.0f,  0.0f,  1.0f, 0.0f,   // bottom right
        -0.95f, -0.99f,  0.0f,  0.0f,  0.0f, 1.0f,   // bottom left

         0.7f,  -0.6f,   0.0f,  0.0f,  0.0f, 1.0f,   // bottom right
         0.45f, -0.99f,  0.0f,  0.0f,  1.0f, 0.0f,   // bottom right
         0.95f, -0.99f,  0.0f,  1.0f,  0.0f, 0.0f,   // bottom left

    };
    unsigned int indices[] = {

        0, 1, 3, // first triangle
        1, 2, 3, // second triangle
        

        4,5,6,7,8,9
    };

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Create Vertex Array Object
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Create Vertex Buffer Object
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind VAO
    glBindVertexArray(0);

    //----[ 2nd VAO ]------------------------------------------------------------------------------------------------------
    float another_triangle[] = {
            
       -0.7f,   0.55f,   0.0f,  1.0f,  0.0f, 0.0f,   // bottom
       -0.45f,  0.99f,   0.0f,  0.0f,  1.0f, 0.0f,   // top right
       -0.95f,  0.99f,   0.0f,  0.0f,  0.0f, 1.0f,   // top left

   };
    // Create 2nd Vertex Array Object
    GLuint VAO2;
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);

    // Create Vertex Buffer Object
    GLuint VBO2;
    glGenBuffers(1, &VBO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(another_triangle), another_triangle, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Unbind VAO
    glBindVertexArray(0);
    //----[ 2nd VAO ]------------------------------------------------------------------------------------------------------

    // Main rendering loop
    float r = 0.2f, g = 0.3f, b = 0.3f, a = 1.0f;

    GameCode game_code = {};
    load_game_code(&game_code);
    
    // Triangle movement variables
    float dx = 0.0f, dy = -0.005f;
    int direction = 0; // 0: down, 1: right, 2: up, 3: left

    // Calculate original shape dimensions
    float width = fabs(another_triangle[6] - another_triangle[12])/2.f; // ~0.5
    float height = fabs(another_triangle[1] - another_triangle[7])/2.f; // ~0.44

    auto patrol_triangle = [&]() -> void
    {
        // Update all vertices
        for (int i = 0; i < 3; i++)
        {
            another_triangle[i*6] += dx;
            another_triangle[i*6 + 1] += dy;
        }
        
        // Calculate center point for rotation
        float cx = (another_triangle[0] + another_triangle[6] + another_triangle[12]) / 3.0f;
        float cy = (another_triangle[1] + another_triangle[7] + another_triangle[13]) / 3.0f;
        
        // Check boundaries and change direction
        if (cy <= -0.7f && direction == 0)
        {
            // Bottom edge - turn right
            direction = 1;
            dy = 0.0f;
            dx = 0.005f;
            
            // Reposition to point right while preserving shape
            another_triangle[0] = cx - height;    // left
            another_triangle[1] = cy - width;
            another_triangle[6] = cx + height;    // right (point)
            another_triangle[7] = cy;
            another_triangle[12] = cx - height;   // left
            another_triangle[13] = cy + width;
        }
        else if (cx >= 0.7f && direction == 1)
        {
            // Right edge - turn up
            direction = 2;
            dx = 0.0f;
            dy = 0.005f;
            
            // Reposition to point up while preserving shape
            another_triangle[0] = cx - width;     // bottom right
            another_triangle[1] = cy - height;
            another_triangle[6] = cx;               // top (point)
            another_triangle[7] = cy + height;
            another_triangle[12] = cx + width;    // bottom left
            another_triangle[13] = cy - height;
        }
        else if (cy >= 0.7f && direction == 2)
        {
            // Top edge - turn left
            direction = 3;
            dy = 0.0f;
            dx = -0.005f;
            
            // Reposition to point left while preserving shape
            another_triangle[0] = cx + height;    // right
            another_triangle[1] = cy + width;
            another_triangle[6] = cx - height;    // left (point)
            another_triangle[7] = cy;
            another_triangle[12] = cx + height;   // right
            another_triangle[13] = cy - width;
        }
        else if (cx <= -0.7f && direction == 3)
        {
            // Left edge - turn down
            direction = 0;
            dx = 0.0f;
            dy = -0.005f;
            
            // Reposition to point down while preserving shape
            another_triangle[0] = cx + width;     // top right
            another_triangle[1] = cy + height;
            another_triangle[6] = cx;               // bottom (point)
            another_triangle[7] = cy - height;
            another_triangle[12] = cx - width;    // top left
            another_triangle[13] = cy + height;
        }

        //----[ 2nd VAO ]------------------------------------------------------------------------------------------------------
        glBindVertexArray(0);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        //----[ 2nd VAO ]------------------------------------------------------------------------------------------------------
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

        // Draw triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glUseProgram(secondShaderProgram);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(6 * sizeof(unsigned int)));
        // Unbind VAO
        glBindVertexArray(0);

        
        patrol_triangle();


        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(another_triangle), another_triangle, GL_DYNAMIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        
        // Unbind VAO
        glBindVertexArray(0);


        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    return 0;
}