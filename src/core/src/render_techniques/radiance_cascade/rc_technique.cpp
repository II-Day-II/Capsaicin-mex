#include "rc_technique.h"
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
    rc_pingpong_textures[0] = gfxCreateTexture2D(gfx_, capsaicin.getWidth() / 2, capsaicin.getHeight() / 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
    rc_pingpong_textures[1] = gfxCreateTexture2D(
        gfx_, capsaicin.getWidth() / 2, capsaicin.getHeight() / 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
   
    return initKernel(capsaicin);
}

void RCTechnique::terminate() noexcept
{
    gfxDestroyProgram(gfx_, rc_program);
    gfxDestroyKernel(gfx_, rc_kernel);
    //gfxDestroyKernel(gfx_, rc_intermediate_kernel);
    //gfxDestroyKernel(gfx_, rc_finalize_kernel);
    gfxDestroyTexture(gfx_, rc_pingpong_textures[0]);
    gfxDestroyTexture(gfx_, rc_pingpong_textures[1]);
    if (!!debug_rc_program)
    {
        gfxDestroyProgram(gfx_, debug_rc_program);
        gfxDestroyKernel(gfx_, debug_rc_kernel);
    }
}


void RCTechnique::render([[maybe_unused]] CapsaicinInternal &capsaicin) noexcept
{
    // check for options change
    RenderOptions newOptions = convertOptions(capsaicin.getOptions());
    bool          recompile  = false;
    options                  = newOptions;
    if (recompile)
    {
        gfxDestroyProgram(gfx_, rc_program);
        gfxDestroyKernel(gfx_, rc_kernel);

        initKernel(capsaicin);
    }

    gfxProgramSetParameter(gfx_, rc_program, "o_CascadeTex", capsaicin.getAOVBuffer("rc_probes0"));
    gfxProgramSetParameter(
        gfx_, rc_program, "o_IntermediateCascade", capsaicin.getAOVBuffer("rc_intermediate"));
    gfxProgramSetParameter(gfx_, rc_program, "o_FinalCascade", capsaicin.getAOVBuffer("rc_final"));

    uint2 buffer_dimensions = uint2(capsaicin.getWidth(), capsaicin.getHeight());
    gfxProgramSetParameter(gfx_, rc_program, "g_BufferDimensions", buffer_dimensions);
    uint2 cascade_dimensions = buffer_dimensions / 2u;
    gfxProgramSetParameter(gfx_, rc_program, "g_CascadeTexDimensions", cascade_dimensions);

    gfxProgramSetParameter(gfx_, rc_program, "g_Scene", capsaicin.getAccelerationStructure());

    gfxProgramSetParameter(
        gfx_, rc_program, "g_TextureMaps", capsaicin.getTextures(), capsaicin.getTextureCount());

    gfxProgramSetParameter(gfx_, rc_program, "g_NearestSampler", capsaicin.getNearestSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_LinearSampler", capsaicin.getLinearSampler());
    gfxProgramSetParameter(gfx_, rc_program, "g_TextureSampler", capsaicin.getLinearSampler());

    gfxProgramSetParameter(gfx_, rc_program, "g_Depth", capsaicin.getAOVBuffer("VisibilityDepth"));
    gfxProgramSetParameter(
        gfx_, rc_program, "g_GeometryNormalBuffer", capsaicin.getAOVBuffer("GeometryNormal"));
    gfxProgramSetParameter(gfx_, rc_program, "g_VisibilityBuffer", capsaicin.getAOVBuffer("Visibility"));
    gfxProgramSetParameter(gfx_, rc_program, "g_IndexBuffer", capsaicin.getIndexBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_VertexBuffer", capsaicin.getVertexBuffer());

    gfxProgramSetParameter(gfx_, rc_program, "g_MeshBuffer", capsaicin.getMeshBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_InstanceBuffer", capsaicin.getInstanceBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_MaterialBuffer", capsaicin.getMaterialBuffer());
    gfxProgramSetParameter(gfx_, rc_program, "g_TransformBuffer", capsaicin.getTransformBuffer());

    gfxCommandBindKernel(gfx_, rc_kernel);

    for (int cascade_level = 5; cascade_level >= 0; cascade_level -= 1)
    {
        bool  ping_pong   = cascade_level % 2 == 0;
        uint2 probe_count =
            uint2(cascade_dimensions.x / (8 * 1 << cascade_level), cascade_dimensions.y / (8 * 1 << cascade_level));
        gfxProgramSetParameter(gfx_, rc_program, "g_cascadeId", cascade_level);
        gfxProgramSetParameter(gfx_, rc_program, "g_probesCount", probe_count);
        
        gfxProgramSetParameter(gfx_, rc_program, "g_lastCascade", rc_pingpong_textures[ping_pong ? 0 : 1]);
        gfxProgramSetParameter(gfx_, rc_program, "o_currentCascade", rc_pingpong_textures[ping_pong ? 1 : 0]);


        uint32_t const *thread_nums = gfxKernelGetNumThreads(gfx_, rc_kernel);
        uint32_t        x = thread_nums[0], y = thread_nums[1];
        uint32_t        thread_size_x = uint32_t(glm::ceil(cascade_dimensions.x / float(x)));
        uint32_t        thread_size_y = uint32_t(glm::ceil(cascade_dimensions.y / float(y)));


        gfxCommandDispatch(gfx_, thread_size_x, thread_size_y, 1); 
    }

    //gfxCommandBindKernel(gfx_, rc_intermediate_kernel);
    //gfxCommandDispatch(gfx_, buffer_dimensions.x / 8, buffer_dimensions.y / 8, 1);
    //gfxCommandCopyTexture(gfx_, capsaicin.getAOVBuffer("rc_intermediate"), capsaicin.getAOVBuffer("rc_probes"));
    
    //gfxCommandBindKernel(gfx_, rc_finalize_kernel);
    //gfxCommandDispatch(gfx_, buffer_dimensions.x / 8, buffer_dimensions.y / 8, 1);



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
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_CascadeTex", rc_pingpong_textures[0]);
        gfxCommandBindKernel(gfx_, debug_rc_kernel);
        gfxCommandDraw(gfx_, 3);
    }

}

RenderOptionList RCTechnique::getRenderOptions() noexcept 
{
    RenderOptionList newOptions;
    newOptions.emplace(RENDER_OPTION_MAKE(rc_cascade_count, options));
    newOptions.emplace(RENDER_OPTION_MAKE(rc_debug_cascade_stop, options));
    return newOptions;
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;
    RENDER_OPTION_GET(rc_cascade_count, newOptions, options);
    RENDER_OPTION_GET(rc_debug_cascade_stop, newOptions, options);
    return newOptions;
}

ComponentList RCTechnique::getComponents() const noexcept 
{
    ComponentList components;
    return components;
}

AOVList RCTechnique::getAOVs() const noexcept
{
    AOVList aovs;
    aovs.push_back({"VisibilityDepth", AOV::Read});
    aovs.push_back({"GeometryNormal", AOV::Read});
    aovs.push_back({"Visibility", AOV::Read});
    //aovs.push_back({"rc_probes", AOV::Write, AOV::Clear, DXGI_FORMAT_R8G8B8A8_UNORM});
    aovs.push_back({"rc_probes0", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT});
    aovs.push_back({"rc_probes1", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT});
    aovs.push_back({"rc_intermediate", AOV::ReadWrite, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT});
    aovs.push_back({"rc_final", AOV::Write, AOV::Clear, DXGI_FORMAT_R16G16B16A16_FLOAT});
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

}

bool RCTechnique::initKernel(CapsaicinInternal const& capsaicin) noexcept
{
    rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/radiance_cascades", capsaicin.getShaderPath());
    std::vector<char const *> defines;

    rc_kernel = gfxCreateComputeKernel(
        gfx_, rc_program, "PingPongCascades", defines.data(), (uint32_t)defines.size()); // TODO: put entry point name here

    //rc_intermediate_kernel = gfxCreateComputeKernel(gfx_, rc_program, "MergeCascades", defines.data(), (uint32_t)defines.size());

    //rc_finalize_kernel = gfxCreateComputeKernel(gfx_, rc_program, "AintNoWayINeedAnotherPass", defines.data(), (uint32_t)defines.size());
    return !!rc_program;
}


}
