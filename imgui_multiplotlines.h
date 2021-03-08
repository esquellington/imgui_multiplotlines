//----------------------------------------------------------------
// ImGui::MultiPlotLines
//----------------------------------------------------------------
#ifndef IMGUI_MULTIPLOTLINES
#define IMGUI_MULTIPLOTLINES

#include <imgui/imgui.h>

namespace ImGui
{

// ImGui::MultiPlotLines
//
// Plot mutliple channels simultaneously using the same number of values and horizontal/vertical ranges
//
// Basic usage is similar to ImGui::PlotLines() and only requires:
// - Providing a get_value() function pointer that accepts and additional channel_idx parameter
// - Providing the number of channels
//
// See imgui_multiplotlines_demo.h for example advanced usage
void MultiPlotLines( const char* label,
                     float (*get_value)(void* data, int value_idx, int channel_idx),
                     void* data,
                     int num_values,
                     int num_channels,
                     // Default params
                     struct MultiPlotLines_Params* params = nullptr,
                     float scale_min = FLT_MAX,
                     float scale_max = FLT_MAX,
                     ImVec2 graph_size = ImVec2(0, 0) );

// ImGui::MultiPlotLines_Params struct
//
// Pass it to MultiPlotLines() to enable additional functionality,
// such as displaying a Legend, and enabling/tweaking UI elements:
// - All fields are read-only unless explicitly prefixed with RW_
// - Defaults turn off most features to mimic ImGui::PlotLines behaviour
// - Defaults are safe in all cases
//
struct MultiPlotLines_Params
{
    //---- Channels
    enum EConstants { cMaxChannels = 32 };
    typedef const char* gcn_fn_t ( const void* data, int channel_idx );
    typedef ImU32       gcc_fn_t ( const void* data, int channel_idx );
    typedef int         gcp_fn_t ( const void* data, int channel_idx );
    const gcn_fn_t* get_channel_name   = nullptr; //if undefined will generate 'C_%d' name
    const gcc_fn_t* get_channel_color  = nullptr; //if undefined will use default Palette[i] color
    const gcp_fn_t* get_channel_parent = nullptr; //if undefined will return -1 (no parent)
    bool  RW_HideChannel[cMaxChannels] = {};      //All channels are visible by default (hide == false)

    //-- Hover/Selection
    ImU32 SelectedColor         = 0xFFFFFFFF; //Selected channel(s) use this color (white, not present in default palette)
    int   RW_SelectedChannelIdx = -1;         //Updated by clicking on channel with button mapped to eMCA_SelectChannel
    int   RW_HoveredChannelIdx  = -1;         //Updated by hovering over channel in Plot or Legend

    //-- Plot
    float PlotDrawThickness     = 1.0f;  //Base thickness
    float SelectedDrawThickness = 1.0f;  //Additional thickness on selected channel
    float HoveredDrawThickness  = 1.0f;  //Additional thickness on hovered channel
    bool  HoveredDrawTooltip    = true;  //Draw tooltip for closest hovered channel+value
    bool  HoveredDrawValue      = false; //Draw circle at closest hovered value
    bool  bFilterUI             = false; //Enables additional Filter UI
    float RW_FilterAlpha        = 1.0f;  //Low pass filter: f_{i+1} = f_i + alpha*(v_{i+1}-f_i), alpha=1.0 means no filtering

    //-- Legend
    bool RW_ShowLegend    = false; //RW Enable to display interactive legend
    bool bLegendUI        = false; //Enables additional Legend UI (Hide/Show Legend and All/None channel visibility)
    int  LegendMaxColumns = 4;     //Number of columns to use, unless a smaller value results in the same number of rows

    //-- Mouse interaction
    enum EMouseClickAction { eMCA_None,             //Do nothing
                             eMCA_SelectChannel,    //Toggle channel selection on click
                             eMCA_ToggleChannel,    //Toggle channel show/hide on click
                             eMCA_ToggleChildren }; //Toggle subhierarchy show/hide on click
    int  PlotMCA[3]   = { eMCA_SelectChannel, eMCA_None, eMCA_None }; //Left,Right,Middle buttons
    int  LegendMCA[3] = { eMCA_SelectChannel, eMCA_ToggleChannel, eMCA_None };  //Left,Right,Middle buttons
};

} //namespace ImGui

#endif //IMGUI_MULTIPLOTLINES
