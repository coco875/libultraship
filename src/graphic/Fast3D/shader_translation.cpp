#include <filesystem>
#include <fstream>
#include <variant>

#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/SpvTools.h>

#include <LLGL/LLGL.h>
#include <LLGL/Utils/VertexFormat.h>

#include "spirv.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "shader_translation.h"

static TBuiltInResource InitResources() {
    TBuiltInResource Resources;

    Resources.maxLights = 32;
    Resources.maxClipPlanes = 6;
    Resources.maxTextureUnits = 32;
    Resources.maxTextureCoords = 32;
    Resources.maxVertexAttribs = 64;
    Resources.maxVertexUniformComponents = 4096;
    Resources.maxVaryingFloats = 64;
    Resources.maxVertexTextureImageUnits = 32;
    Resources.maxCombinedTextureImageUnits = 80;
    Resources.maxTextureImageUnits = 32;
    Resources.maxFragmentUniformComponents = 4096;
    Resources.maxDrawBuffers = 32;
    Resources.maxVertexUniformVectors = 128;
    Resources.maxVaryingVectors = 8;
    Resources.maxFragmentUniformVectors = 16;
    Resources.maxVertexOutputVectors = 16;
    Resources.maxFragmentInputVectors = 15;
    Resources.minProgramTexelOffset = -8;
    Resources.maxProgramTexelOffset = 7;
    Resources.maxClipDistances = 8;
    Resources.maxComputeWorkGroupCountX = 65535;
    Resources.maxComputeWorkGroupCountY = 65535;
    Resources.maxComputeWorkGroupCountZ = 65535;
    Resources.maxComputeWorkGroupSizeX = 1024;
    Resources.maxComputeWorkGroupSizeY = 1024;
    Resources.maxComputeWorkGroupSizeZ = 64;
    Resources.maxComputeUniformComponents = 1024;
    Resources.maxComputeTextureImageUnits = 16;
    Resources.maxComputeImageUniforms = 8;
    Resources.maxComputeAtomicCounters = 8;
    Resources.maxComputeAtomicCounterBuffers = 1;
    Resources.maxVaryingComponents = 60;
    Resources.maxVertexOutputComponents = 64;
    Resources.maxGeometryInputComponents = 64;
    Resources.maxGeometryOutputComponents = 128;
    Resources.maxFragmentInputComponents = 128;
    Resources.maxImageUnits = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    Resources.maxCombinedShaderOutputResources = 8;
    Resources.maxImageSamples = 0;
    Resources.maxVertexImageUniforms = 0;
    Resources.maxTessControlImageUniforms = 0;
    Resources.maxTessEvaluationImageUniforms = 0;
    Resources.maxGeometryImageUniforms = 0;
    Resources.maxFragmentImageUniforms = 8;
    Resources.maxCombinedImageUniforms = 8;
    Resources.maxGeometryTextureImageUnits = 16;
    Resources.maxGeometryOutputVertices = 256;
    Resources.maxGeometryTotalOutputComponents = 1024;
    Resources.maxGeometryUniformComponents = 1024;
    Resources.maxGeometryVaryingComponents = 64;
    Resources.maxTessControlInputComponents = 128;
    Resources.maxTessControlOutputComponents = 128;
    Resources.maxTessControlTextureImageUnits = 16;
    Resources.maxTessControlUniformComponents = 1024;
    Resources.maxTessControlTotalOutputComponents = 4096;
    Resources.maxTessEvaluationInputComponents = 128;
    Resources.maxTessEvaluationOutputComponents = 128;
    Resources.maxTessEvaluationTextureImageUnits = 16;
    Resources.maxTessEvaluationUniformComponents = 1024;
    Resources.maxTessPatchComponents = 120;
    Resources.maxPatchVertices = 32;
    Resources.maxTessGenLevel = 64;
    Resources.maxViewports = 16;
    Resources.maxVertexAtomicCounters = 0;
    Resources.maxTessControlAtomicCounters = 0;
    Resources.maxTessEvaluationAtomicCounters = 0;
    Resources.maxGeometryAtomicCounters = 0;
    Resources.maxFragmentAtomicCounters = 8;
    Resources.maxCombinedAtomicCounters = 8;
    Resources.maxAtomicCounterBindings = 1;
    Resources.maxVertexAtomicCounterBuffers = 0;
    Resources.maxTessControlAtomicCounterBuffers = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers = 0;
    Resources.maxGeometryAtomicCounterBuffers = 0;
    Resources.maxFragmentAtomicCounterBuffers = 1;
    Resources.maxCombinedAtomicCounterBuffers = 1;
    Resources.maxAtomicCounterBufferSize = 16384;
    Resources.maxTransformFeedbackBuffers = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances = 8;
    Resources.maxCombinedClipAndCullDistances = 8;
    Resources.maxSamples = 4;
    Resources.maxMeshOutputVerticesNV = 256;
    Resources.maxMeshOutputPrimitivesNV = 512;
    Resources.maxMeshWorkGroupSizeX_NV = 32;
    Resources.maxMeshWorkGroupSizeY_NV = 1;
    Resources.maxMeshWorkGroupSizeZ_NV = 1;
    Resources.maxTaskWorkGroupSizeX_NV = 32;
    Resources.maxTaskWorkGroupSizeY_NV = 1;
    Resources.maxTaskWorkGroupSizeZ_NV = 1;
    Resources.maxMeshViewCountNV = 4;

    Resources.limits.nonInductiveForLoops = 1;
    Resources.limits.whileLoops = 1;
    Resources.limits.doWhileLoops = 1;
    Resources.limits.generalUniformIndexing = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing = 1;
    Resources.limits.generalSamplerIndexing = 1;
    Resources.limits.generalVariableIndexing = 1;
    Resources.limits.generalConstantMatrixVectorIndexing = 1;

    return Resources;
}

