
#include <prism/processor.h>
#include "gfx_rendering_api.h"
#include "window/gui/Gui.h"
#include "../interpreter.h"
#include <LLGL/LLGL.h>
#include "LLGL/Utils/VertexFormat.h"
#ifdef LLGL_OS_LINUX
#include <GL/glx.h>
#endif
#include "gfx_llgl.h"

#include "gfx_rendering_api.h"

#include <LLGL/Backend/OpenGL/NativeHandle.h>
#if LLGL_BUILD_RENDERER_VULKAN
#include <LLGL/Backend/Vulkan/NativeHandle.h>
#include <imgui_impl_vulkan.h>
#include <LLGL/../../sources/Renderer/Vulkan/Texture/VKTexture.h>
#endif
#include "../../../resource/type/Shader.h"
#include <Context.h>
#include "../shader_translation.h"
#include "graphic/Fast3D/Fast3dWindow.h"

#include "utils/StringHelper.h"
#include <consolevariablebridge.h>

LLGL::RenderSystemPtr llgl_renderer;
LLGL::SwapChain* llgl_swapChain;
LLGL::CommandBuffer* llgl_cmdBuffer;
namespace Fast {

GfxRenderingAPILLGL::GfxRenderingAPILLGL(GfxWindowBackend* backend) {
    mWindowBackend = backend;
}

const char* Fast::GfxRenderingAPILLGL::GetName(void) {
    // renderer->GetName();
    return "LLGL";
}

int Fast::GfxRenderingAPILLGL::GetMaxTextureSize(void) {
    auto caps = llgl_renderer->GetRenderingCaps();
    return caps.limits.max2DTextureSize;
}

Fast::GfxClipParameters Fast::GfxRenderingAPILLGL::GetClipParameters(void) {
    return { false, false };
}

void Fast::GfxRenderingAPILLGL::UnloadShader(struct ShaderProgram* old_prg) {
}

void Fast::GfxRenderingAPILLGL::LoadShader(struct ShaderProgram* new_prg) {
    mCurrentShaderProgram = (ShaderProgramLLGL*)new_prg;
}

static void llgl_append_str(char* buf, size_t* len, const char* str) {
    while (*str != '\0') {
        buf[(*len)++] = *str++;
    }
}

static void llgl_append_line(char* buf, size_t* len, const char* str) {
    while (*str != '\0') {
        buf[(*len)++] = *str++;
    }
    buf[(*len)++] = '\n';
}

#define RAND_NOISE "((random(vec3(floor(gl_FragCoord.xy * noise_scale), float(frame_count))) + 1.0) / 2.0)"

static const char* llgl_shader_item_to_str(uint32_t item, bool with_alpha, bool only_alpha, bool inputs_have_alpha,
                                           bool first_cycle, bool hint_single_element) {
    if (!only_alpha) {
        switch (item) {
            case SHADER_0:
                return with_alpha ? "vec4(0.0, 0.0, 0.0, 0.0)" : "vec3(0.0, 0.0, 0.0)";
            case SHADER_1:
                return with_alpha ? "vec4(1.0, 1.0, 1.0, 1.0)" : "vec3(1.0, 1.0, 1.0)";
            case SHADER_INPUT_1:
                return with_alpha || !inputs_have_alpha ? "vInput1" : "vInput1.rgb";
            case SHADER_INPUT_2:
                return with_alpha || !inputs_have_alpha ? "vInput2" : "vInput2.rgb";
            case SHADER_INPUT_3:
                return with_alpha || !inputs_have_alpha ? "vInput3" : "vInput3.rgb";
            case SHADER_INPUT_4:
                return with_alpha || !inputs_have_alpha ? "vInput4" : "vInput4.rgb";
            case SHADER_TEXEL0:
                return first_cycle ? (with_alpha ? "texVal0" : "texVal0.rgb")
                                   : (with_alpha ? "texVal1" : "texVal1.rgb");
            case SHADER_TEXEL0A:
                return first_cycle
                           ? (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"))
                           : (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"));
            case SHADER_TEXEL1A:
                return first_cycle
                           ? (hint_single_element ? "texVal1.a"
                                                  : (with_alpha ? "vec4(texVal1.a, texVal1.a, texVal1.a, texVal1.a)"
                                                                : "vec3(texVal1.a, texVal1.a, texVal1.a)"))
                           : (hint_single_element ? "texVal0.a"
                                                  : (with_alpha ? "vec4(texVal0.a, texVal0.a, texVal0.a, texVal0.a)"
                                                                : "vec3(texVal0.a, texVal0.a, texVal0.a)"));
            case SHADER_TEXEL1:
                return first_cycle ? (with_alpha ? "texVal1" : "texVal1.rgb")
                                   : (with_alpha ? "texVal0" : "texVal0.rgb");
            case SHADER_COMBINED:
                return with_alpha ? "texel" : "texel.rgb";
            case SHADER_NOISE:
                return with_alpha ? "vec4(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")"
                                  : "vec3(" RAND_NOISE ", " RAND_NOISE ", " RAND_NOISE ")";
        }
    } else {
        switch (item) {
            case SHADER_0:
                return "0.0";
            case SHADER_1:
                return "1.0";
            case SHADER_INPUT_1:
                return "vInput1.a";
            case SHADER_INPUT_2:
                return "vInput2.a";
            case SHADER_INPUT_3:
                return "vInput3.a";
            case SHADER_INPUT_4:
                return "vInput4.a";
            case SHADER_TEXEL0:
                return first_cycle ? "texVal0.a" : "texVal1.a";
            case SHADER_TEXEL0A:
                return first_cycle ? "texVal0.a" : "texVal1.a";
            case SHADER_TEXEL1A:
                return first_cycle ? "texVal1.a" : "texVal0.a";
            case SHADER_TEXEL1:
                return first_cycle ? "texVal1.a" : "texVal0.a";
            case SHADER_COMBINED:
                return "texel.a";
            case SHADER_NOISE:
                return RAND_NOISE;
        }
    }
    return "";
}

bool llgl_get_bool(prism::ContextTypes* value) {
    if (std::holds_alternative<int>(*value)) {
        return std::get<int>(*value) == 1;
    }
    return false;
}

prism::ContextTypes* llgl_append_formula(prism::ContextItems* _, prism::ContextTypes* a_arg,
                                         prism::ContextTypes* a_single, prism::ContextTypes* a_mult,
                                         prism::ContextTypes* a_mix, prism::ContextTypes* a_with_alpha,
                                         prism::ContextTypes* a_only_alpha, prism::ContextTypes* a_alpha,
                                         prism::ContextTypes* a_first_cycle) {
    auto c = std::get<prism::MTDArray<int>>(*a_arg);
    bool do_single = llgl_get_bool(a_single);
    bool do_multiply = llgl_get_bool(a_mult);
    bool do_mix = llgl_get_bool(a_mix);
    bool with_alpha = llgl_get_bool(a_with_alpha);
    bool only_alpha = llgl_get_bool(a_only_alpha);
    bool opt_alpha = llgl_get_bool(a_alpha);
    bool first_cycle = llgl_get_bool(a_first_cycle);
    std::string out = "";
    if (do_single) {
        out += llgl_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    } else if (do_multiply) {
        out += llgl_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " * ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
    } else if (do_mix) {
        out += "mix(";
        out += llgl_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ", ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += ")";
    } else {
        out += "(";
        out += llgl_shader_item_to_str(c.at(only_alpha, 0), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += " - ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 1), with_alpha, only_alpha, opt_alpha, first_cycle, false);
        out += ") * ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 2), with_alpha, only_alpha, opt_alpha, first_cycle, true);
        out += " + ";
        out += llgl_shader_item_to_str(c.at(only_alpha, 3), with_alpha, only_alpha, opt_alpha, first_cycle, false);
    }
    return new prism::ContextTypes{ out };
}

std::optional<std::string> llgl_opengl_include_fs(const std::string& path) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path, true, init));
    if (res == nullptr) {
        return std::nullopt;
    }
    auto inc = static_cast<std::string*>(res->GetRawPointer());
    return *inc;
}

