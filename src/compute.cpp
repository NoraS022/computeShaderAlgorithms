#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

template<typename T = size_t>
struct size3{
  T x{0};
  T y{0};
  T z{0};
  size3(){}
  size3(T _x, T _y, T _z):x(_x), y(_y), z(_z){}
  friend std::ostream& operator<<(std::ostream& os, const size3& obj){ os << "[" << obj.x << ", " << obj.y << ", " << obj.z << "]"; return os; }
};

std::string readShaderSource(const std::string &filepath){
    std::ifstream file(filepath);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint loadShaderProgram(const std::string &shaderFilepath){
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    std::string shaderSource = readShaderSource(shaderFilepath);
    std::cout << "Shader: " << shaderSource;
    const char *shaderSourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourcePtr, nullptr);

    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        std::cerr << "Shader compilation failed with status " << compiled << std::endl;
        
        GLchar logInfoC[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, &logInfoC[0]);
        std::string logInfo(logInfoC);
        std::cout << logInfo << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    // Setup program
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glDeleteShader(shader);
    glLinkProgram(program);

    // Check linking status
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        std::cerr << "Linking failed" << std::endl;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

int main(){
  glfwSetErrorCallback([](int error, const char *description)->void{
    std::cerr << description << std::endl;
  });
  glfwInit();

  // Demand OpenGL 4.3 at minimum for compute shader compatability
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  
  // TODO, adapt windowsize to debug quad rendering
  auto window = glfwCreateWindow(512, 512, "viewer", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  
  glfwSetWindowUserPointer(window, nullptr);
  glfwSetKeyCallback(window, nullptr);
  glfwSetCharCallback(window, nullptr);
  glfwSetMouseButtonCallback(window, nullptr);
  glfwSetCursorPosCallback(window, nullptr);
  glfwSetScrollCallback(window, nullptr);
  glfwSetFramebufferSizeCallback(window, nullptr);

  // Load OpenGL functions
  if (gl3wInit() || !gl3wIsSupported(4, 3)) {
      std::cerr << "Error: failed to initialize OpenGL" << std::endl;
      std::exit(EXIT_FAILURE);
  }
  std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

  // Get compute execution parameters
  size3<GLint> workGroupCount(0,0,0);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCount.x);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCount.y);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCount.z);
  size3<GLint> workGroupSize(0,0,0);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSize.x);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSize.y);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSize.z);
  GLint workGroupInvocations{0};
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workGroupInvocations);
  std::cout << "Max amount of workgroups: " << workGroupCount << ", max size of each workgroup: " << workGroupSize << ". Maximum local work group invocations: " << workGroupInvocations << " " << std::endl;

  // Compute shader program
  std::string envVariable = "COMPUTE_ROOT";
  envVariable = std::string(std::getenv(envVariable.c_str()));
  auto program = loadShaderProgram(envVariable + "/src/compute.dispatch");

  // Setup compute texture for communicating with shader program
  int textureW = 10, textureH = 5;
  size_t textureDataSize{textureW*textureH*4};

  GLuint tex_output;
  glGenTextures(1, &tex_output);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, tex_output);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  std::vector<GLfloat> initialData(textureDataSize, 0.5f);  
  glPixelStoref(GL_PACK_ALIGNMENT, 1); // No padding for the bytes
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureW, textureH, 0, GL_RGBA, GL_FLOAT,
   &initialData[0]);
  glBindImageTexture(3, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F); // binds a single level of a texture to an image unit for the purpose of reading and writing it from shaders


  // Retrieve and print tex data
  std::vector<GLfloat> data(textureDataSize, -1.0f);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data[0]); // return a texture image into pixels
  for(auto e : data){
      std::cout << e << ", ";
  }


  // Main loop
  while(!glfwWindowShouldClose(window)) { 
    // Launch compute, input: texture image
    glUseProgram(program);
    glDispatchCompute(textureW, textureH, 1); // REQUIRES OpenGL 4.3 
    
    // Sync on image access
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Retrieve output
    std::vector<GLfloat> dataLoop(textureDataSize, -1.0f);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &dataLoop[0]);
    for(auto e : dataLoop){
      std::cout << e << ", ";
    }


    /* TODO, implement debug drawing pass (program with quad vertex shader and texture sampler fragment shader) 
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(quad_program);
    glBindVertexArray(quad_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    */


    std::cout << std::endl << "GLerror: " << glGetError() << std::endl;
    glfwPollEvents();
    if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
      glfwSetWindowShouldClose(window, 1);
    }
    glfwSwapBuffers(window);
  }

  return 0;
}