glslang::TShader create_shader(EShLanguage type, std::filesystem::path shaderPath, std::string& shaderSource,
                               char*& shaderSourceC) {
    glslang::TShader shaderGlslang(type);

    std::ifstream shaderFile(shaderPath);
    if (!shaderFile.is_open()) {
        LLGL::Log::Printf("Failed to open shader file");
        throw std::runtime_error("Failed to open shader file");
    }
    shaderSource = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());

    shaderSourceC = shaderSource.data();

    shaderGlslang.setStrings(&shaderSourceC, 1);

    // shaderGlslang.setSourceFile(shaderPath.c_str());

    shaderGlslang.setEnvInput(glslang::EShSourceGlsl, type, glslang::EShClientVulkan, 100);
    shaderGlslang.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    shaderGlslang.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);

    return shaderGlslang;
}

void parse_shader(glslang::TShader& vertShaderGlslang, glslang::TShader& fragShaderGlslang) {
    TBuiltInResource builtInResources = InitResources();
    vertShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);
    const char* vertShaderGlslangLog = vertShaderGlslang.getInfoLog();

    if (vertShaderGlslangLog != nullptr && *vertShaderGlslangLog != '\0') {
        LLGL::Log::Printf(vertShaderGlslangLog);
        throw std::runtime_error("Failed to compile vertex shader");
    }

    fragShaderGlslang.parse(&builtInResources, 120, true, EShMsgDefault);
    const char* fragShaderGlslangLog = fragShaderGlslang.getInfoLog();

    if (fragShaderGlslangLog != nullptr && *fragShaderGlslangLog != '\0') {
        LLGL::Log::Printf(fragShaderGlslangLog);
        throw std::runtime_error("Failed to compile fragment shader");
    }
}

void glslang_spirv_cross_test() {
    glslang::InitializeProcess();

    std::filesystem::path shaderPath = "../shader";
    std::filesystem::path vertShaderPath = shaderPath / "test.vert";
    std::filesystem::path fragShaderPath = shaderPath / "test.frag";

    std::string vertShaderSource;
    std::string fragShaderSource;
    char *vertShaderSourceC, *fragShaderSourceC;
    auto vertShaderGlslang = create_shader(EShLangVertex, vertShaderPath, vertShaderSource, vertShaderSourceC);
    auto fragShaderGlslang = create_shader(EShLangFragment, fragShaderPath, fragShaderSource, fragShaderSourceC);

    glslang::TProgram program;
    parse_shader(vertShaderGlslang, fragShaderGlslang);

    auto vertShaderGlslangIntermediate = vertShaderGlslang.getIntermediate();
    auto fragShaderGlslangIntermediate = fragShaderGlslang.getIntermediate();

    if (vertShaderGlslangIntermediate == nullptr || fragShaderGlslangIntermediate == nullptr) {
        LLGL::Log::Printf("Failed to link shaders");
        throw std::runtime_error("Failed to link shaders");
    }

    spv::SpvBuildLogger logger;
    std::vector<uint32_t> spirvSourceVert;
    std::vector<uint32_t> spirvSourceFrag;

    glslang::SpvOptions spvOptions;
    spvOptions.validate = false;
    spvOptions.disableOptimizer = true;
    spvOptions.optimizeSize = false;

    glslang::GlslangToSpv(*vertShaderGlslangIntermediate, spirvSourceVert, &logger, &spvOptions);
    glslang::GlslangToSpv(*fragShaderGlslangIntermediate, spirvSourceFrag, &logger, &spvOptions);

    glslang::FinalizeProcess();

    spirv_cross::CompilerGLSL::Options scoptions;
    scoptions.version = 120;
    scoptions.es = false;
    spirv_cross::CompilerGLSL glslVert(spirvSourceVert);
    glslVert.set_common_options(scoptions);
    LLGL::Log::Printf("GLSL:\n%s\n", glslVert.compile().c_str());
    spirv_cross::CompilerGLSL glslFrag(spirvSourceFrag);
    glslFrag.set_common_options(scoptions);
    LLGL::Log::Printf("GLSL:\n%s\n", glslFrag.compile().c_str());

    spirv_cross::CompilerHLSL::Options hlslOptions;
    hlslOptions.shader_model = 500;
    spirv_cross::CompilerHLSL hlslVert(spirvSourceVert);
    hlslVert.set_hlsl_options(hlslOptions);
    LLGL::Log::Printf("HLSL:\n%s\n", hlslVert.compile().c_str());
    spirv_cross::CompilerHLSL hlslFrag(spirvSourceFrag);
    hlslFrag.set_hlsl_options(hlslOptions);
    LLGL::Log::Printf("HLSL:\n%s\n", hlslFrag.compile().c_str());

    spirv_cross::CompilerMSL mslVert(spirvSourceVert);
    LLGL::Log::Printf("MSL:\n%s\n", mslVert.compile().c_str());
    spirv_cross::CompilerMSL mslFrag(spirvSourceFrag);
    LLGL::Log::Printf("MSL:\n%s\n", mslFrag.compile().c_str());
}

