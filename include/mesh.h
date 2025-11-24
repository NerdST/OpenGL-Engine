#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <shader.h>
#include <texture.h>
#include <vector>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoords;
  glm::vec3 tangent;
  glm::vec3 bitangent;
  int m_BoneIDs[MAX_BONE_INFLUENCE];
  float m_Weights[MAX_BONE_INFLUENCE];
};

class Mesh
{
public:
  // mesh data
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<Texture> textures;

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
      : vertices(vertices), indices(indices), textures(textures)
  {
    // Ensure textures are loaded
    for (auto &texture : textures)
    {
      std::cout << "Loaded texture: " << texture.path << " of type: " << texture.type << std::endl;
    }
    processTextures();
    setupMesh();
  }

  void Draw(Shader &shader) const
  {
    // Track presence flags - these must still be calculated here
    bool hasDiffuse = false, hasSpecular = false,
         hasNormal = false, hasMetallic = false,
         hasRoughness = false, hasAO = false,
         hasHeight = false, hasEmissive = false;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
      glActiveTexture(GL_TEXTURE0 + i);

      // The logic for name, number, and concatenation is gone!
      shader.setInt(textures[i].uniformName.c_str(), i);

      // Update the presence flags based on the texture type
      const std::string &type = textures[i].type;

      if (type == "texture_diffuse")
        hasDiffuse = true;
      else if (type == "texture_specular")
        hasSpecular = true;
      else if (type == "texture_normal")
        hasNormal = true;
      else if (type == "texture_metallic")
        hasMetallic = true;
      else if (type == "texture_roughness")
        hasRoughness = true;
      else if (type == "texture_ao")
        hasAO = true;
      else if (type == "texture_height")
        hasHeight = true;
      else if (type == "texture_emissive")
        hasEmissive = true;

      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // Provide presence flags the shaders can use
    shader.setBool("hasDiffuse", hasDiffuse);
    shader.setBool("hasSpecular", hasSpecular);
    shader.setBool("hasNormal", hasNormal);
    shader.setBool("hasMetallic", hasMetallic);
    shader.setBool("hasRoughness", hasRoughness);
    shader.setBool("hasAO", hasAO);
    shader.setBool("hasHeight", hasHeight);
    shader.setBool("hasEmissive", hasEmissive);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0);
  }

  void DrawInstanced(Shader &shader, unsigned int instanceCount)
  {
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    for (unsigned int i = 0; i < textures.size(); i++)
    {
      glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
      // retrieve texture number (the N in diffuse_textureN)
      std::string number;
      std::string name = textures[i].type;
      if (name == "texture_diffuse")
        number = std::to_string(diffuseNr++);
      else if (name == "texture_specular")
        number = std::to_string(specularNr++);
      else if (name == "texture_normal")
        number = std::to_string(normalNr++);
      else if (name == "texture_height")
        number = std::to_string(heightNr++);

      shader.setInt(("material." + name + number).c_str(), i);
      glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }

    // draw mesh
    glBindVertexArray(VAO);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
    // glBindVertexArray(0);
  }

private:
protected:
  //  render data
  unsigned int VAO, VBO, EBO;
  void processTextures()
  {
    // Reset counters for uniform numbering
    unsigned int diffuseNr = 1, specularNr = 1, normalNr = 1, heightNr = 1;
    unsigned int metallicNr = 1, roughnessNr = 1, aoNr = 1, emissiveNr = 1;

    for (auto &texture : textures)
    {
      std::string number;
      const std::string &name = texture.type;

      // Use a pointer to the correct counter for DRY principle
      unsigned int *counter = nullptr;

      if (name == "texture_diffuse")
        counter = &diffuseNr;
      else if (name == "texture_specular")
        counter = &specularNr;
      else if (name == "texture_normal")
        counter = &normalNr;
      else if (name == "texture_metallic")
        counter = &metallicNr;
      else if (name == "texture_roughness")
        counter = &roughnessNr;
      else if (name == "texture_ao")
        counter = &aoNr;
      else if (name == "texture_height")
        counter = &heightNr;
      else if (name == "texture_emissive")
        counter = &emissiveNr;

      // If a known type, generate the name and store it
      if (counter)
      {
        number = std::to_string((*counter)++);
        // Store the final calculated string
        texture.uniformName = name + number;
      }
      // Handle unknown texture types if necessary
      else
      {
        std::cerr << "Warning: Unknown texture type encountered: " << name << std::endl;
      }
    }
  }
  void setupMesh()
  {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texCoords));
    // vertex tangents
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
    // vertex bitangents
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, bitangent));
    // bone IDs
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, m_BoneIDs));
    // bone weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, m_Weights));

    glBindVertexArray(0);
  }
};

#endif // MESH_H