std::string prism_to_string(const prism::ContextTypes* value) {
    if (std::holds_alternative<std::string>(*value)) {
        return std::get<std::string>(*value);
    } else if (std::holds_alternative<int>(*value)) {
        return std::to_string(std::get<int>(*value));
    } else if (std::holds_alternative<float>(*value)) {
        return std::to_string(std::get<float>(*value));
    }
    return "";
}

prism::ContextTypes* prism_context_to_string(prism::ContextItems* items, prism::ContextTypes* value) {
    return new prism::ContextTypes{ prism_to_string(value) };
}

LLGL::Format llgl_get_format(const std::string& format) {
    if (format == "RGBA32Float") {
        return LLGL::Format::RGBA32Float;
    } else if (format == "RGB32Float") {
        return LLGL::Format::RGB32Float;
    } else if (format == "RG32Float") {
        return LLGL::Format::RG32Float;
    } else if (format == "R32Float") {
        return LLGL::Format::R32Float;
    }
    // Add more formats as needed
    return LLGL::Format::Undefined;
}

prism::ContextTypes* get_vs_input_location(prism::ContextItems* items, prism::ContextTypes* name,
                                           prism::ContextTypes* vtx_format) {
    auto format = std::get<std::string>(*vtx_format);
    auto name_str = std::get<std::string>(*name);

    int& input_index = std::get<int>(items->at("input_index"));
    auto input = input_index;
    input_index++;

    LLGL::VertexFormat* vertex_format =
        static_cast<LLGL::VertexFormat*>((void*)std::get<prism::Opaque>(items->at("vertex_format")).ptr);

    LLGL::StringLiteral name_literal{ name_str.c_str(), LLGL::CopyTag{} };

    vertex_format->AppendAttribute({ name_literal, llgl_get_format(format) });

    return new prism::ContextTypes{ input };
}

prism::ContextTypes* get_input_location(prism::ContextItems* items) {
    int& input_index = std::get<int>(items->at("input_index"));
    auto input = input_index;
    input_index++;
    return new prism::ContextTypes{ input };
}

prism::ContextTypes* get_output_location(prism::ContextItems* items) {
    int& output_index = std::get<int>(items->at("output_index"));
    auto output = output_index;
    output_index++;
    return new prism::ContextTypes{ output };
}

LLGL::ResourceType llgl_get_resource_type(const std::string& type) {
    if (type == "Texture") {
        return LLGL::ResourceType::Texture;
    } else if (type == "Buffer") {
        return LLGL::ResourceType::Buffer;
    } else if (type == "Sampler") {
        return LLGL::ResourceType::Sampler;
    }
    // Add more resource types as needed
    return LLGL::ResourceType::Undefined;
}

int llgl_get_binding_type(const std::string& type) {
    if (type == "ConstantBuffer") {
        return LLGL::BindFlags::ConstantBuffer;
    } else if (type == "Sampled") {
        return LLGL::BindFlags::Sampled;
    }
    // Add more binding types as needed
    return 0;
}

prism::ContextTypes* get_binding_index(prism::ContextItems* items, prism::ContextTypes* name,
                                       prism::ContextTypes* resource_type, prism::ContextTypes* binding_type) {
    auto name_str = std::get<std::string>(*name);

    int& binding_index = std::get<int>(items->at("binding_index"));
    auto bind = binding_index;
    binding_index++;

    long stage_flags = std::get<int>(items->at("stage_flags"));

    LLGL::StringLiteral name_literal{ name_str.c_str(), LLGL::CopyTag{} };

    LLGL::PipelineLayoutDescriptor* layoutDesc =
        static_cast<LLGL::PipelineLayoutDescriptor*>((void*)std::get<prism::Opaque>(items->at("layout_desc")).ptr);
    layoutDesc->bindings.push_back(LLGL::BindingDescriptor{
        name_literal, llgl_get_resource_type(std::get<std::string>(*resource_type)),
        (long)llgl_get_binding_type(std::get<std::string>(*binding_type)), stage_flags, bind });
    if (std::get<std::string>(*resource_type) == "Sampler") {
        int index = 0;
        for (const auto& bind : layoutDesc->bindings) {
            if (bind.stageFlags == stage_flags && bind.type == LLGL::ResourceType::Texture &&
                bind.name == std::get<std::string>(*binding_type)) {
                layoutDesc->combinedTextureSamplers.push_back(
                    LLGL::CombinedTextureSamplerDescriptor{ bind.name, bind.name, name_literal, bind.slot });
                break;
            }
            index++;
        }
    }
    return new prism::ContextTypes{ bind };
}

