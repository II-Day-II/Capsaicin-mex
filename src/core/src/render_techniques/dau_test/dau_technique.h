#include "render_technique.h"
#include "gpu_shared.h"

namespace Capsaicin
{
class DAUTechnique
	: public RenderTechnique
{
public:
	DAUTechnique();
	~DAUTechnique();
    /*
     * Gets configuration options for current technique.
     * @return A list of all valid configuration options.
     */
    RenderOptionList getRenderOptions() noexcept override;
    
    struct RenderOptions
    {

    };

    /**
     * Convert render options to internal options format.
     * @param options Current render options.
     * @returns The options converted.
     */
    static RenderOptions convertOptions(RenderOptionList const &options) noexcept;

    /**
     * Gets a list of any shared components used by the current render technique.
     * @return A list of all supported components.
     */
    ComponentList getComponents() const noexcept override;

    /**
     * Gets the required list of AOVs needed for the current render technique.
     * @return A list of all required AOV buffers.
     */
    AOVList getAOVs() const noexcept override;

     /**
     * Gets a list of any debug views provided by the current render technique.
     * @return A list of all supported debug views.
     */
    DebugViewList getDebugViews() const noexcept override;

    /**
     * Initialise any internal data or state.
     * @note This is automatically called by the framework after construction and should be used to create
     * any required CPU|GPU resources.
     * @param capsaicin Current framework context.
     * @return True if initialisation succeeded, False otherwise.
     */
    bool init(CapsaicinInternal const &capsaicin) noexcept override;

    /**
     * Perform render operations.
     * @param [in,out] capsaicin The current capsaicin context.
     */
    void render(CapsaicinInternal &capsaicin) noexcept override;

    /**
     * Destroy any used internal resources and shutdown.
     */
    void terminate() noexcept override;

    /**
     * Render GUI options.
     * @param [in,out] capsaicin The current capsaicin context.
     */
    void renderGUI(CapsaicinInternal &capsaicin) const noexcept override;

protected:

    bool initKernel(CapsaicinInternal const &capsaicin) noexcept;
    bool initTextures(CapsaicinInternal const &capsaicin) noexcept;

    RenderOptions options;
    GfxProgram    dau_program;
    GfxKernel     dau_kernel;
    GfxProgram    blit_program;
    GfxKernel     halfres_blit_kernel;
    GfxKernel     debug_blit_kernel;

    GfxTexture halfres_tex;
    GfxTexture dau_output_tex;

};
}
