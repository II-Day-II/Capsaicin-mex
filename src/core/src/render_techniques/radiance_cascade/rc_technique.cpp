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
    return true;
}

void RCTechnique::terminate() noexcept
{

}

void RCTechnique::render([[maybe_unused]] CapsaicinInternal &capsaicin) noexcept 
{

}

RenderOptionList RCTechnique::getRenderOptions() noexcept 
{
    return {};
}

RCTechnique::RenderOptions RCTechnique::convertOptions(
    [[maybe_unused]] RenderOptionList const &options) noexcept
{
    return {};
}

ComponentList RCTechnique::getComponents() const noexcept 
{
    return {};
}

AOVList RCTechnique::getAOVs() const noexcept
{
    return {};
}

void RCTechnique::renderGUI([[maybe_unused]] CapsaicinInternal &capsaicin) const noexcept 
{

}

}