std::string GfxRenderingAPILLGL::llgl_build_fs_shader(const CCFeatures& cc_features,
                                                      LLGL::PipelineLayoutDescriptor& layoutDesc) {
    prism::Processor processor;
    prism::ContextItems context = {
        { "o_c", M_ARRAY(cc_features.c, int, 2, 2, 4) },
        { "o_alpha", cc_features.opt_alpha },
        { "o_fog", cc_features.opt_fog },
        { "o_texture_edge", cc_features.opt_texture_edge },
        { "o_noise", cc_features.opt_noise },
        { "o_2cyc", cc_features.opt_2cyc },
        { "o_alpha_threshold", cc_features.opt_alpha_threshold },
        { "o_invisible", cc_features.opt_invisible },
        { "o_grayscale", cc_features.opt_grayscale },
        { "o_textures", M_ARRAY(cc_features.usedTextures, bool, 2) },
        { "o_masks", M_ARRAY(cc_features.used_masks, bool, 2) },
        { "o_blend", M_ARRAY(cc_features.used_blend, bool, 2) },
        { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
        { "o_inputs", cc_features.numInputs },
        { "o_do_mix", M_ARRAY(cc_features.do_mix, bool, 2, 2) },
        { "o_do_single", M_ARRAY(cc_features.do_single, bool, 2, 2) },
        { "o_do_multiply", M_ARRAY(cc_features.do_multiply, bool, 2, 2) },
        { "o_color_alpha_same", M_ARRAY(cc_features.color_alpha_same, bool, 2) },
        { "o_three_point_filtering", current_filter_mode == FILTER_THREE_POINT },
        { "FILTER_THREE_POINT", Fast::FILTER_THREE_POINT },
        { "FILTER_LINEAR", Fast::FILTER_LINEAR },
        { "FILTER_NONE", Fast::FILTER_NONE },
        { "srgb_mode", srgb_mode },
        { "SHADER_0", SHADER_0 },
        { "SHADER_INPUT_1", SHADER_INPUT_1 },
        { "SHADER_INPUT_2", SHADER_INPUT_2 },
        { "SHADER_INPUT_3", SHADER_INPUT_3 },
        { "SHADER_INPUT_4", SHADER_INPUT_4 },
        { "SHADER_INPUT_5", SHADER_INPUT_5 },
        { "SHADER_INPUT_6", SHADER_INPUT_6 },
        { "SHADER_INPUT_7", SHADER_INPUT_7 },
        { "SHADER_TEXEL0", SHADER_TEXEL0 },
        { "SHADER_TEXEL0A", SHADER_TEXEL0A },
        { "SHADER_TEXEL1", SHADER_TEXEL1 },
        { "SHADER_TEXEL1A", SHADER_TEXEL1A },
        { "SHADER_1", SHADER_1 },
        { "SHADER_COMBINED", SHADER_COMBINED },
        { "SHADER_NOISE", SHADER_NOISE },
        { "append_formula", (InvokeFunc)llgl_append_formula },
        { "get_input_location", (InvokeFunc)get_input_location },
        { "get_binding_index", (InvokeFunc)get_binding_index },
        { "to_string", (InvokeFunc)prism_context_to_string },
        // local variables
        { "input_index", 0 },
        { "binding_index", 1 },
        { "layout_desc", prism::Opaque{ (uintptr_t)&layoutDesc } },
        { "stage_flags", LLGL::StageFlags::FragmentStage },
    };
    processor.populate(context);
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource("shaders/default.shader.fs", true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default fragment shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(llgl_opengl_include_fs);
    auto result = processor.process();

    SPDLOG_INFO("=========== FRAGMENT SHADER ============");
    // print line per line with number
    size_t line_num = 0;
    for (const auto& line : StringHelper::Split(result, "\n")) {
        printf("%zu: %s\n", line_num, line.c_str());
        line_num++;
    }
    SPDLOG_INFO("========================================");
    return result;
}

std::string GfxRenderingAPILLGL::llgl_build_vs_shader(const CCFeatures& cc_features,
                                                      LLGL::PipelineLayoutDescriptor& layoutDesc,
                                                      LLGL::VertexFormat& vertexFormat) {
    prism::Processor processor;

    prism::ContextItems context = {
        { "o_textures", M_ARRAY(cc_features.usedTextures, bool, 2) },
        { "o_clamp", M_ARRAY(cc_features.clamp, bool, 2, 2) },
        { "o_fog", cc_features.opt_fog },
        { "o_grayscale", cc_features.opt_grayscale },
        { "o_alpha", cc_features.opt_alpha },
        { "o_inputs", cc_features.numInputs },
        { "get_vs_input_location", (InvokeFunc)get_vs_input_location },
        { "get_output_location", (InvokeFunc)get_output_location },
        { "to_string", (InvokeFunc)prism_context_to_string },
        // local variables
        { "input_index", 0 },
        { "binding_index", 1 },
        { "output_index", 0 },
        { "vertex_format", prism::Opaque{ (uintptr_t)&vertexFormat } },
        { "layout_desc", prism::Opaque{ (uintptr_t)&layoutDesc } },
        { "stage_flags", LLGL::StageFlags::VertexStage },
    };
    processor.populate(context);

    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Type = (uint32_t)Ship::ResourceType::Shader;
    init->ByteOrder = Ship::Endianness::Native;
    init->Format = RESOURCE_FORMAT_BINARY;
    auto res = static_pointer_cast<Ship::Shader>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource("shaders/default.shader.vs", true, init));

    if (res == nullptr) {
        SPDLOG_ERROR("Failed to load default vertex shader, missing f3d.o2r?");
        abort();
    }

    auto shader = static_cast<std::string*>(res->GetRawPointer());
    processor.load(*shader);
    processor.bind_include_loader(llgl_opengl_include_fs);
    auto result = processor.process();
    SPDLOG_INFO("=========== VERTEX SHADER ============");
    // print line per line with number
    size_t line_num = 0;
    for (const auto& line : StringHelper::Split(result, "\n")) {
        printf("%zu: %s\n", line_num, line.c_str());
        line_num++;
    }
    SPDLOG_INFO("========================================");
    return result;
}

LLGL::PipelineState* create_pipeline(LLGL::RenderSystemPtr& llgl_renderer, LLGL::SwapChain* llgl_swapChain,
                                     LLGL::VertexFormat& vertexFormat, std::string vertShaderSource,
                                     std::string fragShaderSource, LLGL::GraphicsPipelineDescriptor& pipelineDesc,
                                     LLGL::PipelineLayout* pipelineLayout = nullptr) {
    const auto& languages = llgl_renderer->GetRenderingCaps().shadingLanguages;
    LLGL::ShaderDescriptor vertShaderDesc, fragShaderDesc;

    std::variant<std::string, std::vector<uint32_t>> vertShaderSourceC, fragShaderSourceC;

    generate_shader_from_string(vertShaderDesc, fragShaderDesc, languages, vertexFormat, vertShaderSource,
                                fragShaderSource, vertShaderSourceC, fragShaderSourceC);

    // Specify vertex attributes for vertex shader
    vertShaderDesc.vertex.inputAttribs = vertexFormat.attributes;

    LLGL::Shader* vertShader = llgl_renderer->CreateShader(vertShaderDesc);
    LLGL::Shader* fragShader = llgl_renderer->CreateShader(fragShaderDesc);

    if (const LLGL::Report* report = vertShader->GetReport()) {
        if (std::holds_alternative<std::string>(vertShaderSourceC)) {
            int line_num = 0;
            for (const auto& line : StringHelper::Split(std::get<std::string>(vertShaderSourceC), "\n")) {
                printf("%d: %s\n", line_num, line.c_str());
                line_num++;
            }
        }
        SPDLOG_ERROR("vertShader: {}", report->GetText());
    }

    if (const LLGL::Report* report = fragShader->GetReport()) {
        if (std::holds_alternative<std::string>(fragShaderSourceC)) {
            int line_num = 0;
            for (const auto& line : StringHelper::Split(std::get<std::string>(fragShaderSourceC), "\n")) {
                printf("%d: %s\n", line_num, line.c_str());
                line_num++;
            }
        }
        SPDLOG_ERROR("fragShader: {}", report->GetText());
    }

    if (std::holds_alternative<std::string>(vertShaderSourceC)) {
        int line_num = 0;
        for (const auto& line : StringHelper::Split(std::get<std::string>(vertShaderSourceC), "\n")) {
            printf("%d: %s\n", line_num, line.c_str());
            line_num++;
        }
    }

    if (std::holds_alternative<std::string>(fragShaderSourceC)) {
        int line_num = 0;
        for (const auto& line : StringHelper::Split(std::get<std::string>(fragShaderSourceC), "\n")) {
            printf("%d: %s\n", line_num, line.c_str());
            line_num++;
        }
    }

    // Create graphics pipeline
    LLGL::PipelineState* pipeline = nullptr;
    LLGL::PipelineCache* pipelineCache = nullptr;

    {
        pipelineDesc.vertexShader = vertShader;
        pipelineDesc.fragmentShader = fragShader;
        pipelineDesc.renderPass = llgl_swapChain->GetRenderPass();
        pipelineDesc.pipelineLayout = pipelineLayout;
        pipelineDesc.depth = LLGL::DepthDescriptor{
            .testEnabled = false,
            .writeEnabled = false,
            .compareOp = LLGL::CompareOp::AlwaysPass,
        };
        pipelineDesc.rasterizer = LLGL::RasterizerDescriptor{ .cullMode = LLGL::CullMode::Disabled };
        pipelineDesc.blend.targets[0].blendEnabled = true;
    }

    // Create graphics PSO
    pipeline = llgl_renderer->CreatePipelineState(pipelineDesc, pipelineCache);

    // Link shader program and check for errors
    if (const LLGL::Report* report = pipeline->GetReport()) {
        if (report->HasErrors()) {
            const char* a = report->GetText();
            SPDLOG_ERROR(a);
            throw std::runtime_error("Failed to link shader program");
        }
    }
    return pipeline;
}

LLGL::PipelineState* duplicate_pipeline(LLGL::RenderSystemPtr& llgl_renderer,
                                        LLGL::GraphicsPipelineDescriptor& pipelineDesc) {
    // Create graphics pipeline
    LLGL::PipelineCache* pipelineCache = nullptr;

    // Create graphics PSO
    LLGL::PipelineState* pipeline = llgl_renderer->CreatePipelineState(pipelineDesc, pipelineCache);

    // Link shader program and check for errors
    if (const LLGL::Report* report = pipeline->GetReport()) {
        if (report->HasErrors()) {
            const char* a = report->GetText();
            SPDLOG_ERROR(a);
            throw std::runtime_error("Failed to link shader program");
        }
    }
    return pipeline;
}

struct ShaderProgram* Fast::GfxRenderingAPILLGL::CreateAndLoadNewShader(uint64_t shader_id0, uint32_t shader_id1) {
    CCFeatures cc_features;
    gfx_cc_get_features(shader_id0, shader_id1, &cc_features);

    LLGL::VertexFormat vertexFormat;
    LLGL::PipelineLayoutDescriptor layoutDesc;

    const auto vs_buf = llgl_build_vs_shader(cc_features, layoutDesc, vertexFormat);
    const auto fs_buf = llgl_build_fs_shader(cc_features, layoutDesc);

    LLGL::PipelineLayout* pipeline_layout = llgl_renderer->CreatePipelineLayout(layoutDesc);

    ShaderProgramLLGL* prg = &mShaderProgramPool[std::make_pair(shader_id0, shader_id1)];

    LLGL::GraphicsPipelineDescriptor pipelineDesc;
    for (int depth_bool = 0; depth_bool < 2; depth_bool++) {
        for (int write_depth_bool = 0; write_depth_bool < 2; write_depth_bool++) {
            if (depth_bool == 0 && write_depth_bool == 0) {
                auto pipeline = create_pipeline(llgl_renderer, llgl_swapChain, vertexFormat, vs_buf, fs_buf,
                                                pipelineDesc, pipeline_layout);
                prg->pipeline[0][0] = pipeline;
            } else {
                pipelineDesc.depth.writeEnabled = write_depth_bool;
                pipelineDesc.depth.testEnabled = depth_bool;
                if (!depth_bool) {
                    pipelineDesc.depth.compareOp = LLGL::CompareOp::AlwaysPass;
                } else {
                    pipelineDesc.depth.compareOp = LLGL::CompareOp::Less;
                }
                auto pipeline = duplicate_pipeline(llgl_renderer, pipelineDesc);
                prg->pipeline[depth_bool][write_depth_bool] = pipeline;
            }
        }
    }

    int i = 0;
    for (auto& binding : layoutDesc.bindings) {
        // harcode for now
        if (binding.name.compare("uTex0") == 0) {
            prg->bindingTexture[0] = i;
        } else if (binding.name.compare("uTexSampl0") == 0) {
            prg->bindingTextureSampl[0] = i;
        } else if (binding.name.compare("uTex1") == 0) {
            prg->bindingTexture[1] = i;
        } else if (binding.name.compare("uTexSampl1") == 0) {
            prg->bindingTextureSampl[1] = i;
        } else if (binding.name.compare("uTexMask0") == 0) {
            prg->bindingMask[0] = i;
        } else if (binding.name.compare("uTexMaskSampl0") == 0) {
            prg->bindingMaskSampl[0] = i;
        } else if (binding.name.compare("uTexMask1") == 0) {
            prg->bindingMask[1] = i;
        } else if (binding.name.compare("uTexMaskSampl1") == 0) {
            prg->bindingMaskSampl[1] = i;
        } else if (binding.name.compare("uTexBlend0") == 0) {
            prg->bindingBlend[0] = i;
        } else if (binding.name.compare("uTexBlendSampl0") == 0) {
            prg->bindingBlendSampl[0] = i;
        } else if (binding.name.compare("uTexBlend1") == 0) {
            prg->bindingBlend[1] = i;
        } else if (binding.name.compare("uTexBlendSampl1") == 0) {
            prg->bindingBlendSampl[1] = i;
        } else if (binding.name.compare("frame_count") == 0) {
            prg->frameCountBinding = i;
        } else if (binding.name.compare("noise_scale") == 0) {
            prg->noiseScaleBinding = i;
        }
        i++;
    }
    prg->numInputs = cc_features.numInputs;
    prg->vertexFormat = vertexFormat;
    mCurrentShaderProgram = prg;
    return (struct ShaderProgram*)prg;
}

Fast::ShaderProgram* Fast::GfxRenderingAPILLGL::LookupShader(uint64_t shader_id0, uint32_t shader_id1) {
    auto it = mShaderProgramPool.find(std::make_pair(shader_id0, shader_id1));
    return it == mShaderProgramPool.end() ? nullptr : (struct ShaderProgram*)&it->second;
}

void Fast::GfxRenderingAPILLGL::ShaderGetInfo(struct ShaderProgram* prg, uint8_t* num_inputs, bool used_textures[2]) {
    auto p = (ShaderProgramLLGL*)(prg);
    if (p == nullptr) {
        *num_inputs = 0;
        used_textures[0] = false;
        used_textures[1] = false;
        return;
    }
    *num_inputs = p->numInputs;
    used_textures[0] = p->bindingTexture[0].has_value();
    used_textures[1] = p->bindingTexture[1].has_value();
}

uint32_t Fast::GfxRenderingAPILLGL::NewTexture(void) {
    textures.resize(textures.size() + 1);
    textures[textures.size() - 1].second = samplers[{ false, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP }];
    return (uint32_t)(textures.size() - 1);
}

void Fast::GfxRenderingAPILLGL::SelectTexture(int tile, uint32_t texture_id) {
    // tile 0-1 normal texture, 2-3 mask texture 4-5 blend texture
    if (tile < 0 || tile >= 6) {
        SPDLOG_ERROR("Invalid tile index: {}", tile);
        return;
    }
    // TODO: not finish
    current_tile = tile;
    current_texture_ids[current_tile] = texture_id;
}

void Fast::GfxRenderingAPILLGL::UploadTexture(const uint8_t* rgba32_buf, uint32_t width, uint32_t height) {
    // TODO: not finish
    LLGL::ImageView imageView;
    imageView.format = LLGL::ImageFormat::RGBA;
    imageView.data = rgba32_buf;
    imageView.dataSize = width * height * 4;
    LLGL::TextureDescriptor texDesc;
    texDesc.type = LLGL::TextureType::Texture2D;
    texDesc.format = LLGL::Format::RGBA8UNorm;
    texDesc.extent = { width, height, 1 };
    textures[current_texture_ids[current_tile]].first = llgl_renderer->CreateTexture(texDesc, &imageView);
    textures[current_texture_ids[current_tile]].second =
        samplers[{ false, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP }];
}

static LLGL::SamplerAddressMode gfx_cm_to_llgl(uint32_t val) {
    switch (val) {
        case G_TX_NOMIRROR | G_TX_CLAMP:
            return LLGL::SamplerAddressMode::Clamp;
        case G_TX_MIRROR | G_TX_WRAP:
            return LLGL::SamplerAddressMode::Mirror;
        case G_TX_MIRROR | G_TX_CLAMP:
            // maybe some change are needed here
            return LLGL::SamplerAddressMode::MirrorOnce;
        case G_TX_NOMIRROR | G_TX_WRAP:
            return LLGL::SamplerAddressMode::Repeat;
    }
    return LLGL::SamplerAddressMode::Clamp;
}

void Fast::GfxRenderingAPILLGL::SetSamplerParameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    textures[current_texture_ids[tile]].second = samplers[{ linear_filter, cms, cmt }];
}

