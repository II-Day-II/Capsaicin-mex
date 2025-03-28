#include "dau_technique.h"

#include "capsaicin_internal.h"


namespace Capsaicin
{

DAUTechnique::DAUTechnique()
    : RenderTechnique("DAU technique") 
{};

DAUTechnique::~DAUTechnique()
{
    terminate();
}

bool DAUTechnique::init([[maybe_unused]] CapsaicinInternal const &capsaicin) noexcept
{
   
    return initTextures(capsaicin) && initKernel(capsaicin);
}

void DAUTechnique::terminate() noexcept
{
    gfxDestroyProgram(gfx_, dau_program);
    gfxDestroyKernel(gfx_, dau_kernel);
    gfxDestroyProgram(gfx_, blit_program);
    gfxDestroyKernel(gfx_, halfres_blit_kernel);
    gfxDestroyKernel(gfx_, debug_blit_kernel);
    gfxDestroyTexture(gfx_, dau_output_tex);
    gfxDestroyTexture(gfx_, halfres_tex);
}


void DAUTechnique::render([[maybe_unused]] CapsaicinInternal &capsaicin) noexcept
{
    // check for options change
    RenderOptions newOptions = convertOptions(capsaicin.getOptions());
    bool          recompile  = false;
    
    if (recompile)
    {
        terminate();

        initKernel(capsaicin);
    }
    if (dau_output_tex.getWidth() != capsaicin.getWidth() || dau_output_tex.getHeight() != capsaicin.getHeight())
    {
        gfxDestroyTexture(gfx_, dau_output_tex);
        gfxDestroyTexture(gfx_, halfres_tex);
        initTextures(capsaicin);
    }
    gfxCommandClearTexture(gfx_, dau_output_tex);
    gfxCommandClearTexture(gfx_, halfres_tex);
    gfxCommandClearTexture(gfx_, halfres_depth);

    gfxCommandBindKernel(gfx_, halfres_depth_blit_kernel);
    gfxProgramSetParameter(gfx_, blit_program, "ColorBuffer", capsaicin.getAOVBuffer("VisibilityDepth"));
    gfxCommandDraw(gfx_, 3);

    //gfxCommandCopyTexture(gfx_, halfres_tex, capsaicin.getAOVBuffer("GeometryNormal")); // TODO: this but allowing resizing...
    gfxCommandBindKernel(gfx_, halfres_blit_kernel);
    gfxProgramSetParameter(gfx_, blit_program, "ColorBuffer", capsaicin.getAOVBuffer("GeometryNormal")); // TODO: set target to halfres
    gfxCommandDraw(gfx_, 3);

    uint2 buffer_dimensions = uint2(capsaicin.getWidth(), capsaicin.getHeight());
  
    const uint32_t *wg_size = gfxKernelGetNumThreads(gfx_, dau_kernel);
    uint2            wg_sizes = uint2(wg_size[0], wg_size[1]);
    uint2           dispatch_size = glm::ceil(float2(buffer_dimensions) / float2(wg_sizes));

    {
        TimedSection dau_timer(*this, "DAU test");
        gfxCommandBindKernel(gfx_, dau_kernel);
        gfxProgramSetParameter(gfx_, dau_program, "g_BufferDimensions", buffer_dimensions);
        gfxProgramSetParameter(gfx_, dau_program, "g_DepthBuffer", capsaicin.getAOVBuffer("VisibilityDepth"));
        gfxProgramSetParameter(gfx_, dau_program, "g_HalfResDepthBuffer", halfres_depth);
        gfxProgramSetParameter(gfx_, dau_program, "g_GeometryNormalBuffer", capsaicin.getAOVBuffer("GeometryNormal"));
        gfxProgramSetParameter(gfx_, dau_program, "g_HalfResGeometryNormalBuffer", halfres_tex);
        gfxProgramSetParameter(gfx_, dau_program, "o_OutputBuffer", dau_output_tex);
        gfxProgramSetParameter(gfx_, dau_program, "g_NearestSampler", capsaicin.getNearestSampler());
        gfxProgramSetParameter(gfx_, dau_program, "g_LinearSampler", capsaicin.getLinearSampler());
        gfxProgramSetParameter(gfx_, dau_program, "g_near", capsaicin.getCamera().nearZ);
        gfxProgramSetParameter(gfx_, dau_program, "g_near", capsaicin.getCamera().farZ);
        gfxCommandDispatch(gfx_, dispatch_size.x, dispatch_size.y, 1);
    }

    if (capsaicin.getCurrentDebugView() == "DAUTest")
    {
        //gfxCommandCopyTexture(gfx_, capsaicin.getAOVBuffer("Debug"), dau_output_tex); // TODO: this but allowing different formats...
        gfxCommandBindKernel(gfx_, debug_blit_kernel);
        gfxProgramSetParameter(gfx_, blit_program, "ColorBuffer", dau_output_tex); 
        gfxCommandDraw(gfx_, 3);
    }

}

RenderOptionList DAUTechnique::getRenderOptions() noexcept 
{
    RenderOptionList newOptions;

    return newOptions;
}

DAUTechnique::RenderOptions DAUTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    RenderOptions newOptions;