bool is_glsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isGLSL = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::GLSL) {
            isGLSL = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::GLSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isGLSL;
}

bool is_glsles(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isGLSLES = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::ESSL) {
            isGLSLES = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::ESSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isGLSLES;
}

bool is_hlsl(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isHLSL = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::HLSL) {
            isHLSL = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::HLSL)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isHLSL;
}

bool is_metal(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isMetal = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::Metal) {
            isMetal = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::Metal)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isMetal;
}

bool is_spirv(const std::vector<LLGL::ShadingLanguage>& languages, int& version) {
    bool isSPIRV = false;

    version = 0;

    for (const auto& language : languages) {
        if (language == LLGL::ShadingLanguage::SPIRV) {
            isSPIRV = true;
        } else if (static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::SPIRV)) {
            int v = static_cast<int>(language) & static_cast<int>(LLGL::ShadingLanguage::VersionBitmask);
            if (v > version) {
                version = v;
            }
        }
    }
    return isSPIRV;
}

void generate_shader(LLGL::ShaderDescriptor& vertShaderDesc, LLGL::ShaderDescriptor& fragShaderDesc,
                     const std::vector<LLGL::ShadingLanguage>& languages, LLGL::VertexFormat& vertexFormat,
                     std::string name_shader, std::variant<std::string, std::vector<uint32_t>>& vertShader,
                     std::variant<std::string, std::vector<uint32_t>>& fragShader) {
    glslang::InitializeProcess();

#ifdef WIN32
    std::filesystem::path shaderPath = "../../shader";
#else
    std::filesystem::path shaderPath = "../shader";
#endif
    std::filesystem::path vertShaderPath = shaderPath / (name_shader + ".vert");
    std::filesystem::path fragShaderPath = shaderPath / (name_shader + ".frag");

    std::string vertShaderSource;
    std::string fragShaderSource;
    char *vertShaderSourceC, *fragShaderSourceC;
    auto vertShaderGlslang = create_shader(EShLangVertex, vertShaderPath, vertShaderSource, vertShaderSourceC);
    auto fragShaderGlslang = create_shader(EShLangFragment, fragShaderPath, fragShaderSource, fragShaderSourceC);

    parse_shader(vertShaderGlslang, fragShaderGlslang);

    auto vertShaderGlslangIntermediate = vertShaderGlslang.getIntermediate();
    auto fragShaderGlslangIntermediate = fragShaderGlslang.getIntermediate();

    if (vertShaderGlslangIntermediate == nullptr || fragShaderGlslangIntermediate == nullptr) {
        LLGL::Log::Errorf("Failed to get intermediate\n");
        exit(1);
    }

    spv::SpvBuildLogger logger;
    std::vector<uint32_t> spirvSourceVert;
    std::vector<uint32_t> spirvSourceFrag;

    glslang::SpvOptions spvOptions;
    spvOptions.validate = false;
    spvOptions.disableOptimizer = true;
    spvOptions.optimizeSize = false;

    glslang::GlslangToSpv(*vertShaderGlslangIntermediate, spirvSourceVert, &logger, &spvOptions);
    glslang::GlslangToSpv(*fragShaderGlslangIntermediate, spirvSourceFrag, &logger, &spvOptions);

    glslang::FinalizeProcess();
    int version = 0;
    if (is_glsl(languages, version)) {
        spirv_cross::CompilerGLSL::Options scoptions;
        scoptions.version = version;
        scoptions.es = false;

        spirv_cross::CompilerGLSL glslVert(spirvSourceVert);
        glslVert.set_common_options(scoptions);
        vertShader = glslVert.compile();
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        LLGL::Log::Printf("GLSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        spirv_cross::CompilerGLSL glslFrag(spirvSourceFrag);
        glslFrag.set_common_options(scoptions);
        glslFrag.build_combined_image_samplers();
        auto remap = glslFrag.get_combined_image_samplers();
        fragShader = glslFrag.compile();
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        LLGL::Log::Printf("GLSL:\n%s\n", std::get<std::string>(fragShader).c_str());
    } else if (is_glsles(languages, version)) {
        spirv_cross::CompilerGLSL::Options scoptions;
        scoptions.version = version;
        scoptions.es = true;

        spirv_cross::CompilerGLSL glslVert(spirvSourceVert);
        glslVert.set_common_options(scoptions);
        vertShader = glslVert.compile();
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        LLGL::Log::Printf("GLSL ES:\n%s\n", std::get<std::string>(vertShader).c_str());

        spirv_cross::CompilerGLSL glslFrag(spirvSourceFrag);
        glslFrag.set_common_options(scoptions);
        fragShader = glslFrag.compile();
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        LLGL::Log::Printf("GLSL ES:\n%s\n", std::get<std::string>(fragShader).c_str());
    } else if (is_spirv(languages, version)) {
        vertShader = spirvSourceVert;
        char* vertShaderSourceC = (char*) malloc(spirvSourceVert.size() * sizeof(uint32_t));
        memcpy(vertShaderSourceC, &(*spirvSourceVert.begin()), spirvSourceVert.size() * sizeof(uint32_t));
        vertShaderDesc = { LLGL::ShaderType::Vertex, vertShaderSourceC };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        vertShaderDesc.sourceSize = spirvSourceVert.size() * sizeof(uint32_t);

        fragShader = spirvSourceFrag;
        char* fragShaderSourceC = (char*) malloc(spirvSourceFrag.size() * sizeof(uint32_t));
        memcpy(fragShaderSourceC, &(*spirvSourceFrag.begin()), spirvSourceFrag.size() * sizeof(uint32_t));
        fragShaderDesc = { LLGL::ShaderType::Fragment, (char*) fragShaderSourceC };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::BinaryBuffer;
        fragShaderDesc.sourceSize = spirvSourceFrag.size() * sizeof(uint32_t);

        LLGL::Log::Printf("SPIRV:\n");
    } else if (is_hlsl(languages, version)) {
        spirv_cross::CompilerHLSL::Options hlslOptions;
        hlslOptions.shader_model = version / 10;

        spirv_cross::CompilerHLSL hlslVert(spirvSourceVert);
        hlslVert.set_hlsl_options(hlslOptions);
        for (unsigned int i = 0; i < vertexFormat.attributes.size(); i++) {
            hlslVert.add_vertex_attribute_remap({ i, vertexFormat.attributes[i].name.c_str() });
        }
        vertShader = hlslVert.compile();
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        vertShaderDesc.entryPoint = "main";
        vertShaderDesc.profile = "vs_5_0";
        LLGL::Log::Printf("HLSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        spirv_cross::CompilerHLSL hlslFrag(spirvSourceFrag);
        hlslFrag.set_hlsl_options(hlslOptions);
        fragShader = hlslFrag.compile();
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        fragShaderDesc.entryPoint = "main";
        fragShaderDesc.profile = "ps_5_0";
        LLGL::Log::Printf("HLSL:\n%s\n", std::get<std::string>(fragShader).c_str());
    } else if (is_metal(languages, version)) {
        spirv_cross::CompilerMSL mslVert(spirvSourceVert);
        vertShader = mslVert.compile();
        vertShaderDesc = { LLGL::ShaderType::Vertex, std::get<std::string>(vertShader).c_str() };
        vertShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        // vertShaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
        vertShaderDesc.entryPoint = "main0";
        vertShaderDesc.profile = "2.1";
        LLGL::Log::Printf("MSL:\n%s\n", std::get<std::string>(vertShader).c_str());

        spirv_cross::CompilerMSL mslFrag(spirvSourceFrag);
        fragShader = mslFrag.compile();
        fragShaderDesc = { LLGL::ShaderType::Fragment, std::get<std::string>(fragShader).c_str() };
        fragShaderDesc.sourceType = LLGL::ShaderSourceType::CodeString;
        // fragShaderDesc.flags |= LLGL::ShaderCompileFlags::DefaultLibrary;
        fragShaderDesc.entryPoint = "main0";
        fragShaderDesc.profile = "2.1";
        LLGL::Log::Printf("MSL:\n%s\n", std::get<std::string>(fragShader).c_str());
    }
}