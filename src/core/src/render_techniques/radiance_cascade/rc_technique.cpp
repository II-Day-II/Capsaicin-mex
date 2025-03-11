#include "rc_technique.h"
#include "components/brdf_lut/brdf_lut.h"
#include "components/light_sampler_grid_stream/light_sampler_grid_stream.h"

#include "capsaicin_internal.h"

namespace Capsaicin
{
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
    gfxDestroyKernel(gfx_, rc_kernel_preavg);
    
    gfxDestroyProgram(gfx_, minmax_depth_program);
    gfxDestroyKernel(gfx_, minmax_depth_kernel);

    if (!!debug_rc_program)
    {
        gfxDestroyProgram(gfx_, debug_rc_program);
        gfxDestroyKernel(gfx_, debug_rc_kernel);
    }

    gfxDestroyTexture(gfx_, rc_probes[0]);
    gfxDestroyTexture(gfx_, rc_probes[1]);
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
    if (options.rc_downscale != newOptions.rc_downscale)
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            gfxDestroyTexture(gfx_, rc_probes[i]);
            rc_probes[i] = gfxCreateTexture2D(gfx_, 1920 >> newOptions.rc_downscale,
                1080 >> newOptions.rc_downscale, DXGI_FORMAT_R16G16B16A16_FLOAT);
        }
    }

    options                  = newOptions;

    // resize minmax depth if necessary
    if (minmax_depth.getWidth() != capsaicin.getWidth() || minmax_depth.getHeight() != capsaicin.getHeight())
    {
        gfxDestroyTexture(gfx_, minmax_depth);
        minmax_depth = gfxCreateTexture2D(
            gfx_, capsaicin.getWidth(), capsaicin.getHeight(), DXGI_FORMAT_R32G32_FLOAT, 7u);
    }
    
    { // clear textures
        gfxCommandClearTexture(gfx_, rc_probes[0]);
        gfxCommandClearTexture(gfx_, rc_probes[1]);
        gfxCommandClearTexture(gfx_, minmax_depth);
    }

    auto brdf_lut = capsaicin.getComponent<BrdfLut>();
    //auto light_sampler = capsaicin.getComponent<LightSamplerGridStream>();

    uint2 buffer_dimensions = uint2(capsaicin.getWidth(), capsaicin.getHeight());

    // TODO: move these things to render settings so ui can change them
    uint cascade_count = capsaicin.getOption<int>("rc_cascade_count");
    gfxProgramSetParameter(gfx_, rc_program, "g_numCascades", cascade_count);
    float c0_length = capsaicin.getOption<float>("rc_c0_length");
    gfxProgramSetParameter(gfx_, rc_program, "g_c0_length", c0_length);
    
    // get min/max depths
    {
        gfxCommandBindKernel(gfx_, minmax_depth_kernel);
        gfxProgramSetParameter(gfx_, minmax_depth_program, "g_Depth", capsaicin.getAOVBuffer("VisibilityDepth"));
        TimedSection minmax_depth_timer(*this, "min_max_depth");
        for (uint i = 0; i <= cascade_count; i++)
        {
            //gfxProgramSetParameter(gfx_, minmax_depth_program, "o_MinMaxDepth", capsaicin.getAOVBuffer("rc_MinMaxDepth"), i);
            gfxProgramSetParameter(gfx_, minmax_depth_program, "o_MinMaxDepth", minmax_depth, i);
            gfxProgramSetParameter(gfx_, minmax_depth_program, "g_mip_level", i);
            gfxCommandDispatch(gfx_, (uint32_t)glm::ceil((buffer_dimensions.x >> i) / 8.0), (uint32_t)glm::ceil((buffer_dimensions.y >> i) / 8.0), 1);
        }
    }



    gfxProgramSetParameter(gfx_, rc_program, "g_BufferDimensions", buffer_dimensions);
    uint2 cascade_dimensions = buffer_dimensions / 2u;
    //cascade_dimensions.y = capsaicin.getAOVBuffer("rc_probes0").getHeight();
    //cascade_dimensions.x = capsaicin.getAOVBuffer("rc_probes0").getWidth();
    cascade_dimensions.y = rc_probes[0].getHeight();
    cascade_dimensions.x = rc_probes[0].getWidth();
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
    //gfxProgramSetParameter(gfx_, rc_program, "g_MinMaxDepth", capsaicin.getAOVBuffer("rc_MinMaxDepth"));
    gfxProgramSetParameter(gfx_, rc_program, "g_MinMaxDepth", minmax_depth);

    gfxCommandBindKernel(gfx_, options.rc_do_preaveraging ? rc_kernel_preavg : rc_kernel);

    {
        TimedSection rc_probes_timer(*this, "render_cascades");

        float far_z = capsaicin.getCamera().farZ;
        float near_z = capsaicin.getCamera().nearZ;
        gfxProgramSetParameter(gfx_, rc_program, "g_near", near_z);
        gfxProgramSetParameter(gfx_, rc_program, "g_far", far_z);

        uint32_t const *thread_nums = gfxKernelGetNumThreads(gfx_, rc_kernel);
        uint32_t        x = thread_nums[0], y = thread_nums[1];
        uint32_t        thread_size_x = uint32_t(glm::ceil(cascade_dimensions.x / float(x)));
        uint32_t        thread_size_y = uint32_t(glm::ceil(cascade_dimensions.y / float(y)));

        for (int cascade_level = cascade_count; cascade_level >= 0; cascade_level -= 1)
        {
            gfxProgramSetParameter(gfx_, rc_program, "g_cascadeId", cascade_level);
            bool  ping_pong   = cascade_level % 2 == 0;
            //gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade", capsaicin.getAOVBuffer(ping_pong ? "rc_probes0" : "rc_probes1"));
            //gfxProgramSetParameter(gfx_, rc_program, "o_currentCascade", capsaicin.getAOVBuffer(ping_pong ? "rc_probes1" : "rc_probes0"));
            gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade", rc_probes[ping_pong ? 0 : 1]);
            gfxProgramSetParameter(gfx_, rc_program, "o_currentCascade", rc_probes[ping_pong ? 1 : 0]);
            
            gfxCommandDispatch(gfx_, thread_size_x, thread_size_y, 1); 
        }

    }
    // something happens to the aov here ??? incorrect sync??? gfxPlease????
    {
        TimedSection resolve(*this, "ResolveRCGI");
        gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getAnisotropicSampler());
        //gfxProgramSetParameter(gfx_, rc_program, "g_IrradianceBuffer", capsaicin.getAOVBuffer("rc_probes1")); // TODO: this is always going to be correct, but damn it looks hardcoded
        gfxProgramSetParameter(gfx_, rc_program, "g_IrradianceBuffer", rc_probes[1]); // TODO: this is always going to be correct, but damn it looks hardcoded
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
    newOptions.emplace(RENDER_OPTION_MAKE(rc_do_preaveraging, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_downscale, options));
    return newOptions;
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;
    RENDER_OPTION_GET(rc_cascade_count, newOptions, options);
    RENDER_OPTION_GET(rc_c0_length, newOptions, options);
    RENDER_OPTION_GET(rc_do_preaveraging, newOptions, options);
    RENDER_OPTION_GET(rc_downscale, newOptions, options);
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
    //aovs.push_back({"rc_probes", AOV::Write, AOV::Clear, DXGI_FORMAT_R8G8B8A8_UNORM});
    //aovs.push_back({"rc_probes0", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1920/1, 1080/1}); // TODO: runtime variable resolution? Make these local textures instead of AOVs, they aren't shared.
    //aovs.push_back({"rc_probes1", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1920/1, 1080/1});
    //aovs.push_back({"rc_MinMaxDepth", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R32G32_FLOAT, 7, 1920, 1080});
   
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
    ImGui::Checkbox("Use preaveraging", &capsaicin.getOption<bool>("rc_do_preaveraging"));
    ImGui::SliderInt("Scaling factor", &capsaicin.getOption<int>("rc_downscale"), 0, 4);
}

bool RCTechnique::initKernel(CapsaicinInternal const& capsaicin) noexcept
{
    rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/radiance_cascades", capsaicin.getShaderPath());
    std::vector<char const *> defines;

    rc_kernel = gfxCreateComputeKernel(
        gfx_, rc_program, "TraceCascades", defines.data(), (uint32_t)defines.size()); 
    rc_kernel_preavg = gfxCreateComputeKernel(gfx_, rc_program, "TraceCascadesPreAvg", defines.data(), (uint32_t)defines.size());
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
    uint32_t probes_width = 1920 >> 0;
    uint32_t probes_height = 1080 >> 0;
    for (uint32_t i = 0; i < 2; i++)
    {
        rc_probes[i]    = gfxCreateTexture2D(gfx_, probes_width, probes_height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    }
    minmax_depth =
        gfxCreateTexture2D(gfx_, capsaicin.getWidth(), capsaicin.getHeight(), DXGI_FORMAT_R32G32_FLOAT, 7);

    return !!minmax_depth && !!rc_probes[0];
}


} // namespace Capsaicin
