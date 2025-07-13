//
//  oidnDenoise.hpp
//  NukeOIDNDenoise
//
//  Created by Hengxing Wang on 2025/7/13.
//

#ifndef oidnDenoise_hpp
#define oidnDenoise_hpp

#include <stdio.h>
#include "DDImage/PlanarIop.h"
#include "DDImage/Knobs.h"

#include <OpenImageDenoise/oidn.hpp>

static const char* const CLASS = "oidnDenoise";
static const char* const HELP = "The Nuke plugin for Intel® Open Image Denoise library.";

static const char* const DeviceTypes[] = {"default", "cpu", "sycl", "cuda", "hip", "metal", 0};
static const char* const FilterTypes[] = {"RT", "RTLightmap", 0};
static const char* const QualityTypes[] = {"default", "high", "balanced", "fast", 0};


class NukeOIDNDenoise: public DD::Image::PlanarIop
{
    int m_deviceType;
    int m_filterType;
    bool m_hdr;
    bool m_srgb;
    bool m_directional;
    bool m_cleanAux;
    float m_inputScale;
    int m_quality;
    int m_threads;
    bool m_affinity;
    int m_maxMem;
    int m_timesTorun;
    
    DD::Image::ChannelSet m_inputChannels;
    int m_numberOfInputChannels;
    
    unsigned int m_width;
    unsigned int m_height;
    
    // This node channel connected
    bool albedo_connected;
    bool normal_connected;
    
    // OIDN
    oidn::DeviceRef m_device;
    oidn::FilterRef m_filter;

    oidn::BufferRef m_colorBuffer;
    oidn::BufferRef m_albedoBuffer;
    oidn::BufferRef m_normalBuffer;
    oidn::BufferRef m_outputBuffer;
    
public:
    NukeOIDNDenoise(Node* node): PlanarIop(node)
    {
        inputs(3);
        
        m_deviceType = 0;
        m_filterType = 0;
        m_hdr = false;
        m_srgb = false;
        m_directional = false;
        m_cleanAux = false;
        m_inputScale = 0.0f;
        m_quality = 0;
        m_threads = 0;
        m_affinity = true;
        m_maxMem = 0;
        m_timesTorun = 1;

        m_inputChannels = DD::Image::Mask_RGB;
        m_numberOfInputChannels = m_inputChannels.size();
        
        m_width = 0;
        m_height = 0;
        
        albedo_connected = true;
        normal_connected = true;
        
        m_device = nullptr;
        m_filter = nullptr;

        m_colorBuffer = nullptr;
        m_albedoBuffer = nullptr;
        m_normalBuffer = nullptr;
        m_outputBuffer = nullptr;
        
        setupDevice();
    }
    
    const char* input_label(int n, char*) const override;
    
    void _validate(bool) override;
        
    void getRequests(const DD::Image::Box& box, const DD::Image::ChannelSet& channels, int count, DD::Image::RequestOutput &reqData) const override;
        
    void renderStripe(DD::Image::ImagePlane& outputPlane) override;
        
    void knobs(DD::Image::Knob_Callback) override;
    int knob_changed(DD::Image::Knob* k) override;
    
    virtual bool useStripes() const override;
    virtual bool renderFullPlanes() const override;
    
    const char* Class() const override { return CLASS; }
    const char* node_help() const override { return HELP; }
    static const Iop::Description description;
    
    // OIDN methods
    void setupDevice();
    void setupFilter();
    void executeFilter();
};


bool NukeOIDNDenoise::useStripes() const
{
    return false;
}


bool NukeOIDNDenoise::renderFullPlanes() const
{
    return true;
}


static DD::Image::Iop* build(Node* node)
{
  return new NukeOIDNDenoise(node);
}


/* The Iop::Description is how NUKE knows what the name of the operator is,
   how to create one, and the menu item to show the user. The menu item may be
   0 if you do not want the operator to be visible.
 */
const DD::Image::Iop::Description NukeOIDNDenoise::description(CLASS, nullptr,  build);

#endif /* oidnDenoise_hpp */
