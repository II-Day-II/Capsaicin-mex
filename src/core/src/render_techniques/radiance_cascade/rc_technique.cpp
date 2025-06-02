#include "rc_technique.h"
#include "components/brdf_lut/brdf_lut.h"
#include "components/light_sampler_grid_stream/light_sampler_grid_stream.h"

#include "capsaicin_internal.h"

#include <bit> // For std::countl_zero
#include <format> // for string formatting in a reasonable manner

namespace Capsaicin
{

static constexpr uint nearest_pow_2(uint const n)
{
    #if 0
    return n;
    #else
    constexpr uint size       = sizeof(uint) * 8;
    uint const     next_pow_2 = (1 << (size - std::countl_zero(n)));
    uint const     prev_pow_2 = next_pow_2 >> 1;
    return (n - prev_pow_2) < (next_pow_2 - n) ? prev_pow_2 : next_pow_2;
    #endif
}

constexpr float LinearizeDepth(float const depth)
{
    float const n    = 0.1f;
    float const f    = 1e4f;
    float const ndcz = 2.0f * depth - 1.0f;
    return (2.0f * n * f) / (f + n - ndcz * (f - n));
}

RCTechnique::RCTechnique()
    : RenderTechnique("RC technique") 
{};

RCTechnique::~RCTechnique()
{
    terminate();
}

bool RCTechnique::init([[maybe_unused]] CapsaicinInternal const &capsaicin) noexcept
{
    return initKernel(capsaicin) && initTextures(capsaicin);
}

void RCTechnique::terminate() noexcept
{
    gfxDestroyProgram(gfx_, rc_program);
    gfxDestroyKernel(gfx_, rc_kernel);
    gfxDestroyKernel(gfx_, rc_kernel_preavg4);
    
    gfxDestroyProgram(gfx_, minmax_depth_program);
    gfxDestroyKernel(gfx_, minmax_depth_kernel);

#ifdef _DEBUG
    gfxDestroyProgram(gfx_, debug_rc_program);
    gfxDestroyKernel(gfx_, debug_rc_kernel);
#endif

    gfxDestroyTexture(gfx_, rc_probes[0].min);
    gfxDestroyTexture(gfx_, rc_probes[1].min);

    gfxDestroyTexture(gfx_, rc_probes[0].max);
    gfxDestroyTexture(gfx_, rc_probes[1].max);

    gfxDestroyTexture(gfx_, minmax_depth);
}


void RCTechnique::render([[maybe_unused]] CapsaicinInternal &capsaicin) noexcept
{
    // check for options change
    RenderOptions newOptions = convertOptions(capsaicin.getOptions());
    bool          recompile  = newOptions.rc_sphere_mapping != options.rc_sphere_mapping || newOptions.rc_minmax_probes != options.rc_minmax_probes || newOptions.rc_probe_placement != options.rc_probe_placement;
    
    if (recompile)
    {
        terminate();

        init(capsaicin);
    }

    int      rf       = newOptions.rc_resolution_factor;
    // resize probe textures if requested
    if (options.rc_resolution_factor != newOptions.rc_resolution_factor)
    {
        uint const w         = nearest_pow_2(capsaicin.getWidth());
        uint const h         = nearest_pow_2(capsaicin.getHeight());
        uint32_t newWidth = rf < 0 ? w >> -rf : w << rf;
        uint32_t newHeight = rf < 0 ? h >> -rf : h << rf;
        for (uint32_t i = 0; i < 2; i++)
        {
            gfxDestroyTexture(gfx_, rc_probes[i].min);
            gfxDestroyTexture(gfx_, rc_probes[i].max);
            rc_probes[i].min = gfxCreateTexture2D(gfx_, newWidth, newHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
            rc_probes[i].max = gfxCreateTexture2D(gfx_, newWidth, newHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
            rc_probes[i].setName(texNames[i]);
        }
    }

    gfxProgramSetParameter(gfx_, rc_program, "g_resolution_factor", rf);
    gfxProgramSetParameter(gfx_, rc_program, "g_depth_bias_scale", newOptions.rc_depth_bias_scale);

    // resize minmax depth if necessary
    if (minmax_depth.getWidth() != nearest_pow_2(capsaicin.getWidth()) || minmax_depth.getHeight() != nearest_pow_2(capsaicin.getHeight()) || options.rc_cascade_count != newOptions.rc_cascade_count)
    //if (minmax_depth.getWidth() != rc_probes[0].min.getWidth() || minmax_depth.getHeight() != rc_probes[0].min.getHeight() || options.rc_cascade_count != newOptions.rc_cascade_count)
    {
        gfxDestroyTexture(gfx_, minmax_depth);
        [[maybe_unused]]uint32_t mmdepth_width = nearest_pow_2(capsaicin.getWidth());
        [[maybe_unused]]uint32_t mmdepth_height = nearest_pow_2(capsaicin.getHeight());
        uint                      mip_count      = newOptions.rc_cascade_count + 3;
            //(uint)glm::floor(glm::log2(glm::max((float)rc_probes[0].min.getWidth(), (float)rc_probes[0].min.getHeight()))); // fuck it we ball, always get max mips
        //minmax_depth = gfxCreateTexture2D(gfx_, rc_probes[0].min.getWidth(), rc_probes[0].min.getHeight(), DXGI_FORMAT_R32G32_FLOAT, mip_count);
        minmax_depth = gfxCreateTexture2D(gfx_, mmdepth_width, mmdepth_height, DXGI_FORMAT_R32G32_FLOAT, mip_count);

        minmax_depth.setName(texNames[2]);
    }
    
    options                  = newOptions;

    { // clear textures
        gfxCommandClearTexture(gfx_, rc_probes[0].min);
        gfxCommandClearTexture(gfx_, rc_probes[1].min);
        gfxCommandClearTexture(gfx_, rc_probes[0].max);
        gfxCommandClearTexture(gfx_, rc_probes[1].max);
        gfxCommandClearTexture(gfx_, minmax_depth);
    }

    auto brdf_lut = capsaicin.getComponent<BrdfLut>();
    //auto light_sampler = capsaicin.getComponent<LightSamplerGridStream>();

    uint2 buffer_dimensions = uint2(capsaicin.getWidth(), capsaicin.getHeight());

    uint cascade_count = capsaicin.getOption<int>("rc_cascade_count");
    gfxProgramSetParameter(gfx_, rc_program, "g_numCascades", cascade_count);
    float c0_length = capsaicin.getOption<float>("rc_c0_length");
    gfxProgramSetParameter(gfx_, rc_program, "g_c0_length", c0_length);

    
    // get min/max depths
    {
        gfxCommandBindKernel(gfx_, minmax_depth_kernel);
        gfxProgramSetParameter(gfx_, minmax_depth_program, "g_Depth", capsaicin.getAOVBuffer("VisibilityDepth")); 
        gfxProgramSetParameter(gfx_, minmax_depth_program, "g_dst_dimensions", uint2(minmax_depth.getWidth(), minmax_depth.getHeight()));
        TimedSection minmax_depth_timer(*this, "min_max_depth");
        for (uint i = 0; i < minmax_depth.getMipLevels(); i++)
        {
            gfxProgramSetParameter(gfx_, minmax_depth_program, "o_MinMaxDepth", minmax_depth, i);
            gfxProgramSetParameter(gfx_, minmax_depth_program, "g_mip_level", i);
            uint2 dispatch_size = glm::ceil(
                float2(
                    minmax_depth.getWidth() >> i, 
                    minmax_depth.getHeight() >> i
                ) / float2(8.0f) // TODO: extract group size from kernel
            );
            gfxCommandDispatch(gfx_, dispatch_size.x, dispatch_size.y, 1);
        }
    }

    gfxProgramSetParameter(gfx_, rc_program, "g_VisibilityDepth", capsaicin.getAOVBuffer("VisibilityDepth"));

    gfxProgramSetParameter(gfx_, rc_program, "g_BufferDimensions", buffer_dimensions);
    uint2 cascade_dimensions = buffer_dimensions / 2u;
    cascade_dimensions.x = rc_probes[0].min.getWidth();
    cascade_dimensions.y = rc_probes[0].min.getHeight();
    gfxProgramSetParameter(gfx_, rc_program, "g_CascadeTexDimensions", cascade_dimensions); 

    
    
    // uniforms required by capsaicin utility code
    gfxProgramSetParameter(gfx_, rc_program, "g_Scene", capsaicin.getAccelerationStructure());

    gfxProgramSetParameter(
        gfx_, rc_program, "g_TextureMaps", capsaicin.getTextures(), capsaicin.getTextureCount());

    gfxProgramSetParameter(gfx_, rc_program, "g_NearestSampler", capsaicin.getNearestSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_LinearSampler", capsaicin.getLinearSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getLinearWrapSampler());


    gfxProgramSetParameter(
        gfx_, rc_program, "g_GeometryNormalBuffer", capsaicin.getAOVBuffer("GeometryNormal"));

    brdf_lut->addProgramParameters(capsaicin, rc_program);
    //light_sampler->addProgramParameters(capsaicin, rc_program);

    gfxProgramSetParameter(gfx_, rc_program, "g_VisibilityBuffer", capsaicin.getAOVBuffer("Visibility"));
    gfxProgramSetParameter(gfx_, rc_program, "g_IndexBuffer", capsaicin.getIndexBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_VertexBuffer", capsaicin.getVertexBuffer());

    gfxProgramSetParameter(gfx_, rc_program, "g_MeshBuffer", capsaicin.getMeshBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_InstanceBuffer", capsaicin.getInstanceBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_MaterialBuffer", capsaicin.getMaterialBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_TransformBuffer", capsaicin.getTransformBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_EnvironmentBuffer", capsaicin.getEnvironmentBuffer());



    gfxProgramSetParameter(
        gfx_, rc_program, "g_ViewProjectionInverse", capsaicin.getCameraMatrices(false).inv_view_projection);
    gfxProgramSetParameter(gfx_, rc_program, "g_ViewProjection", capsaicin.getCameraMatrices(false).view_projection);
    gfxProgramSetParameter(gfx_, rc_program, "g_Eye", capsaicin.getCamera().eye);
    

    // the min_max depth buffer
    gfxProgramSetParameter(gfx_, rc_program, "g_MinMaxDepth", minmax_depth);

    GfxKernel boundKernel;
    switch (options.rc_preaveraging)
    {
    case PreAverage4:
        {
        boundKernel = rc_kernel_preavg4;
        }
        break;
    case PreAverage0:
    default: 
        {
        boundKernel = rc_kernel;
        }
        break;
    }

    gfxCommandBindKernel(gfx_, boundKernel);
    
    int last_output_texture = 0;
    {
        TimedSection rc_probes_timer(*this, "render_cascades");

        float far_z = capsaicin.getCamera().farZ;
        float near_z = capsaicin.getCamera().nearZ;
        gfxProgramSetParameter(gfx_, rc_program, "g_near", near_z);
        gfxProgramSetParameter(gfx_, rc_program, "g_far", far_z);
        
        uint32_t const *thread_nums = gfxKernelGetNumThreads(gfx_, boundKernel); // TODO: use the actually bound kernel instead of assuming they all have the same group sizes
        uint32_t        x = thread_nums[0], y = thread_nums[1];
        uint32_t        thread_size_x = uint32_t(glm::ceil(cascade_dimensions.x / float(x)));
        uint32_t        thread_size_y = uint32_t(glm::ceil(cascade_dimensions.y / float(y)));
        int             cascade_rendering_stop = options.rc_cascade_range_only ? options.rc_cascade_range : 0;
        for (int cascade_level = cascade_count; cascade_level >= cascade_rendering_stop; cascade_level -= 1)
        {
            gfxProgramSetParameter(gfx_, rc_program, "g_cascadeId", cascade_level);
            last_output_texture = cascade_level % 2;

            gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade_min", rc_probes[last_output_texture].min);
            gfxProgramSetParameter(gfx_, rc_program, "o_currentCascade_min", rc_probes[1 - last_output_texture].min);

            gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade_max", rc_probes[last_output_texture].max);
            gfxProgramSetParameter(
                gfx_, rc_program, "o_currentCascade_max", rc_probes[1 - last_output_texture].max);
            
            gfxCommandDispatch(gfx_, thread_size_x, thread_size_y, 1); 
            last_output_texture = 1 - last_output_texture;
        }
    }

    if (!options.rc_skip_final_average) // the preavg16 kernel already has 1 probe per pixel
    {
        TimedSection rc_final_average_timer(*this, "average c0");
        gfxCommandBindKernel(gfx_, rc_average_kernel);
        
        gfxProgramSetParameter(gfx_, rc_program, "g_finalCascade_min", rc_probes[last_output_texture].min);
        gfxProgramSetParameter(gfx_, rc_program, "o_finalCascadeUpscaled_min", rc_probes[1 - last_output_texture].min);

        gfxProgramSetParameter(gfx_, rc_program, "g_finalCascade_max", rc_probes[last_output_texture].max);
        gfxProgramSetParameter(
            gfx_, rc_program, "o_finalCascadeUpscaled_max", rc_probes[1 - last_output_texture].max);

        uint32_t const *group_size = gfxKernelGetNumThreads(gfx_, rc_average_kernel);
        uint2 group_counts = glm::ceil(float2(cascade_dimensions) / float2(group_size[0], group_size[1]));
        gfxCommandDispatch(gfx_, group_counts.x, group_counts.y, 1);
        last_output_texture = 1 - last_output_texture;
    }
    // something happens to the texture here ??? incorrect sync??? gfxPlease????
    {
        TimedSection resolve(*this, "ResolveRCGI");
        gfxProgramSetParameter(gfx_, rc_program, "g_DepthBuffer", capsaicin.getAOVBuffer("VisibilityDepth"));
        gfxProgramSetParameter(gfx_, rc_program, "g_ShadingNormalBuffer", capsaicin.getAOVBuffer("ShadingNormal"));
        gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getAnisotropicSampler());
        gfxProgramSetParameter(gfx_, rc_program, "g_IrradianceBuffer", rc_probes[last_output_texture].min); // TODO: do i need to account for min/max on final cascade?
        gfxCommandBindKernel(gfx_, rc_resolve_kernel);
        gfxCommandDraw(gfx_, 3);
    }

    #if _DEBUG
    if (capsaicin.getCurrentDebugView() == "RCProbes")
    {
//        GfxCommandEvent const commandEvent(gfx_, "DrawDebugRCprobes"); // BUG: this just causes shader reloading to break for some reason
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_CascadeTex", rc_probes[last_output_texture].min);
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_nearestSampler", capsaicin.getNearestSampler());
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_buffer_dimensions", buffer_dimensions);
        gfxCommandBindKernel(gfx_, debug_rc_kernel);
        gfxCommandDraw(gfx_, 3);
    }
    #endif
}

RenderOptionList RCTechnique::getRenderOptions() noexcept 
{
    RenderOptionList newOptions;
    newOptions.emplace(RENDER_OPTION_MAKE(rc_cascade_count, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_cascade_range, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_c0_length, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_preaveraging, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_resolution_factor, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_cascade_range_only, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_skip_final_average, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_sphere_mapping, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_probe_placement, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_minmax_probes, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_depth_bias_scale, options));
    return newOptions;
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;
    RENDER_OPTION_GET(rc_cascade_count, newOptions, options);
    RENDER_OPTION_GET(rc_cascade_range, newOptions, options);
    RENDER_OPTION_GET(rc_c0_length, newOptions, options);
    RENDER_OPTION_GET(rc_preaveraging, newOptions, options);
    RENDER_OPTION_GET(rc_resolution_factor, newOptions, options);
    RENDER_OPTION_GET(rc_cascade_range_only, newOptions, options);
    RENDER_OPTION_GET(rc_skip_final_average, newOptions, options);
    RENDER_OPTION_GET(rc_sphere_mapping, newOptions, options);
    RENDER_OPTION_GET(rc_probe_placement, newOptions, options);
    RENDER_OPTION_GET(rc_minmax_probes, newOptions, options);
    RENDER_OPTION_GET(rc_depth_bias_scale, newOptions, options);
    return newOptions;
}

ComponentList RCTechnique::getComponents() const noexcept 
{
    ComponentList components;
    components.push_back(COMPONENT_MAKE(BrdfLut));
    //components.push_back(COMPONENT_MAKE(LightSamplerGridStream));
    return components;
}

AOVList RCTechnique::getAOVs() const noexcept
{
    AOVList aovs;
    aovs.push_back({"VisibilityDepth", AOV::Read}); // TODO: HELP: Do I need EVERY SINGLE TEXTURE to be pow2? Basically yes... ;_;
    aovs.push_back({"GeometryNormal", AOV::Read});
    aovs.push_back({"Visibility", AOV::Read});
   
    aovs.push_back({"GlobalIllumination", AOV::Write, AOV::None, DXGI_FORMAT_R16G16B16A16_FLOAT});
    /*aovs.push_back({.name = "Reflection",
        .access           = AOV::Write,
        .flags            = AOV::None,
        .format           = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .backup_name      = "PrevReflection"});*/

    aovs.push_back({.name = "ShadingNormal"});
    return aovs;
}

DebugViewList RCTechnique::getDebugViews() const noexcept
{
    DebugViewList dbvs;
    dbvs.push_back("RCProbes");
    return dbvs;
}

void RCTechnique::renderGUI([[maybe_unused]] CapsaicinInternal &capsaicin) const noexcept 
{
    ImGui::SliderInt("Num cascades", &capsaicin.getOption<int>("rc_cascade_count"), 0, 6);
    ImGui::Checkbox("Render cascade range only", &capsaicin.getOption<bool>("rc_cascade_range_only")); 
    if (capsaicin.getOption<bool>("rc_cascade_range_only"))
    {
        ImGui::SliderInt("Cascade range", &capsaicin.getOption<int>("rc_cascade_range"), 0, capsaicin.getOption<int>("rc_cascade_count"));
    };
    ImGui::Checkbox("Skip final average step", &capsaicin.getOption<bool>("rc_skip_final_average"));
    ImGui::SliderFloat(
        "C0 ray length", &capsaicin.getOption<float>("rc_c0_length"), 0.0000001f, 100.0f, "%.7f", ImGuiSliderFlags_Logarithmic);
    
    char const *preavg_labels[] = {"OFF(Debug)", "4"};
    ImGui::Combo("Use Preaveraging", &capsaicin.getOption<int>("rc_preaveraging"),
        preavg_labels, RCTechnique::PreAverageSetupCount);
    ImGui::SliderInt("Resolution factor", &capsaicin.getOption<int>("rc_resolution_factor"), -4, 1);
    char const *spheremap_labels[] = {"Cos-Theta", "Octahedral", "Octahedral (Equal Area)"};
    ImGui::Combo("Sphere Map", &capsaicin.getOption<int>("rc_sphere_mapping"), spheremap_labels, RCTechnique::SphereMapOptionCount);
    char const *probe_placement_labels[] = { "Min/Max depth", "Tile center" };
    ImGui::Checkbox("Min+Max probes", &capsaicin.getOption<bool>("rc_minmax_probes"));
    if (!capsaicin.getOption<bool>("rc_minmax_probes"))
    {
        ImGui::Combo("Probe depth-placement", &capsaicin.getOption<int>("rc_probe_placement"), probe_placement_labels, 2);
    }
    ImGui::SliderFloat("Depth Bias scale", &capsaicin.getOption<float>("rc_depth_bias_scale"), 1.0f, 10000.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
}

bool RCTechnique::initKernel(CapsaicinInternal const& capsaicin) noexcept
{
    rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/radiance_cascades", capsaicin.getShaderPath());
    std::vector<char const *> defines;
    const int spheremap_mode = capsaicin.getOption<int>("rc_sphere_mapping");
    auto define_smm     = std::format("SPHERE_MAP_MODE {}", spheremap_mode);
    defines.push_back(define_smm.c_str()); // this feels like a potential footgun
    const int probe_placement = capsaicin.getOption<int>("rc_probe_placement");
    auto define_pp       = std::format("PROBE_PLACEMENT {}", probe_placement);
    defines.push_back(define_pp.c_str()); // footgun?
    if (capsaicin.getOption<bool>("rc_minmax_probes"))
    {
        defines.push_back("MINMAX_MERGE 1");
    }
    else
    {
        defines.push_back("MINMAX_MERGE 0");
    }

    rc_kernel = gfxCreateComputeKernel(
        gfx_, rc_program, "TraceCascades", defines.data(), (uint32_t)defines.size()); 
    rc_kernel_preavg4 = gfxCreateComputeKernel(
        gfx_, rc_program, "TraceCascadesPreAvg4", defines.data(), (uint32_t)defines.size());
    rc_average_kernel = gfxCreateComputeKernel(
        gfx_, rc_program, "AverageFinalCascade", defines.data(), (uint32_t)defines.size());
    GfxDrawState resolve_draw_state;
    gfxDrawStateSetColorTarget(resolve_draw_state, 0, capsaicin.getAOVBuffer("GlobalIllumination"));

    rc_resolve_kernel =
        gfxCreateGraphicsKernel(gfx_, rc_program, resolve_draw_state, "ResolveRCGI", defines.data(), (uint32_t)defines.size());

    minmax_depth_program = gfxCreateProgram(
        gfx_, "render_techniques/radiance_cascade/downsample_depth", capsaicin.getShaderPath());
    minmax_depth_kernel =
        gfxCreateComputeKernel(gfx_, minmax_depth_program, "MinMaxDepth", defines.data(), (uint32_t)defines.size());

    // this is easier than trying to create it when needed
    #ifdef _DEBUG
    debug_rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/debug_rc_probes", capsaicin.getShaderPath());
    GfxDrawState drawState;
    gfxDrawStateSetColorTarget(drawState, 0, capsaicin.getAOVBuffer("Debug"));
    debug_rc_kernel = gfxCreateGraphicsKernel(gfx_, debug_rc_program, drawState);
    #endif

    return !!rc_program;
}

bool RCTechnique::initTextures(CapsaicinInternal const& capsaicin) noexcept
{
    constexpr uint32_t probes_width  = nearest_pow_2(1920);
    constexpr uint32_t probes_height = nearest_pow_2(1080);
    for (uint32_t i = 0; i < 2; i++)
    {
        rc_probes[i].min =
            gfxCreateTexture2D(gfx_, probes_width, probes_height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        rc_probes[i].max =
            gfxCreateTexture2D(gfx_, probes_width, probes_height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        rc_probes[i].setName(texNames[i]);
    }
    [[maybe_unused]]const uint32_t mmdepth_width = nearest_pow_2(capsaicin.getWidth());
    [[maybe_unused]]const uint32_t mmdepth_height = nearest_pow_2(capsaicin.getHeight());
    uint                            mip_count      = capsaicin.getOption<int>("rc_cascade_count") + 3;
        //(uint)glm::floor(glm::log2(glm::max((float)rc_probes[0].min.getWidth(), (float)rc_probes[0].min.getHeight()))); // fuck it we ball, always get max mips (BAD idea for perf)

    //minmax_depth = gfxCreateTexture2D(gfx_, capsaicin.getWidth(), capsaicin.getHeight(), DXGI_FORMAT_R32G32_FLOAT, mip_count);
    minmax_depth = gfxCreateTexture2D(gfx_, mmdepth_width, mmdepth_height, DXGI_FORMAT_R32G32_FLOAT, mip_count);
    minmax_depth.setName(texNames[2]);

    return !!minmax_depth && !!rc_probes[0].max;
}


} // namespace Capsaicin