void Fast::GfxRenderingAPILLGL::SetDepthTestAndMask(bool depth_test, bool z_upd) {
    disable_depth = !depth_test;
    disable_write_depth = !z_upd;
}

void Fast::GfxRenderingAPILLGL::SetZmodeDecal(bool zmode_decal) {
}

void Fast::GfxRenderingAPILLGL::SetViewport(int x, int y, int width, int height) {
    auto resolution = llgl_swapChain->GetResolution();
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    int render_width, render_height;
    SDL_GL_GetDrawableSize(mWindowBackend->mInitData.LLGL.Window->wnd, &render_width, &render_height);
    if (render_width > 0 && render_height > 0) {
        scale_x = static_cast<float>(render_width) / resolution.width;
        scale_y = static_cast<float>(render_height) / resolution.height;
    }
    // x = static_cast<int>(x * scale_x);
    // y = static_cast<int>(y * scale_y);
    // width = static_cast<int>(width * scale_x);
    // height = static_cast<int>(height * scale_y);
    int y_inverted = resolution.height - y - height;
    llgl_cmdBuffer->SetViewport(LLGL::Viewport(x, y_inverted, width, height));
}

void Fast::GfxRenderingAPILLGL::SetScissor(int x, int y, int width, int height) {
    auto resolution = llgl_swapChain->GetResolution();

    float scale_x = 1.0f;
    float scale_y = 1.0f;
    int render_width, render_height;
    SDL_GL_GetDrawableSize(mWindowBackend->mInitData.LLGL.Window->wnd, &render_width, &render_height);
    if (render_width > 0 && render_height > 0) {
        scale_x = static_cast<float>(render_width) / resolution.width;
        scale_y = static_cast<float>(render_height) / resolution.height;
    }
    // x = static_cast<int>(x * scale_x);
    // y = static_cast<int>(y * scale_y);
    // width = static_cast<int>(width * scale_x);
    // height = static_cast<int>(height * scale_y);
    int y_inverted = resolution.height - y - height;
    llgl_cmdBuffer->SetScissor(LLGL::Scissor(x, y_inverted, width, height));
}

