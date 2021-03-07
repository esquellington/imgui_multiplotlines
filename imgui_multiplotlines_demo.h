//----------------------------------------------------------------
// ImGui::MultiPlotLines_Demo
//----------------------------------------------------------------
#ifndef IMGUI_MULTIPLOTLINES_DEMO
#define IMGUI_MULTIPLOTLINES_DEMO

#include "imgui_multiplotlines.h"
#include <math.h>

namespace ImGui
{

inline void MultiPlotLines_Demo()
{
    // Persistent parameters
    static ImGui::MultiPlotLines_Params params;
    static bool bInit(false);
    if(!bInit)
    {
        //enable interactive UI and Legend view
        params.bFilterUI = true;
        params.bLegendUI = true;
        params.RW_ShowLegend = true;
        bInit = true;
    }

    // MultiPlotLines function parameters
    static int num_channels(2), num_values(100);
    static bool bUseDefaultParams(false);
    if( ImGui::CollapsingHeader("MPL function params") )
    {
        ImGui::SliderInt("NumChannels",&num_channels,1,ImGui::MultiPlotLines_Params::cMaxChannels);
        ImGui::SliderInt("NumValues",&num_values,2,1000);
    }

    // MultiPlotLines_Params struct
    if( ImGui::CollapsingHeader("MPL struct params") )
    {
        // ImGui::Checkbox( "UseDefault", &bUseDefaultParams );
        if( !bUseDefaultParams )
        {
            ImGui::Checkbox( "bFilterUI", &params.bFilterUI );
            ImGui::Checkbox( "bLegendUI", &params.bLegendUI );
            ImGui::Checkbox( "HoveredDrawTooltip", &params.HoveredDrawTooltip );
            ImGui::Checkbox( "HoveredDrawValue", &params.HoveredDrawValue );
            ImGui::SliderInt("LegendMaxColumns",&params.LegendMaxColumns,1,10);
            ImGui::SliderFloat( "HoveredDrawThickness", &params.HoveredDrawThickness, 0.0f, 2.0f );
            ImGui::SliderFloat( "SelectedDrawThickness", &params.SelectedDrawThickness, 0.0f, 2.0f );
        }
    }

    // Adapt to window size
    const float width( ImGui::GetWindowWidth() - 2*ImGui::GetCursorPosX() );
    const float height( width / (4.0f/3.0f) ); // 4/3 aspect ratio
    ImGui::MultiPlotLines( "MPL Demo", //must have a non-null name
                           []( void* data, int slice_idx, int channel_idx )
                               {
                                   return sinf( 0.5f * 3.159265f * float(channel_idx+1) * float(slice_idx)/100.0f );
                               },
                           nullptr, //data
                           num_values, //values
                           num_channels, //channels
                           bUseDefaultParams ? nullptr : &params, //params (nullptr for default)
                           FLT_MAX, FLT_MAX, //ranges
                           ImVec2( width, height ) ); //sizes

    // Query RW_ params, only valid if params != nullptr
    if( ImGui::CollapsingHeader("MPL query RW_ params") )
    {
        ImGui::Text("HoveredIdx = %d, SelectedIdx = %d", params.RW_HoveredChannelIdx, params.RW_SelectedChannelIdx );
    }
}

} //namespace ImGui
#endif //IMGUI_MULTIPLOTLINES_DEMO