    return newOptions;
}

ComponentList DAUTechnique::getComponents() const noexcept 
{
    ComponentList components;

    return components;
}

AOVList DAUTechnique::getAOVs() const noexcept
{
    AOVList aovs;
    aovs.push_back({"VisibilityDepth", AOV::Read}); 
    aovs.push_back({"GeometryNormal", AOV::Read});
    aovs.push_back({"Visibility", AOV::Read});
   
    return aovs;
}

DebugViewList DAUTechnique::getDebugViews() const noexcept
{
    DebugViewList dbvs;
    dbvs.push_back("DAUTest");
    return dbvs;
}

void DAUTechnique::renderGUI([[maybe_unused]] CapsaicinInternal &capsaicin) const noexcept 
{

}

bool DAUTechnique::initKernel(CapsaicinInternal const& capsaicin) noexcept
{
    dau_program = gfxCreateProgram(gfx_, "render_techniques/dau_test/dau_test", capsaicin.getShaderPath());
    std::vector<char const *> defines;

    dau_kernel = gfxCreateComputeKernel(
        gfx_, dau_program, "DepthAwareUpscalingTest", defines.data(), (uint32_t)defines.size()); 

    blit_program = gfxCreateProgram(gfx_, "render_techniques/dau_test/blit", capsaicin.getShaderPath());
    // THIS needs to be done later, when the textures actually exist...
    GfxDrawState halfres_draw_state;
    gfxDrawStateSetColorTarget(halfres_draw_state, 0, halfres_tex);
    halfres_blit_kernel  = gfxCreateGraphicsKernel(gfx_, blit_program, halfres_draw_state);
    GfxDrawState halfres_depth_state;
    gfxDrawStateSetColorTarget(halfres_depth_state, 0, halfres_depth);
    halfres_depth_blit_kernel = gfxCreateGraphicsKernel(gfx_, blit_program, halfres_depth_state);
    GfxDrawState debug_draw_state;
    gfxDrawStateSetColorTarget(debug_draw_state, 0, capsaicin.getAOVBuffer("Debug"));
    debug_blit_kernel = gfxCreateGraphicsKernel(gfx_, blit_program, debug_draw_state);


    return !!dau_program && !!blit_program;
}

bool DAUTechnique::initTextures(CapsaicinInternal const& capsaicin) noexcept
{
    capsaicin;
    dau_output_tex = gfxCreateTexture2D(gfx_, capsaicin.getWidth(), capsaicin.getHeight(), capsaicin.getAOVBuffer("GeometryNormal").getFormat());
    halfres_tex = gfxCreateTexture2D(gfx_, capsaicin.getWidth() / 2, capsaicin.getHeight() / 2, capsaicin.getAOVBuffer("GeometryNormal").getFormat());
    halfres_depth = gfxCreateTexture2D(gfx_, capsaicin.getWidth() / 2, capsaicin.getHeight() / 2, capsaicin.getAOVBuffer("VisibilityDepth").getFormat());
    return !!dau_output_tex && !!halfres_tex;
}


} // namespace Capsaicin
