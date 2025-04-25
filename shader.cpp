#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

typedef uint32_t Shader;

inline void use_shader(Shader shader)
{
    glUseProgram(shader);
}

inline void shader_set_bool(Shader ID, const std::string_view name, bool value)
{
    glUniform1i(glGetUniformLocation(ID, name.data()), (int)value);
}

inline void shader_set_int(Shader ID, const std::string_view name, int value)
{ 
    glUniform1i(glGetUniformLocation(ID, name.data()), value); 
}

inline void shader_set_float(Shader ID, const std::string_view name, float value)
{
    glUniform1f(glGetUniformLocation(ID, name.data()), value);
}


// Function to check shader compilation/linking errors
void check_shader_errors(GLuint shader, const char *type)
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


#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

std::string read_file(char const *fname)
{
    static const auto BUFFER_SIZE = 16*1024;
    int fd = open(fname, O_RDONLY);
    if(fd == -1)
        printf("[%s:%d] file open failed", __FILE__, __LINE__);

#ifdef __linux__
    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL
#endif

    char buf[BUFFER_SIZE + 1];
    uintmax_t lines = 0;
    size_t bytes_read = read(fd, buf, BUFFER_SIZE);
    buf[bytes_read] = 0;

    return (buf);
}

Shader create_shader_from_source(const char*vertex_shader_source, const char*fragment_shader_source)
{
    // Compile vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    check_shader_errors(vertex_shader, "VERTEX");

    // Compile fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    check_shader_errors(fragment_shader, "FRAGMENT");

    // Create shader program and link shaders
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    check_shader_errors(shader_program, "PROGRAM");
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}


Shader create_shader(const char*vertex_file, const char*fragment_file)
{
    auto vertex_source = read_file(vertex_file);
    auto fragment_source = read_file(fragment_file);
    return create_shader_from_source(vertex_source.c_str(), fragment_source.c_str());
}