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
    return initKernel(capsaicin);
}

void RCTechnique::terminate() noexcept
{
    gfxDestroyProgram(gfx_, rc_program);
    gfxDestroyKernel(gfx_, rc_kernel);
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

    uint2 buffer_dimensions = uint2(capsaicin.getWidth(), capsaicin.getHeight());
    gfxProgramSetParameter(gfx_, rc_program, "g_BufferDimensions", buffer_dimensions);
    gfxProgramSetParameter(gfx_, rc_program, "g_Depth", capsaicin.getAOVBuffer("Depth"));
    gfxProgramSetParameter(gfx_, rc_program, "o_CascadeTex", capsaicin.getAOVBuffer("rc_probes"));

    gfxCommandBindKernel(gfx_, rc_kernel);

    for (int cascade_level = 5; cascade_level >= 0; cascade_level -= 1)
    {
        uint2 probe_count =
            uint2(buffer_dimensions.x / (8 * 1 << cascade_level), buffer_dimensions.y / (8 * 1 << cascade_level));
        gfxProgramSetParameter(gfx_, rc_program, "g_cascadeId", cascade_level);
        gfxProgramSetParameter(gfx_, rc_program, "g_probesCount", probe_count);


        uint32_t const *thread_nums = gfxKernelGetNumThreads(gfx_, rc_kernel);
        uint32_t        x = thread_nums[0], y = thread_nums[1];
        uint32_t        thread_size_x = uint32_t(glm::ceil(buffer_dimensions.x / float(x)));
        uint32_t        thread_size_y = uint32_t(glm::ceil(buffer_dimensions.y / float(y * (1 << cascade_level))));


        gfxCommandDispatch(gfx_, thread_size_x, thread_size_y, 1); 
    }

    /*if (capsaicin.getCurrentDebugView() == "RCProbes")
    {
        if (!debug_rc_program)
        {
            debug_rc_program = gfxCreateProgram(gfx_, "render_techniques/radiance_cascade/debug_rc_probes", capsaicin.getShaderPath());
            GfxDrawState drawState;
            gfxDrawStateSetColorTarget(drawState, 0, capsaicin.getAOVBuffer("Debug"));
            debug_rc_kernel = gfxCreateGraphicsKernel(gfx_, debug_rc_program, drawState);
        }
        GfxCommandEvent const commandEvent(gfx_, "DrawDebugRCprobes");
        gfxProgramSetParameter(gfx_, debug_rc_program, "g_CascadeTex", capsaicin.getAOVBuffer("rc_probes"));
        gfxCommandBindKernel(gfx_, debug_rc_kernel);
        gfxCommandDraw(gfx_, 3);
    }*/

}

RenderOptionList RCTechnique::getRenderOptions() noexcept 
{
    RenderOptionList newOptions;
    return newOptions;
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;
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
    aovs.push_back({"Depth", AOV::Read});
    aovs.push_back({"ShadingNormal", AOV::Read});
    aovs.push_back({"rc_probes", AOV::Write, AOV::Clear, DXGI_FORMAT_R8G8B8A8_UNORM});
    return aovs;
}

DebugViewList RCTechnique::getDebugViews() const noexcept
{
    DebugViewList dbvs;
    //dbvs.push_back("RCProbes");
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
        gfx_, rc_program, "RenderCascades", defines.data(), (uint32_t)defines.size()); // TODO: put entry point name here
    return !!rc_program;
}


}
