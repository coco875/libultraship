#ifndef SHADER_TRANSLATION_H
#define SHADER_TRANSLATION_H

void glslang_spirv_cross_test();
void generate_shader(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                     const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                     std::string name_shader, std::variant<std::string, std::vector<uint32_t>>& vertShader,
                     std::variant<std::string, std::vector<uint32_t>>& fragShader);

#endif