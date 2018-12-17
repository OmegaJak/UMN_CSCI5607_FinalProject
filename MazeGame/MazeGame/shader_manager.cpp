#include <fstream>
#include <sstream>
#include "constants.h"
#include "shader_manager.h"

int ShaderManager::InitShaders() {
    Textured_Shader = CompileShaderProgram("textured-Vertex.glsl", "textured-Fragment.glsl");
    CompanionWindow_Shader = CompileShaderProgram("companionWindow-Vertex.glsl", "companionWindow-Fragment.glsl");
    RenderModel_Shader = CompileShaderProgram("renderModel-Vertex.glsl", "renderModel-Fragment.glsl");

    InitShaderAttributes();

    return Textured_Shader;
}

void ShaderManager::Cleanup() {
    glDeleteProgram(Textured_Shader);
}

// Tell OpenGL how to set fragment shader input
void ShaderManager::InitShaderAttributes() {
    GLint posAttrib = glGetAttribLocation(Textured_Shader, "position");
    glVertexAttribPointer(posAttrib, VALUES_PER_POSITION, GL_FLOAT, GL_FALSE, ATTRIBUTE_STRIDE * sizeof(float), POSITION_OFFSET);
    // Attribute, vals/attrib., type, isNormalized, stride, offset
    glEnableVertexAttribArray(posAttrib);

    GLint normAttrib = glGetAttribLocation(Textured_Shader, "inNormal");
    glVertexAttribPointer(normAttrib, VALUES_PER_NORMAL, GL_FLOAT, GL_FALSE, ATTRIBUTE_STRIDE * sizeof(float),
                          (void*)(NORMAL_OFFSET * sizeof(float)));
    glEnableVertexAttribArray(normAttrib);

    GLint texAttrib = glGetAttribLocation(Textured_Shader, "inTexcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, VALUES_PER_TEXCOORD, GL_FLOAT, GL_FALSE, ATTRIBUTE_STRIDE * sizeof(float),
                          (void*)(TEXCOORD_OFFSET * sizeof(float)));

    GLint uniColor = glGetUniformLocation(Textured_Shader, "inColor");
    GLint uniTexID = glGetUniformLocation(Textured_Shader, "texID");
    GLint uniView = glGetUniformLocation(Textured_Shader, "view");
    GLint uniProj = glGetUniformLocation(Textured_Shader, "proj");
    GLint uniModel = glGetUniformLocation(Textured_Shader, "model");
    GLint uniShaderMode = glGetUniformLocation(Textured_Shader, "shaderMode");

    glUseProgram(Textured_Shader);
    glUniform1i(uniShaderMode, 0);
    glUseProgram(0);

    Attributes.position = posAttrib;
    Attributes.normals = normAttrib;
    Attributes.texCoord = texAttrib;
    Attributes.color = uniColor;
    Attributes.texID = uniTexID;
    Attributes.view = uniView;
    Attributes.projection = uniProj;
    Attributes.model = uniModel;
    Attributes.shaderMode = uniShaderMode;
}

GLuint ShaderManager::CompileShaderProgram(const std::string& vertex_shader_file, const std::string& fragment_shader_file) {
    GLuint vertex_shader, fragment_shader;
    GLchar *vs_text, *fs_text;
    GLuint program;

    // check GLSL version
    printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Create shader handlers
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    // Read source code from shader files
    vs_text = ReadShaderSource(vertex_shader_file.c_str());
    fs_text = ReadShaderSource(fragment_shader_file.c_str());

    // error check
    if (vs_text == NULL) {
        printf("Failed to read from vertex shader file %s\n", vertex_shader_file.c_str());
        exit(1);
    } else if (DEBUG_ON) {
        printf("Vertex Shader:\n=====================\n");
        printf("%s\n", vs_text);
        printf("=====================\n\n");
    }
    if (fs_text == NULL) {
        printf("Failed to read from fragment shader file %s\n", fragment_shader_file.c_str());
        exit(1);
    } else if (DEBUG_ON) {
        printf("\nFragment Shader:\n=====================\n");
        printf("%s\n", fs_text);
        printf("=====================\n\n");
    }

    // Load Vertex Shader
    const char* vv = vs_text;
    glShaderSource(vertex_shader, 1, &vv, NULL);  // Read source
    glCompileShader(vertex_shader);               // Compile shaders
    VerifyShaderCompiled(vertex_shader);          // Check for errors

    // Load Fragment Shader
    const char* ff = fs_text;
    glShaderSource(fragment_shader, 1, &ff, NULL);
    glCompileShader(fragment_shader);
    VerifyShaderCompiled(fragment_shader);

    // Create the program
    program = glCreateProgram();

    // Attach shaders to program
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    // Link and set program to use
    glLinkProgram(program);

    return program;
}

// Create a NULL-terminated string by reading the provided file
char* ShaderManager::ReadShaderSource(const char* shaderFile) {
    std::ifstream t(shaderFile);
    std::stringstream buffer;
    buffer << t.rdbuf();

    return _strdup(buffer.str().c_str());
}

void ShaderManager::VerifyShaderCompiled(GLuint shader) {
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printf("Vertex shader failed to compile:\n");
        if (DEBUG_ON) {
            GLint logMaxSize, logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logMaxSize);
            printf("printing error message of %d bytes\n", logMaxSize);
            char* logMsg = new char[logMaxSize];
            glGetShaderInfoLog(shader, logMaxSize, &logLength, logMsg);
            printf("%d bytes retrieved\n", logLength);
            printf("error message: %s\n", logMsg);
            delete[] logMsg;
        }
        exit(1);
    }
}

GLuint ShaderManager::Textured_Shader;
GLuint ShaderManager::CompanionWindow_Shader;
GLuint ShaderManager::RenderModel_Shader;

ShaderAttributes ShaderManager::Attributes;
