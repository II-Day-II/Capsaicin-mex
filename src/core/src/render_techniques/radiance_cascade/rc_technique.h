#include "render_technique.h"
#include "gpu_shared.h"

namespace Capsaicin
{
class RCTechnique
	: public RenderTechnique
{
public:
	RCTechnique();
	~RCTechnique();
    /*
     * Gets configuration options for current technique.
     * @return A list of all valid configuration options.
     */
    RenderOptionList getRenderOptions() noexcept override;
    enum PreAvgSetup 
    {
        PreAverage0 = 0,
        PreAverage4 = 1,
        PreAverage16 = 2,
        PreAverageSetupCount = 3,
    };
    struct RenderOptions
    {
        // TODO: put parameters here, e.g. min/max bounds
        int rc_cascade_count = 5;
        float rc_c0_length       = 0.01f;
        int rc_preaveraging      = PreAvgSetup::PreAverage16;
        int   rc_resolution_factor       = 0;
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
    GfxProgram    rc_program;
    GfxKernel     rc_kernel;
    GfxKernel     rc_kernel_preavg16;
    GfxKernel     rc_kernel_preavg4;
    GfxKernel     rc_average_kernel;
    GfxKernel     rc_resolve_kernel;

    struct MinMaxTexture
    {
        GfxTexture min;
        GfxTexture max;

        void setName(char const *name)
        {
            // this sucks.
            std::string maxname(name);
            std::string minname(name); 
            maxname += "_max";
            minname += "_min";
            min.setName(minname.c_str());
            max.setName(maxname.c_str());
        }
    };
    MinMaxTexture rc_probes[2];

    GfxProgram minmax_depth_program;
    GfxKernel  minmax_depth_kernel;
    GfxTexture minmax_depth;
    
    char const * const texNames[3] = {"RC_Probes_0", "RC_Probes_1", "RC_MinMaxDepth"};
    

    GfxKernel     debug_rc_kernel;
    GfxProgram    debug_rc_program;


};
}
