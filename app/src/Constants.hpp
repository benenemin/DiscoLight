//
// Created by bened on 30/12/2025.
//

#pragma once

//////////////////////////////////////////////////////////////////////////////////
/************************ global constants **************************************/
//////////////////////////////////////////////////////////////////////////////////

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#define STRIP_NODE		DT_ALIAS(led_strip)
#if !DT_NODE_EXISTS(DT_NODELABEL(load_switch))
#error "Overlay for power output node not properly defined."
#endif
#if DT_NODE_HAS_PROP(STRIP_NODE, chain_length)
#define STRIP_NUM_PIXELS	DT_PROP(STRIP_NODE, chain_length)
#else
#error Unable to determine length of LED strip
#endif
#define CTRL_BTN_NODE DT_ALIAS(ctrl_btn)

namespace Constants
{

    static constexpr int SamplingInterval_us = 100; //us
    static constexpr size_t SamplingFrameSize = 512;
    static constexpr size_t ChainLength = STRIP_NUM_PIXELS;
}
