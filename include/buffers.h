#pragma once

#include <glad/glad.h>

struct GBuffer
{
  unsigned int fbo = 0;
  unsigned int texPosition = 0;
  unsigned int texNormal = 0;
  unsigned int texAlbedoSpec = 0;
  unsigned int texMatProps = 0; // metal + roughness + AO
  unsigned int texEmissive = 0;
  unsigned int rboDepth = 0;

  unsigned int HDRfbo = 0;
  unsigned int bufHDR = 0;

  unsigned int DEBUGfbo = 0;
  unsigned int texNormalDEBUG = 0;
  unsigned int texDepthDEBUG = 0;
  unsigned int texMetallicDEBUG = 0;
  unsigned int texRoughnessDEBUG = 0;
  unsigned int texAODEBUG = 0;
  unsigned int texEmissiveDEBUG = 0;

  bool init(int width, int height)
  {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // gPosition, position color buffer
    glGenTextures(1, &texPosition);
    glBindTexture(GL_TEXTURE_2D, texPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPosition, 0);

    // gNormal, normal color buffer
    glGenTextures(1, &texNormal);
    glBindTexture(GL_TEXTURE_2D, texNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texNormal, 0);

    // gAlbedoMetal, albedo + specular color buffer
    glGenTextures(1, &texAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, texAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, texAlbedoSpec, 0);

    // gMatProps, metal + roughness + AO
    glGenTextures(1, &texMatProps);
    glBindTexture(GL_TEXTURE_2D, texMatProps);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, texMatProps, 0);

    // gEmissive, emissive color buffer
    glGenTextures(1, &texEmissive);
    glBindTexture(GL_TEXTURE_2D, texEmissive);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, texEmissive, 0);

    GLenum attachments[5] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4};
    glDrawBuffers(5, attachments);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "GBuffer Framebuffer not complete!" << std::endl;
      return false;
    }

    // HDR FBO
    glGenFramebuffers(1, &HDRfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, HDRfbo);

    glGenTextures(1, &bufHDR);
    glBindTexture(GL_TEXTURE_2D, bufHDR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufHDR, 0);

    GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "HDR Framebuffer not complete!" << std::endl;
      return false;
    }

    // DEBUG FBO
    glGenFramebuffers(1, &DEBUGfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, DEBUGfbo);

    glGenTextures(1, &texNormalDEBUG);
    glBindTexture(GL_TEXTURE_2D, texNormalDEBUG);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texNormalDEBUG, 0);

    glGenTextures(1, &texDepthDEBUG);
    glBindTexture(GL_TEXTURE_2D, texDepthDEBUG);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texDepthDEBUG, 0);

    GLenum debugAttachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, debugAttachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "DEBUG Framebuffer not complete!" << std::endl;
      return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
  }
};