void Fast::GfxRenderingAPILLGL::SetUseAlpha(bool use_alpha) {
}

void Fast::GfxRenderingAPILLGL::DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    if (vertexBufferSize < buf_vbo_len) {
        garbage_collection_buffers.push_back(vertexBuffer);
        vertexBuffer = nullptr;
        LLGL::BufferDescriptor vboDesc;
        {
            vboDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
            vboDesc.size = buf_vbo_len * sizeof(float);
        }
        vertexBuffer = llgl_renderer->CreateBuffer(vboDesc);
        vertexBufferSize = buf_vbo_len;
    }
    llgl_cmdBuffer->UpdateBuffer(*vertexBuffer, 0, buf_vbo, buf_vbo_len * sizeof(float));
    llgl_cmdBuffer->SetVertexBuffer(*vertexBuffer, mCurrentShaderProgram->vertexFormat.attributes.size(),
                                    mCurrentShaderProgram->vertexFormat.attributes.data());
    llgl_cmdBuffer->SetPipelineState(
        *mCurrentShaderProgram->pipeline[disable_depth ? 0 : 1][disable_write_depth ? 0 : 1]);

    llgl_cmdBuffer->SetResource(mCurrentShaderProgram->frameCountBinding, *frameCountBuffer);
    llgl_cmdBuffer->SetResource(mCurrentShaderProgram->noiseScaleBinding, *noiseScaleBuffer);

    for (int i = 0; i < 2; i++) {

        if (mCurrentShaderProgram->bindingTexture[i].has_value() && textures[current_texture_ids[i]].first != nullptr) {
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingTexture[i],
                                        *textures[current_texture_ids[i]].first);
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingTextureSampl[i],
                                        *textures[current_texture_ids[i]].second);
        }

        if (mCurrentShaderProgram->bindingMask[i].has_value() &&
            textures[current_texture_ids[2 + i]].first != nullptr) {
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingMask[i],
                                        *textures[current_texture_ids[2 + i]].first);
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingMaskSampl[i],
                                        *textures[current_texture_ids[2 + i]].second);
        }

        if (mCurrentShaderProgram->bindingBlend[i].has_value() &&
            textures[current_texture_ids[4 + i]].first != nullptr) {
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingBlend[i],
                                        *textures[current_texture_ids[4 + i]].first);
            llgl_cmdBuffer->SetResource(*mCurrentShaderProgram->bindingBlendSampl[i],
                                        *textures[current_texture_ids[4 + i]].second);
        }
    }

    llgl_cmdBuffer->Draw(3 * buf_vbo_num_tris, 0);
}

