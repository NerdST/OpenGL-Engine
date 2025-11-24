#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <filesystem>

class Shader
{
public:
  unsigned int ID;
  const char *vertexPath;
  const char *fragmentPath;
  const char *name;

  static std::map<std::string, Shader *> shaders;
  mutable std::unordered_map<std::string, GLint> uniformLocationCache;

  char vtext[4096], ftext[8192];

  // constructor generates the shader on the fly
  // ------------------------------------------------------------------------
  Shader(const char *vertexPath, const char *fragmentPath, const char *tName = "default")
  {
    this->vertexPath = vertexPath;
    this->fragmentPath = fragmentPath;
    this->name = tName;
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
      // open files
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vShaderStream, fShaderStream;
      // read file's buffer contents into streams
      vShaderStream << vShaderFile.rdbuf();
      fShaderStream << fShaderFile.rdbuf();
      // close file handlers
      vShaderFile.close();
      fShaderFile.close();
      // convert stream into string
      vertexCode = vShaderStream.str();
      fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure &e)
    {
      std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }

    strncpy(vtext, vertexCode.c_str(), sizeof(vtext) - 1);
    vtext[sizeof(vtext) - 1] = '\0';
    strncpy(ftext, fragmentCode.c_str(), sizeof(ftext) - 1);
    ftext[sizeof(ftext) - 1] = '\0';

    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    // Add this shader to the static map
    shaders[std::string(tName)] = this;
  }
  // activate the shader
  // ------------------------------------------------------------------------
  void use() const
  {
    glUseProgram(ID);
  }
  void reload(const char *vShaderCode, const char *fShaderCode)
  {
    // compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");
    // shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  void reload()
  {
    reload(vtext, ftext);
  }
  // utility uniform functions
  // ------------------------------------------------------------------------
  void setBool(const std::string &name, bool value) const
  {
    glUniform1i(getUniformLocation(name), (int)value);
  }
  // ------------------------------------------------------------------------
  void setInt(const std::string &name, int value) const
  {
    glUniform1i(getUniformLocation(name), value);
  }
  // ------------------------------------------------------------------------
  void setFloat(const std::string &name, float value) const
  {
    glUniform1f(getUniformLocation(name), value);
  }
  // ------------------------------------------------------------------------
  void setVec2(const std::string &name, const glm::vec2 &value) const
  {
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
  }
  void setVec2(const std::string &name, float x, float y) const
  {
    glUniform2f(getUniformLocation(name), x, y);
  }
  // ------------------------------------------------------------------------
  void setVec3(const std::string &name, const glm::vec3 &value) const
  {
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
  }
  void setVec3(const std::string &name, float x, float y, float z) const
  {
    glUniform3f(getUniformLocation(name), x, y, z);
  }
  // ------------------------------------------------------------------------
  void setVec4(const std::string &name, const glm::vec4 &value) const
  {
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
  }
  void setVec4(const std::string &name, float x, float y, float z, float w) const
  {
    glUniform4f(getUniformLocation(name), x, y, z, w);
  }
  // ------------------------------------------------------------------------
  void setMat2(const std::string &name, const glm::mat2 &mat) const
  {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
  }
  // ------------------------------------------------------------------------
  void setMat3(const std::string &name, const glm::mat3 &mat) const
  {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
  }
  // ------------------------------------------------------------------------
  void setMat4(const std::string &name, const glm::mat4 &mat) const
  {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
  }
  void saveShaders()
  {
    std::ofstream myfile;

    myfile.open(vertexPath);
    myfile << vtext;
    myfile.close();

    myfile.open(fragmentPath);
    myfile << ftext;
    myfile.close();
  }

private:
  // utility function for checking shader compilation/linking errors.
  // ------------------------------------------------------------------------
  void checkCompileErrors(GLuint shader, std::string type)
  {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        glGetShaderInfoLog(shader, 1024, NULL, infoLog);
        std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
      }
    }
    else
    {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success)
      {
        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
        std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                  << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
      }
    }
  }
  // Get uniform location with caching
  GLint getUniformLocation(const std::string &name) const
  {
    if (uniformLocationCache.find(name) != uniformLocationCache.end())
      return uniformLocationCache[name];

    GLint location = glGetUniformLocation(ID, name.c_str());
    uniformLocationCache[name] = location;
    return location;
  }
};

#endif