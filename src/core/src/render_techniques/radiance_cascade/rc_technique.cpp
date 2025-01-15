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

}

void RCTechnique::render([[maybe_unused]] CapsaicinInternal &capsaicin) noexcept 
{

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
        gfx_, rc_program, "RenderAndMergeCascade", defines.data(), defines.size()); // TODO: put entry point name here
    return !!rc_program;
}


}