void Fast::GfxRenderingAPILLGL::Init() {
    LLGL::Report report;

    llgl_renderer = LLGL::RenderSystem::Load(mWindowBackend->mInitData.LLGL.desc, &report);

    if (!llgl_renderer) {
        auto a = report.GetText();
        SPDLOG_ERROR("Failed to load \"%s\" module. Falling back to \"Null\" device.\n", "OpenGL");
        printf("Reason for failure: %s", report.HasErrors() ? report.GetText() : "Unknown\n");
        llgl_renderer = LLGL::RenderSystem::Load("Null");
        if (!llgl_renderer) {
            SPDLOG_CRITICAL("Failed to load \"Null\" module. Exiting.\n");
            exit(1);
        }
    }

    LLGL::SwapChainDescriptor swapChainDesc;
    swapChainDesc.resolution = { 800, 400 };
    swapChainDesc.resizable = true;
    swapChainDesc.samples = CVarGetInteger(CVAR_MSAA_VALUE, 1);
    llgl_swapChain = llgl_renderer->CreateSwapChain(swapChainDesc, mWindowBackend->mInitData.LLGL.Window);
    llgl_swapChain->SetVsyncInterval(0);

    llgl_cmdBuffer = llgl_renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);
    framebuffers.push_back({ llgl_swapChain, 0 });
    LLGL::BufferDescriptor bufferDescFrameCount;
    {
        bufferDescFrameCount.bindFlags = LLGL::BindFlags::ConstantBuffer;
        bufferDescFrameCount.size = sizeof(int);
    }
    frameCountBuffer = llgl_renderer->CreateBuffer(bufferDescFrameCount, &frame_count);

    LLGL::BufferDescriptor bufferDescNoiseScale;
    {
        bufferDescNoiseScale.bindFlags = LLGL::BindFlags::ConstantBuffer;
        bufferDescNoiseScale.size = sizeof(float);
    }
    noiseScaleBuffer = llgl_renderer->CreateBuffer(bufferDescNoiseScale, &noise_scale);

    for (int linear : { false, true }) {
        for (int mirror_x : { G_TX_NOMIRROR, G_TX_MIRROR }) {
            for (int wrap_clamp_x : { G_TX_WRAP, G_TX_CLAMP }) {
                for (int mirror_y : { G_TX_NOMIRROR, G_TX_MIRROR }) {
                    for (int wrap_clamp_y : { G_TX_WRAP, G_TX_CLAMP }) {
                        int cms = wrap_clamp_x | mirror_x;
                        int cmt = wrap_clamp_y | mirror_y;
                        LLGL::SamplerDescriptor samplerDesc;
                        {
                            samplerDesc.addressModeU = gfx_cm_to_llgl(cms);
                            samplerDesc.addressModeV = gfx_cm_to_llgl(cmt);
                            samplerDesc.minFilter = linear ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
                            samplerDesc.magFilter = linear ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
                            samplerDesc.maxAnisotropy = CVarGetInteger(CVAR_ANISOTROPIC_FILTERING, 1);
                        }
                        samplers[{ linear, cms, cmt }] = llgl_renderer->CreateSampler(samplerDesc);
                    }
                }
            }
        }
    }
    LLGL::BufferDescriptor vboDesc;
    {
        vboDesc.bindFlags = LLGL::BindFlags::VertexBuffer;
        vboDesc.size = 256 * sizeof(float);
    }
    vertexBuffer = llgl_renderer->CreateBuffer(vboDesc);
    vertexBufferSize = 256;
}

