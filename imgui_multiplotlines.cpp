//----------------------------------------------------------------
// ImGui::MultiPlotLines
//----------------------------------------------------------------
#include "imgui_multiplotlines.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui/imgui_internal.h>

namespace ImGui
{

void MultiPlotLines( const char* label,
                     float (*get_value)(void* data, int value_idx, int channel_idx),
                     void* data,
                     int num_values,
                     int num_channels,
                     MultiPlotLines_Params* params,
                     float scale_min, float scale_max,
                     ImVec2 frame_size )
{
    // Early-out if skipped
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    // Fix/skip bad inputs
    if( num_values < 2 || num_channels < 1 )
        return;
    if( num_channels > MultiPlotLines_Params::cMaxChannels )
        num_channels = MultiPlotLines_Params::cMaxChannels;

    //-- Proces params, use defaults for anything undefined
    MultiPlotLines_Params DEFAULT_PARAMS;
    if( !params )
        params = &DEFAULT_PARAMS;
    static const char* s_Names[MultiPlotLines_Params::cMaxChannels] =
        { "C_00", "C_01", "C_02", "C_03", "C_04", "C_05", "C_06", "C_07",
          "C_08", "C_09", "C_10", "C_11", "C_12", "C_13", "C_14", "C_15",
          "C_16", "C_17", "C_18", "C_19", "C_20", "C_21", "C_22", "C_23",
          "C_24", "C_25", "C_26", "C_27", "C_28", "C_29", "C_30", "C_31" };
    auto gcn_fn = params->get_channel_name
                  ? params->get_channel_name
                  : []( const void* data, int channel_idx ){ return s_Names[channel_idx%MultiPlotLines_Params::cMaxChannels]; };
    // Palette from Vibrant,Muted and Light schemes in https://personal.sron.nl/~pault/
    static const ImU32 s_Palette[MultiPlotLines_Params::cMaxChannels] =
        { 0xFFDDAA77,0xFFFFDD99,0xFF998844,0xFF33CCBB,
          0xFF00AAAA,0xFF88DDEE,0xFF6688EE,0xFFBBAAFF,
          0xFF3377EE,0xFF1133CC,0xFF7733EE,0xFF7766CC,
          0xFF552288,0xFF9944AA,0xFF0000FF,0xFF00FF00,
          // REPEATED, modify to make LIGHTER versions instead?
          0xFFDDAA77,0xFFFFDD99,0xFF998844,0xFF33CCBB,
          0xFF00AAAA,0xFF88DDEE,0xFF6688EE,0xFFBBAAFF,
          0xFF3377EE,0xFF1133CC,0xFF7733EE,0xFF7766CC,
          0xFF552288,0xFF9944AA,0xFF0000FF,0xFF00FF00 };
    auto gcc_fn = params->get_channel_color
                  ? params->get_channel_color
                  : []( const void* data, int channel_idx ){ return s_Palette[channel_idx%MultiPlotLines_Params::cMaxChannels]; };
    auto gcp_fn = params->get_channel_parent
                  ? params->get_channel_parent
                  : []( const void* data, int channel_idx ){ return -1; };

    //-- Start drawing
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // Adjust sizes
    if (frame_size.x == 0.0f)
        frame_size.x = CalcItemWidth();
    if (frame_size.y == 0.0f)
        frame_size.y = frame_size.x;

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0, &frame_bb))
        return;

    const bool bHovered = ItemHoverable(frame_bb, id);

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX)
    {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = 0; i < num_values; i++)
        {
            for( int it_channel=0; it_channel<num_channels; it_channel++ )
            {
                if( params->RW_HideChannel[it_channel] )
                    continue;
                const float v = get_value(data, i, it_channel);
                if (v != v) // Ignore NaN values
                    continue;
                v_min = ImMin(v_min, v);
                v_max = ImMax(v_max, v);
            }
        }
        if (scale_min == FLT_MAX)
            scale_min = v_min;
        if (scale_max == FLT_MAX)
            scale_max = v_max;
    }

    //-- Plots
    RenderFrame( frame_bb.Min, frame_bb.Max,
                 GetColorU32(ImGuiCol_WindowBg), //FrameBg is blue, too intrusive
                 true, style.FrameRounding );

    // Init hovered from params, if within range
    int hovered_c_idx = params->RW_HoveredChannelIdx < num_channels
                        ? params->RW_HoveredChannelIdx
                        : -1;
    if( num_values > 1 )
    {
        int num_lines = num_values - 1;

        // UI/Interaction on hovered channel/slice
        if( bHovered && inner_bb.Contains(g.IO.MousePos) )
        {
            // Reset hovered, forget param value if any
            hovered_c_idx = -1;

            // mouse t,y \in [0..1)
            const float mouse_t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
            const float mouse_y01 = 1.0f - ImClamp((g.IO.MousePos.y - inner_bb.Min.y) / (inner_bb.Max.y - inner_bb.Min.y), 0.0f, 1.0f);
            const float mouse_v = scale_min + mouse_y01*(scale_max-scale_min);
            const int hovered_v_idx = (int)(mouse_t * num_lines);

            const float cHoveredMaxDistanceSq = (0.1f * (scale_max - scale_min))*(0.1f * (scale_max - scale_min));
            float closest_v = 0.0f;
            float closest_dist_sq = 2.0f * cHoveredMaxDistanceSq;
            for( int it_channel=0; it_channel<num_channels; it_channel++ )
            {
                if( params->RW_HideChannel[it_channel] )
                    continue;

                // Min at/around hover point, to prevent 1-frame spikes from being unselectable
                const float v[3] = { get_value(data, hovered_v_idx > 0 ? hovered_v_idx-1 : hovered_v_idx, it_channel),
                                     get_value(data, hovered_v_idx, it_channel),
                                     get_value(data, hovered_v_idx < num_values-2 ? hovered_v_idx+1 : hovered_v_idx, it_channel)};
                // Dist from centroid
                const float mid_v( 0.3333f*(v[0]+v[1]+v[2]) );
                const float mid_d_sq( (mouse_v - mid_v)*(mouse_v - mid_v) );
                // Dist from samples
                const float d_sq[3] = { (v[0] - mouse_v)*(v[0] - mouse_v),
                                        (v[1] - mouse_v)*(v[1] - mouse_v),
                                        (v[2] - mouse_v)*(v[2] - mouse_v) };
                const int min_d_idx( d_sq[0] < d_sq[1]
                                     ? (d_sq[0] < d_sq[2] ? 0 : 2)
                                     : (d_sq[1] < d_sq[2] ? 1 : 2) );
                // Choose closest to interval, but save closest value too
                if( mid_d_sq < closest_dist_sq )
                {
                    hovered_c_idx = it_channel;
                    closest_v = v[min_d_idx];
                    closest_dist_sq = mid_d_sq;
                }
            }

            // User-defined hovered value drawing
            if( hovered_c_idx != -1 )
            {
                if( params->HoveredDrawTooltip )
                    SetTooltip("%s %4.4g", gcn_fn( data, hovered_c_idx ), closest_v);
                if( params->HoveredDrawValue )
                {
                    const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));
                    const float closest_y01( (closest_v - scale_min) * inv_scale );
                    ImVec2 closest_pos = ImLerp(inner_bb.Min, inner_bb.Max, ImVec2(mouse_t,1.0f-closest_y01) );
                    window->DrawList->AddCircleFilled( closest_pos, 5, gcc_fn(data,hovered_c_idx), 10 );
                }
            }

            // User-defined mouse actions
            for( int it_mb=0; it_mb<3; it_mb++ )
            {
                if( g.IO.MouseClicked[it_mb] )
                {
                    switch( params->PlotMCA[it_mb] )
                    {
                    case MultiPlotLines_Params::eMCA_SelectChannel:
                        // Toggle selection, unselect if no channel hovereded
                        if( params->RW_SelectedChannelIdx == hovered_c_idx )
                            params->RW_SelectedChannelIdx = -1;
                        else
                            params->RW_SelectedChannelIdx = hovered_c_idx;
                        break;
                    case MultiPlotLines_Params::eMCA_ToggleChannel:
                        // Toggle channel visibility
                        if( hovered_c_idx != -1 )
                            params->RW_HideChannel[hovered_c_idx] = true;
                        break;
                    case MultiPlotLines_Params::eMCA_ToggleChildren:
                        // Toggle whole subhierarchy visibility
                        if( hovered_c_idx != -1 )
                            for( int it_c=hovered_c_idx+1; it_c<num_channels; it_c++ )
                                for( int parent = gcp_fn(data,it_c);
                                     parent != -1;
                                     parent = gcp_fn(data,parent) )
                                    if( parent == hovered_c_idx )
                                        params->RW_HideChannel[it_c] = !params->RW_HideChannel[it_c];
                        break;
                    case MultiPlotLines_Params::eMCA_None:
                    default:
                        break;
                    }
                }
            }
        }
        // Update hovered (will remain unmodified, if Plot is not currently hovered)
        params->RW_HoveredChannelIdx = hovered_c_idx;

        // Per-channel plot line
        for( int it_channel=0; it_channel<num_channels; it_channel++ )
        {
            if( params->RW_HideChannel[it_channel] )
                continue;

            const ImU32 channel_color = it_channel == params->RW_SelectedChannelIdx
                                        ? params->SelectedColor
                                        : gcc_fn(data,it_channel);
            // Additive channel + hovered + selected thickness
            float channel_thickness = 1.0f;
            if( it_channel == params->RW_HoveredChannelIdx )
                channel_thickness += params->HoveredDrawThickness;
            if( it_channel == params->RW_SelectedChannelIdx )
                channel_thickness += params->SelectedDrawThickness;

            const float t_step = 1.0f/num_lines;
            const ImVec2 alpha( 1.0f, params->RW_FilterAlpha );
            const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

            // Draw lines
            float t0 = 0.0f;
            float v0 = get_value(data, 0, it_channel);
            ImVec2 ftp0 = ImVec2( t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale) );
            for (int n = 0; n < num_lines; n++)
            {
                const float t1 = t0 + t_step;
                const float v1 = get_value( data, n+1, it_channel );
                const ImVec2 tp1 = ImVec2( t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale) ); //normalized
                const ImVec2 ftp1 = ftp0 + alpha*(tp1-ftp0); //filtered
                ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, ftp0);
                ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, ftp1);
                window->DrawList->AddLine( pos0, pos1, channel_color, channel_thickness );
                t0 = t1;
                ftp0 = ftp1;
            }
        }
    }

    // Centered Plot label, if not ##
    RenderTextClipped( ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y),
                       frame_bb.Max,
                       label, NULL, NULL, ImVec2(0.5f,0.0f));

    //-- Plot/UI
    if( params->bFilterUI )
    {
        ImGui::Text( "Filter" );
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.0f); //Control width to avoid leaking right
        ImGui::SliderFloat( "##FilterAlpha", &params->RW_FilterAlpha, 0.1f, 1.0f );
    }

    //-- Legend/UI
    if( params->bLegendUI )
    {
        ImGui::Checkbox( "Legend?", &params->RW_ShowLegend );
        bool bAll(false);
        ImGui::SameLine();
        if( ImGui::Button("All") )
            for( int it_channel=0; it_channel<num_channels; it_channel++ )
                params->RW_HideChannel[it_channel] = false;
        ImGui::SameLine();
        if( ImGui::Button("None") )
            for( int it_channel=0; it_channel<num_channels; it_channel++ )
                params->RW_HideChannel[it_channel] = true;
    }
    if( params->RW_ShowLegend )
    {
        // Try to use max possible columns minimize height
        int num_columns( num_channels > params->LegendMaxColumns
                         ? params->LegendMaxColumns
                         : num_channels );
        // Reduce column count if it results in the same height (rows) and a better distribution
        if( (num_channels % num_columns) != 0 //C is inexact
            && (num_channels % (num_columns-1)) == 0 //C-1 is exact
            && (num_channels / (num_columns-1)) <= (num_channels / num_columns + 1) ) //C-1 yields <= rows as C
            num_columns = num_columns - 1;
        ImGui::Columns( num_columns );
        for( int it_channel=0; it_channel<num_channels; it_channel++ )
        {
            const ImU32 channel_color = it_channel == params->RW_SelectedChannelIdx
                                        ? params->SelectedColor
                                        : gcc_fn(data,it_channel);

            // Setup checkbox colors
            ImGui::PushStyleColor( ImGuiCol_CheckMark, 0x00000000 ); //Disable check
            // Bg and Bg hovered use channel color, unless hidden
            ImGui::PushStyleColor( ImGuiCol_FrameBg, params->RW_HideChannel[it_channel] ? 0x00000000 : channel_color );
            ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, params->RW_HideChannel[it_channel] ? 0x00000000 : channel_color );
            ImGui::PushStyleColor( ImGuiCol_FrameBgActive, (channel_color & 0x00FFFFFF) | 0x77000000 ); //semi-transparent
            // Text+Border
            ImGui::PushStyleColor( ImGuiCol_Text, channel_color );
            ImGui::PushStyleColor( ImGuiCol_Border, channel_color );
            // Scope for 6x ImGui::PushStyleColor()
            {
                // NOTE: We draw a Checkbox but discard its potential
                // changes, and instead process clicks as MCA below
                const bool bTmp = params->RW_HideChannel[it_channel];
                ImGui::Checkbox( gcn_fn( data, it_channel ), &params->RW_HideChannel[it_channel] );
                params->RW_HideChannel[it_channel] = bTmp;

                // If hovered in Legend save for next Plot draw
                if( ImGui::IsItemHovered() )
                    params->RW_HoveredChannelIdx = it_channel;

                // If hovered in Plot or Legend draw border
                if( it_channel == params->RW_HoveredChannelIdx )
                    window->DrawList->AddRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), channel_color );

                // Run user-defined mouse actions on "fake checkbox"
                // TODO Duplicated from Plot, try to reuse same code?
                for( int it_mb=0; it_mb<3; it_mb++ )
                {
                    if( ImGui::IsItemClicked(it_mb) )
                    {
                        switch( params->LegendMCA[it_mb] )
                        {
                        case MultiPlotLines_Params::eMCA_SelectChannel:
                            // Toggle selection, unselect if no channel hovereded
                            if( params->RW_SelectedChannelIdx == it_channel )
                                params->RW_SelectedChannelIdx = -1;
                            else
                                params->RW_SelectedChannelIdx = it_channel;
                            break;
                        case MultiPlotLines_Params::eMCA_ToggleChannel:
                            // Toggle channel visibility
                            params->RW_HideChannel[it_channel] = !params->RW_HideChannel[it_channel];
                            break;
                        case MultiPlotLines_Params::eMCA_ToggleChildren:
                            // Toggle whole subhierarchy visibility
                            for( int it_c=it_channel+1; it_c<num_channels; it_c++ )
                                for( int parent = gcp_fn(data,it_c);
                                     parent != -1;
                                     parent = gcp_fn(data,parent) )
                                    if( parent == it_channel )
                                        params->RW_HideChannel[it_c] = !params->RW_HideChannel[it_c];
                            break;
                        case MultiPlotLines_Params::eMCA_None:
                        default:
                            break;
                        }
                    }
                }
            }
            ImGui::PopStyleColor(6);

            // Move to next column, consecutive channels are spread
            // horizontally, not vertically
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
    }
}

} //namespace ImGui
