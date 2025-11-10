Performance notes:

- Pack position.xyz + depth or reconstruct from depth + screen coordinates to save bandwidth.
- Consider storing normal in 2 components with octahedral encoding later.
- Emissive animation can expand with scrolling UV offset: add uniform uTime and sample with TexCoords + vec2(0, fract(uTime*0.2)).

Next actions for you:

- Create the new shader files above.
- Add shader compilation for new deferred shaders in your setup (similar to existing Shader usage).
- Generate SSAO kernel + noise textures on init (store 64 hemisphere samples).
- Fill in actual uniform bindings (projection/view/model, light arrays).
- Gradually migrate from existing forward light uniforms to light pass accumulation.
