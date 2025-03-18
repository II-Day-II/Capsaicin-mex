#include "rc_technique.h"
#include "components/brdf_lut/brdf_lut.h"
#include "components/light_sampler_grid_stream/light_sampler_grid_stream.h"

#include "capsaicin_internal.h"

#include <bit> // For std::countl_zero

namespace Capsaicin
{

static constexpr uint nearest_pow_2(uint const n)
{
    constexpr uint size       = sizeof(uint) * 8;
    uint const     next_pow_2 = (1 << (size - std::countl_zero(n)));
    uint const     prev_pow_2 = next_pow_2 >> 1;
    return (n - prev_pow_2) < (next_pow_2 - n) ? prev_pow_2 : next_pow_2;
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
    gfxDestroyKernel(gfx_, rc_kernel_preavg16);
    
    gfxDestroyProgram(gfx_, minmax_depth_program);
    gfxDestroyKernel(gfx_, minmax_depth_kernel);

    if (!!debug_rc_program)
    {
        gfxDestroyProgram(gfx_, debug_rc_program);
        gfxDestroyKernel(gfx_, debug_rc_kernel);
    }

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
    bool          recompile  = false;
    
    if (recompile)
    {
        terminate();

        initKernel(capsaicin);
    }

    // resize probe textures if requested
    if (options.rc_resolution_factor != newOptions.rc_resolution_factor)
    {
        int      rf       = newOptions.rc_resolution_factor;
        constexpr uint const w         = nearest_pow_2(1920);
        constexpr uint const h         = nearest_pow_2(1080);
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

    options                  = newOptions;

    // resize minmax depth if necessary
    //if (minmax_depth.getWidth() != nearest_pow_2(capsaicin.getWidth()) || minmax_depth.getHeight() != nearest_pow_2(capsaicin.getHeight()))
    if (minmax_depth.getWidth() != capsaicin.getWidth() || minmax_depth.getHeight() != capsaicin.getHeight())
    {
        gfxDestroyTexture(gfx_, minmax_depth);
        [[maybe_unused]]uint32_t mmdepth_width = nearest_pow_2(capsaicin.getWidth());
        [[maybe_unused]]uint32_t mmdepth_height = nearest_pow_2(capsaicin.getHeight());
        minmax_depth = gfxCreateTexture2D(gfx_, capsaicin.getWidth(), capsaicin.getHeight(), DXGI_FORMAT_R32G32_FLOAT, 7u);
        //minmax_depth = gfxCreateTexture2D(gfx_, mmedpth_width, mmdepth_height, DXGI_FORMAT_R32G32_FLOAT, 7u);
        minmax_depth.setName(texNames[2]);
    }
    
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
        gfxProgramSetParameter(gfx_, minmax_depth_program, "g_Depth", capsaicin.getAOVBuffer("VisibilityDepth")); // TODO: this either needs to be the same size as the output or we need the shader to sample as though it were.
        TimedSection minmax_depth_timer(*this, "min_max_depth");
        for (uint i = 0; i <= cascade_count; i++)
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



    gfxProgramSetParameter(gfx_, rc_program, "g_BufferDimensions", buffer_dimensions);
    uint2 cascade_dimensions = buffer_dimensions / 2u;
    cascade_dimensions.y = rc_probes[0].min.getHeight();
    cascade_dimensions.x = rc_probes[0].min.getWidth();
    gfxProgramSetParameter(gfx_, rc_program, "g_CascadeTexDimensions", cascade_dimensions); 

    
    
    // uniforms required by capsaicin utility code
    gfxProgramSetParameter(gfx_, rc_program, "g_Scene", capsaicin.getAccelerationStructure());

    gfxProgramSetParameter(
        gfx_, rc_program, "g_TextureMaps", capsaicin.getTextures(), capsaicin.getTextureCount());

    gfxProgramSetParameter(gfx_, rc_program, "g_NearestSampler", capsaicin.getNearestSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_LinearSampler", capsaicin.getLinearSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getLinearWrapSampler());

    gfxProgramSetParameter(gfx_, rc_program, "g_Depth", capsaicin.getAOVBuffer("VisibilityDepth"));


    gfxProgramSetParameter(
        gfx_, rc_program, "g_GeometryNormalBuffer", capsaicin.getAOVBuffer("GeometryNormal"));
    gfxProgramSetParameter(
        gfx_, rc_program, "g_ShadingNormalBuffer", capsaicin.getAOVBuffer("ShadingNormal"));
    gfxProgramSetParameter(gfx_, rc_program, "g_DepthBuffer", capsaicin.getAOVBuffer("VisibilityDepth"));

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
    gfxProgramSetParameter(gfx_, rc_program, "g_Eye", capsaicin.getCamera().eye);
    

    // the min_max depth buffer
    gfxProgramSetParameter(gfx_, rc_program, "g_MinMaxDepth", minmax_depth);

    switch (options.rc_preaveraging)
    {
    case PreAverage4: gfxCommandBindKernel(gfx_, rc_kernel_preavg4); break;
    case PreAverage16: gfxCommandBindKernel(gfx_, rc_kernel_preavg16); break;
    case PreAverage0:
    default: gfxCommandBindKernel(gfx_, rc_kernel); break;
    }
    int last_output_texture = 0;
    {
        TimedSection rc_probes_timer(*this, "render_cascades");

        float far_z = capsaicin.getCamera().farZ;
        float near_z = capsaicin.getCamera().nearZ;
        gfxProgramSetParameter(gfx_, rc_program, "g_near", near_z);
        gfxProgramSetParameter(gfx_, rc_program, "g_far", far_z);
        
        uint32_t const *thread_nums = gfxKernelGetNumThreads(gfx_, rc_kernel); // TODO: use the actually bound kernel instead of assuming they all have the same group sizes
        uint32_t        x = thread_nums[0], y = thread_nums[1];
        uint32_t        thread_size_x = uint32_t(glm::ceil(cascade_dimensions.x / float(x)));
        uint32_t        thread_size_y = uint32_t(glm::ceil(cascade_dimensions.y / float(y)));

        for (int cascade_level = cascade_count; cascade_level >= 0; cascade_level -= 1)
        {
            gfxProgramSetParameter(gfx_, rc_program, "g_cascadeId", cascade_level);
            bool  ping_pong   = cascade_level % 2 == 0;

            gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade_min", rc_probes[ping_pong ? 0 : 1].min);
            gfxProgramSetParameter(gfx_, rc_program, "o_currentCascade_min", rc_probes[ping_pong ? 1 : 0].min);

            gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade_max", rc_probes[ping_pong ? 0 : 1].max);
            gfxProgramSetParameter(
                gfx_, rc_program, "o_currentCascade_max", rc_probes[ping_pong ? 1 : 0].max);
            
            last_output_texture = ping_pong ? 1 : 0;
            gfxCommandDispatch(gfx_, thread_size_x, thread_size_y, 1); 
        }

    }
    if (options.rc_preaveraging != PreAverage16) // the preavg16 kernel already has 1 probe per pixel
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
        gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getAnisotropicSampler());
        gfxProgramSetParameter(gfx_, rc_program, "g_IrradianceBuffer", rc_probes[last_output_texture].min); // TODO: do i need to account for min/max on final cascade?
        gfxCommandBindKernel(gfx_, rc_resolve_kernel);
        gfxCommandDraw(gfx_, 3);
    }

    if (capsaicin.getCurrentDebugView() == "RCProbes")
    {
        if (!debug_rc_program)
        {
            debug_rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/debug_rc_probes", capsaicin.getShaderPath());
            GfxDrawState drawState;
            gfxDrawStateSetColorTarget(drawState, 0, capsaicin.getAOVBuffer("Debug"));
            debug_rc_kernel = gfxCreateGraphicsKernel(gfx_, debug_rc_program, drawState);
        }
        GfxCommandEvent const commandEvent(gfx_, "DrawDebugRCprobes");
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_CascadeTex", rc_probes[1]);
        gfxCommandBindKernel(gfx_, debug_rc_kernel);
        gfxCommandDraw(gfx_, 3);
    }

}

RenderOptionList RCTechnique::getRenderOptions() noexcept 
{
    RenderOptionList newOptions;
    newOptions.emplace(RENDER_OPTION_MAKE(rc_cascade_count, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_c0_length, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_preaveraging, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_resolution_factor, options));
    return newOptions;
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;
    RENDER_OPTION_GET(rc_cascade_count, newOptions, options);
    RENDER_OPTION_GET(rc_c0_length, newOptions, options);
    RENDER_OPTION_GET(rc_preaveraging, newOptions, options);
    RENDER_OPTION_GET(rc_resolution_factor, newOptions, options);
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
    aovs.push_back({"VisibilityDepth", AOV::Read});
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
    ImGui::SliderFloat(
        "C0 ray length", &capsaicin.getOption<float>("rc_c0_length"), 0.0000001f, 1.0f, "%.7f", ImGuiSliderFlags_Logarithmic);
    
    char const *preavg_labels[] = {"OFF", "4", "16"};
    ImGui::Combo("Use Preaveraging", &capsaicin.getOption<int>("rc_preaveraging"),
        preavg_labels, RCTechnique::PreAverageSetupCount);
    ImGui::SliderInt("Resolution factor", &capsaicin.getOption<int>("rc_resolution_factor"), -4, 1);
}

bool RCTechnique::initKernel(CapsaicinInternal const& capsaicin) noexcept
{
    rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/radiance_cascades", capsaicin.getShaderPath());
    std::vector<char const *> defines;

    rc_kernel = gfxCreateComputeKernel(
        gfx_, rc_program, "TraceCascades", defines.data(), (uint32_t)defines.size()); 
    rc_kernel_preavg16 = gfxCreateComputeKernel(
        gfx_, rc_program, "TraceCascadesPreAvg16", defines.data(), (uint32_t)defines.size());
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

    
    return !!rc_program;
}

bool RCTechnique::initTextures(CapsaicinInternal const& capsaicin) noexcept
{
    capsaicin;
    constexpr uint32_t probes_width = nearest_pow_2(1920);
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
    minmax_depth = gfxCreateTexture2D(gfx_, capsaicin.getWidth(), capsaicin.getHeight(), DXGI_FORMAT_R32G32_FLOAT, 7);
    //minmax_depth = gfxCreateTexture2D(gfx_, mmdepth_width, mmdepth_height, DXGI_FORMAT_R32G32_FLOAT, 7);
    minmax_depth.setName(texNames[2]);

    return !!minmax_depth && !!rc_probes[0].max;
}


} // namespace Capsaicin
