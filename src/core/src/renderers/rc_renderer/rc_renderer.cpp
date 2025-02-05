/*

*/
#pragma once

#include "renderer.h"

#include "visibility_buffer/visibility_buffer.h"
#include "ssgi/ssgi.h"
#include "gi10/gi10.h"
#include "atmosphere/atmosphere.h"
#include "skybox/skybox.h"
#include "taa/update_history.h"
#include "taa/taa.h"
#include "tone_mapping/tone_mapping.h"

#include "radiance_cascade/rc_technique.h"

namespace Capsaicin
{
/* renderer using radiance cascades */
class RCRenderer 
	: public Renderer
	, public RendererFactory::Registrar<RCRenderer>
{
public:
	static constexpr std::string_view Name = "RC renderer";
	
	/*Must have empty constructor*/
	RCRenderer() noexcept {}

	/**
     * Sets up the required render techniques.
     * @param renderOptions The current global render options.
     * @return A list of all required render techniques in the order that they are required. The calling
     * function takes all ownership of the returned list.
     */
    std::vector<std::unique_ptr<RenderTechnique>> setupRenderTechniques(
		[[maybe_unused]] RenderOptionList const &renderOptions) noexcept override
	{
		std::vector<std::unique_ptr<RenderTechnique>> render_techniques;
		render_techniques.emplace_back(std::make_unique<VisibilityBuffer>());
        //render_techniques.emplace_back(std::make_unique<SSGI>());
        //render_techniques.emplace_back(std::make_unique<GI10>());
        render_techniques.emplace_back(std::make_unique<RCTechnique>());
        //render_techniques.emplace_back(std::make_unique<Atmosphere>());
        render_techniques.emplace_back(std::make_unique<Skybox>());
        render_techniques.emplace_back(std::make_unique<UpdateHistory>());
        render_techniques.emplace_back(std::make_unique<TAA>());
        render_techniques.emplace_back(std::make_unique<ToneMapping>());

		return render_techniques;
	}

private:
};
} // namespace Capsaicin
