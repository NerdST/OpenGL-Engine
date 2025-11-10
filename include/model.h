#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
  // model data
  vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
  vector<Mesh> meshes;
  string directory;
  bool gammaCorrection;

  // constructor, expects a filepath to a 3D model.
  Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
  {
    loadModel(path);
  }

  void setDefaultTexture(const string &texturePath)
  {
    std::cout << "Setting default texture: " << texturePath << std::endl;
    unsigned int textureID = TextureFromFile(texturePath.c_str(), "", false);

    for (auto &mesh : meshes)
    {
      // Replace ALL textures, not just when empty
      mesh.textures.clear(); // Clear existing (potentially invalid) textures

      std::cout << "Adding default textures to mesh" << std::endl;

      // Add diffuse texture
      Texture diffuseTexture;
      diffuseTexture.id = textureID;
      diffuseTexture.type = "texture_diffuse";
      diffuseTexture.path = texturePath;
      mesh.textures.push_back(diffuseTexture);

      // Add specular texture (same texture for now)
      Texture specularTexture;
      specularTexture.id = textureID;
      specularTexture.type = "texture_specular";
      specularTexture.path = texturePath;
      mesh.textures.push_back(specularTexture);
    }
  }

  // draws the model, and thus all its meshes
  void Draw(Shader &shader)
  {
    for (unsigned int i = 0; i < meshes.size(); i++)
      meshes[i].Draw(shader);
  }

private:
  // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
  void loadModel(string const &path)
  {
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
      cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
      return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
  }

  // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
  void processNode(aiNode *node, const aiScene *scene)
  {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
      // the node object only contains indices to index the actual objects in the scene.
      // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
      processNode(node->mChildren[i], scene);
    }
  }

  Mesh processMesh(aiMesh *mesh, const aiScene *scene)
  {
    // data to fill
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex;
      // Initialize all fields to prevent garbage data
      vertex.position = glm::vec3(0.0f);
      vertex.normal = glm::vec3(0.0f);
      vertex.texCoords = glm::vec2(0.0f);
      vertex.tangent = glm::vec3(0.0f);
      vertex.bitangent = glm::vec3(0.0f);
      // Initialize bone data
      for (int j = 0; j < MAX_BONE_INFLUENCE; j++)
      {
        vertex.m_BoneIDs[j] = -1;
        vertex.m_Weights[j] = 0.0f;
      }

      glm::vec3 vector;
      // positions
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.position = vector;

      // normals
      if (mesh->HasNormals())
      {
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.normal = vector;
      }

      // texture coordinates
      if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
      {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.texCoords = vec;

        // Only set tangents/bitangents if they exist
        if (mesh->mTangents)
        {
          vector.x = mesh->mTangents[i].x;
          vector.y = mesh->mTangents[i].y;
          vector.z = mesh->mTangents[i].z;
          vertex.tangent = vector;
        }
        else
        {
          vertex.tangent = glm::vec3(0.0f);
        }

        if (mesh->mBitangents)
        {
          vector.x = mesh->mBitangents[i].x;
          vector.y = mesh->mBitangents[i].y;
          vector.z = mesh->mBitangents[i].z;
          vertex.bitangent = vector;
        }
        else
        {
          vertex.bitangent = glm::vec3(0.0f);
        }
      }
      else
        vertex.texCoords = glm::vec2(0.0f, 0.0f);

      vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];
      // retrieve all indices of the face and store them in the indices vector
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // Add PBR maps (if authoring tool exported them)
    // albedo maps
    vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic");
    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

    // roughness maps
    vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness");
    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

    // ambient occlusion maps
    vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao");
    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

    // emissive maps
    vector<Texture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive");
    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

    // Fallback texture insertion (load once)
    static unsigned int fallbackID = 0;
    if (fallbackID == 0)
    {
      fallbackID = TextureFromFile("data/textures/FALLBACK.png", "", false);
    }
    auto ensureType = [&](const char *typeName)
    {
      bool found = false;
      for (auto &t : textures)
        if (t.type == typeName)
        {
          found = true;
          break;
        }
      if (!found)
      {
        Texture ft;
        ft.id = fallbackID;
        ft.type = typeName;
        ft.path = "data/textures/FALLBACK.png";
        textures.push_back(ft);
      }
    };
    ensureType("texture_diffuse");
    ensureType("texture_specular");
    ensureType("texture_metallic");
    ensureType("texture_roughness");
    ensureType("texture_ao");
    ensureType("texture_emissive");

    return Mesh(vertices, indices, textures);
  }

  // checks all material textures of a given type and loads the textures if they're not loaded yet.
  // the required info is returned as a Texture struct.
  vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
  {
    vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
      aiString str;
      mat->GetTexture(type, i, &str);
      // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
      bool skip = false;
      for (unsigned int j = 0; j < textures_loaded.size(); j++)
      {
        if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
        {
          textures.push_back(textures_loaded[j]);
          skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
          break;
        }
      }
      if (!skip)
      { // if texture hasn't been loaded already, load it
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), this->directory);
        texture.type = typeName;
        texture.path = str.C_Str();
        textures.push_back(texture);
        textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
      }
    }
    return textures;
  }
};

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
  string filename = string(path);

  // Only add directory if it's not empty AND the path is not already absolute
  if (!directory.empty() && filename[0] != '/')
  {
    filename = directory + '/' + filename;
  }

  std::cout << "Attempting to load texture: " << filename << std::endl;

  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, nrComponents;
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
  if (data)
  {
    std::cout << "Successfully loaded texture: " << filename << " (" << width << "x" << height << ", " << nrComponents << " components)" << std::endl;

    GLenum format;
    if (nrComponents == 1)
      format = GL_RED;
    else if (nrComponents == 3)
      format = GL_RGB;
    else if (nrComponents == 4)
      format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }
  else
  {
    std::cout << "Texture failed to load at path: " << filename << std::endl;
    std::cout << "STB Error: " << stbi_failure_reason() << std::endl;

    // Create a 1x1 magenta texture as fallback so you can see it's missing
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char magentaPixel[] = {255, 0, 255, 255}; // Bright magenta
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, magentaPixel);

    // Don't free data if it's NULL
    if (data)
      stbi_image_free(data);
  }

  return textureID;
}
#endif