void Fast::GfxRenderingAPILLGL::OnResize(void) {
}

void Fast::GfxRenderingAPILLGL::StartFrame(void) {
    llgl_cmdBuffer->Begin();
    current_framebuffer_id = -1;
}

void Fast::GfxRenderingAPILLGL::EndFrame(void) {
    // llgl_cmdBuffer->EndRenderPass();
    // llgl_cmdBuffer->End();
    // llgl_swapChain->Present();
}

void Fast::GfxRenderingAPILLGL::FinishRender(void) {
    llgl_cmdBuffer->EndRenderPass();
    llgl_cmdBuffer->End();
    llgl_swapChain->Present();
    for (auto& buffer : garbage_collection_buffers) {
        if (buffer != nullptr) {
            llgl_renderer->Release(*buffer);
            buffer = nullptr;
        }
    }
    garbage_collection_buffers.clear();
}

int Fast::GfxRenderingAPILLGL::CreateFramebuffer(void) {
    textures.resize(textures.size() + 1);
    textures[textures.size() - 1].first = nullptr;
    textures[textures.size() - 1].second = nullptr;
    int texture_id = (int)(textures.size() - 1);
    int fb_id = (int)framebuffers.size();
    framebuffers.resize(framebuffers.size() + 1);
    framebuffers[fb_id] = { nullptr, texture_id };
    return fb_id;
}

void Fast::GfxRenderingAPILLGL::UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height,
                                                            bool opengl_invert_y, bool render_target,
                                                            bool has_depth_buffer, bool can_extract_depth) {
    if (fb_id == 0) {
        llgl_swapChain->ResizeBuffers({ width, height });
        return;
    }
    if (fb_id < 0 || fb_id >= (int)framebuffers.size()) {
        SPDLOG_ERROR("Invalid framebuffer ID: {}", fb_id);
        return;
    }
    if (framebuffers[fb_id].first != nullptr) {
        llgl_renderer->Release(*framebuffers[fb_id].first);
        framebuffers[fb_id].first = nullptr;
    }
    if (textures[framebuffers[fb_id].second].first != nullptr) {
        llgl_renderer->Release(*textures[framebuffers[fb_id].second].first);
        textures[framebuffers[fb_id].second].first = nullptr;
        textures[framebuffers[fb_id].second].second = nullptr;
    }
    LLGL::TextureDescriptor texDesc;
    {
        texDesc.type = LLGL::TextureType::Texture2D;
        texDesc.format = LLGL::Format::RGBA8UNorm;
        texDesc.extent = { width, height, 1 };
    }

    LLGL::Texture* texture = llgl_renderer->CreateTexture(texDesc);

    textures[framebuffers[fb_id].second].first = texture;
    textures[framebuffers[fb_id].second].second =
        samplers[{ false, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP }];

    LLGL::RenderTargetDescriptor renderTargetDesc;
    {
        renderTargetDesc.resolution = { width, height };
        renderTargetDesc.colorAttachments[0] = texture;
        if (has_depth_buffer) {
            LLGL::TextureDescriptor depthTexDesc;
            {
                depthTexDesc.bindFlags = LLGL::BindFlags::DepthStencilAttachment;
                depthTexDesc.format = LLGL::Format::D32Float;
                depthTexDesc.extent.width = width;
                depthTexDesc.extent.height = height;
                depthTexDesc.mipLevels = 1;
                depthTexDesc.type =
                    (depthTexDesc.samples > 1 ? LLGL::TextureType::Texture2DMS : LLGL::TextureType::Texture2D);
                depthTexDesc.miscFlags = LLGL::MiscFlags::NoInitialData;
            }
            LLGL::Texture* depthTexture = llgl_renderer->CreateTexture(depthTexDesc);
            renderTargetDesc.depthStencilAttachment = depthTexture;
        }
    }

    LLGL::RenderTarget* renderTarget = llgl_renderer->CreateRenderTarget(renderTargetDesc);
    framebuffers[fb_id].first = renderTarget;
}

void Fast::GfxRenderingAPILLGL::StartDrawToFramebuffer(int fb_id, float noise_scale) {
    if (fb_id < 0 || fb_id >= (int)framebuffers.size() || framebuffers[fb_id].first == nullptr) {
        SPDLOG_ERROR("Invalid framebuffer ID: {}", fb_id);
        return;
    }
    this->noise_scale = noise_scale;
    if (fb_id == current_framebuffer_id) {
        // Already drawing to the same framebuffer, no need to reset
        return;
    }
    if (current_framebuffer_id != -1) {
        llgl_cmdBuffer->EndRenderPass();
    }
    current_framebuffer_id = fb_id;
    if (fb_id == 0) {
        llgl_cmdBuffer->BeginRenderPass(*llgl_swapChain);
    } else {
        // Set the noise scale for the shader
        llgl_cmdBuffer->BeginRenderPass(*framebuffers[fb_id].first);
    }
    // llgl_cmdBuffer->SetViewport(framebuffers[fb_id].first->GetResolution());
}

void Fast::GfxRenderingAPILLGL::CopyFramebuffer(int fb_dst_id, int fb_src_id, int srcX0, int srcY0, int srcX1,
                                                int srcY1, int dstX0, int dstY0, int dstX1, int dstY1) {
    if (fb_dst_id < 0 || fb_dst_id >= (int)framebuffers.size() || framebuffers[fb_dst_id].first == nullptr ||
        fb_src_id < 0 || fb_src_id >= (int)framebuffers.size() || framebuffers[fb_src_id].first == nullptr) {
        SPDLOG_ERROR("Invalid framebuffer ID: {} or {}", fb_dst_id, fb_src_id);
        return;
    }
    // probably wrong
    const LLGL::TextureLocation location({ dstX0, dstY0, 0 });
    if (fb_src_id == current_framebuffer_id) {
        const LLGL::TextureRegion dstRegion(
            { 0, 0, 0 }, { static_cast<uint32_t>(dstX1 - dstX0), static_cast<uint32_t>(dstY1 - dstY0), 0 });
        llgl_cmdBuffer->CopyTextureFromFramebuffer(*textures[framebuffers[fb_dst_id].second].first, dstRegion,
                                                   { 0, 0 });
    } else {
        llgl_cmdBuffer->CopyTexture(*textures[framebuffers[fb_dst_id].second].first, location,
                                    *textures[framebuffers[fb_src_id].second].first, location,
                                    { (uint32_t)(dstX1 - dstX0), (uint32_t)(dstY1 - dstY0), 0 });
    }
}

void Fast::GfxRenderingAPILLGL::ClearFramebuffer(bool color, bool depth) {
    int flags = 0 | (color ? LLGL::ClearFlags::Color : 0) | (depth ? LLGL::ClearFlags::Depth : 0);
    llgl_cmdBuffer->Clear(flags, LLGL::ClearValue{ 0.0f, 0.0f, 0.0f, 1.0f });
}

void Fast::GfxRenderingAPILLGL::ReadFramebufferToCPU(int fb_id, uint32_t width, uint32_t height, uint16_t* rgba16_buf) {
    LLGL::MutableImageView rgba16_view(LLGL::ImageFormat::RGBA, LLGL::DataType::UInt8, rgba16_buf, width * height * 4);
    LLGL::TextureDescriptor texDesc;
    {
        texDesc.type = LLGL::TextureType::Texture2D;
        texDesc.bindFlags = LLGL::BindFlags::CopyDst;
        texDesc.format = LLGL::Format::RGBA8UNorm;
        texDesc.extent.width = width;
        texDesc.extent.height = height;
    }
    LLGL::Texture* texture = llgl_renderer->CreateTexture(texDesc);
    llgl_cmdBuffer->CopyTextureFromFramebuffer(*texture, LLGL::TextureRegion({ 0, 0, 0 }, { width, height, 1 }),
                                               { 0, 0 });
    llgl_renderer->ReadTexture(*texture, LLGL::TextureRegion({ 0, 0, 0 }, { width, height, 1 }), rgba16_view);
    // llgl_renderer->Release(*texture);
}

void Fast::GfxRenderingAPILLGL::ResolveMSAAColorBuffer(int fb_id_target, int fb_id_source) {
}

std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
Fast::GfxRenderingAPILLGL::GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) {
    return {};
}

ImTextureID Fast::GfxRenderingAPILLGL::GetFramebufferTextureId(int fb_id) {
    // TODO: not finish
    return nullptr;
    switch (llgl_renderer->GetRendererID()) {
        case LLGL::RendererID::OpenGL:
            LLGL::OpenGL::ResourceNativeHandle native_handle;
            textures[framebuffers[fb_id].second].first->GetNativeHandle(&native_handle, sizeof(native_handle));
            return (void*)native_handle.id;
#if LLGL_BUILD_RENDERER_VULKAN
        case LLGL::RendererID::Vulkan:
            LLGL::VKTexture* vk_texture = static_cast<LLGL::VKTexture*>(textures[framebuffers[fb_id].second].first);
            // return ImGui_ImplVulkan_AddTexture(vk_texture->Get, vk_texture->GetVkImageView(),
            //                                    vk_texture->GetVkImageLayout());
#endif
    }
    return nullptr;
}

void Fast::GfxRenderingAPILLGL::SelectTextureFb(int fb_id) {
    int tile = 0;
    SelectTexture(tile, framebuffers[fb_id].second);
}

void Fast::GfxRenderingAPILLGL::DeleteTexture(uint32_t texture_id) {
    if (texture_id < textures.size()) {
        llgl_renderer->Release(*textures[texture_id].first);
        textures[texture_id].first = nullptr;
    } else {
        SPDLOG_ERROR("Tried to delete texture with id {}, but it does not exist.", texture_id);
    }
}

void Fast::GfxRenderingAPILLGL::SetTextureFilter(Fast::FilteringMode mode) {
    current_filter_mode = mode;
}

Fast::FilteringMode Fast::GfxRenderingAPILLGL::GetTextureFilter(void) {
    return current_filter_mode;
}

void Fast::GfxRenderingAPILLGL::SetSrgbMode(void) {
    srgb_mode = true;
}

ImTextureID Fast::GfxRenderingAPILLGL::GetTextureById(int id) {
    return nullptr; // TODO fix me
}

void Fast::GfxRenderingAPILLGL::SetMsaaLevel(uint32_t level) {
    // TODO: change dynamically the msaa level
};

void Fast::GfxRenderingAPILLGL::SetAnisotropicFilteringLevel(uint32_t level) {
    for (auto& sampler : samplers) {
        llgl_renderer->Release(*sampler.second);
        LLGL::SamplerDescriptor samplerDesc;
        {
            samplerDesc.addressModeU = gfx_cm_to_llgl(std::get<1>(sampler.first));
            samplerDesc.addressModeV = gfx_cm_to_llgl(std::get<2>(sampler.first));
            samplerDesc.minFilter =
                std::get<0>(sampler.first) ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
            samplerDesc.magFilter =
                std::get<0>(sampler.first) ? LLGL::SamplerFilter::Linear : LLGL::SamplerFilter::Nearest;
            samplerDesc.maxAnisotropy = level;
        }
        sampler.second = llgl_renderer->CreateSampler(samplerDesc);
    }
};

} // namespace Fast
