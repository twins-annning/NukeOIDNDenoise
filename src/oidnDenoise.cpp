//
//  oidnDenoise.cpp
//  NukeOIDNDenoise
//
//  Created by Hengxing Wang on 2025/7/13.
//

#include "oidnDenoise.hpp"
#include "DDImage/Black.h"

void NukeOIDNDenoise::setupDevice()
{
    try
    {
        // create device
        m_device = oidnNewDevice(static_cast<OIDNDeviceType>(m_deviceType));
        
        const char *errorMessage;
        if (m_device.getError(errorMessage) != oidn::Error::None)
            throw std::runtime_error(errorMessage);

        // set device parameters
        m_device.set("numThreads", m_threads);
        m_device.set("setAffinity", m_affinity);

        // commit changes to the device
        m_device.commit();
    }
    catch (const std::exception &e)
    {
        std::string message = e.what();
        error("[OIDN]: %s", message.c_str());
    }
}


void NukeOIDNDenoise::setupFilter()
{
    try
    {
        // This can be an expensive operation, so try no to create a new filter for every image!
        m_filter = m_device.newFilter(FilterTypes[m_filterType]);
        
        // set the images
        m_filter.setImage("color", m_colorBuffer, oidn::Format::Float3, m_width, m_height);
        m_filter.setImage("output", m_outputBuffer, oidn::Format::Float3, m_width, m_height);
        
        if (albedo_connected)
        {
            m_filter.setImage("albedo", m_albedoBuffer, oidn::Format::Float3, m_width, m_height);
        }
        
        if (normal_connected)
        {
            m_filter.setImage("normal", m_normalBuffer, oidn::Format::Float3, m_width, m_height);
        }
        
        // set filter parameters
        if (m_filterType == 0) // RT
        {
            m_filter.set("hdr", m_hdr);
            m_filter.set("srgb", m_srgb);
        }
        else if (m_filterType == 1) // RTLightmap
        {
            m_filter.set("directional", m_directional);
        }
        
        if (m_inputScale > 0)
        {
            m_filter.set("inputScale", m_inputScale);
        }
        
        if (m_maxMem > 0)
        {
            m_filter.set("maxMemoryMB", m_maxMem);
        }
        
        oidn::Quality quality = oidn::Quality::Default;
        if (m_quality == 1)
        {
            quality = oidn::Quality::High;
        }
        else if (m_quality == 2)
        {
            quality = oidn::Quality::Balanced;
        }
        else if (m_quality == 3)
        {
            quality = oidn::Quality::Fast;
        }
        
        m_filter.set("quality", quality);
        m_filter.set("cleanAux", m_cleanAux);
        
        // commit changes to the filter
        m_filter.commit();
    }
    catch (const std::exception &e)
    {
        std::string message = e.what();
        error("[OIDN]: %s", message.c_str());
    }
}


void NukeOIDNDenoise::executeFilter()
{
    try
    {
        for (int i = 0; i < m_timesTorun; i++)
        {
            m_filter.execute();
            
            const char* errorMessage;
            if (m_device.getError(errorMessage) != oidn::Error::None)
                throw std::runtime_error(errorMessage);
        }
    }
    catch (const std::exception &e)
    {
        std::string message = e.what();
        error("[OIDN]: %s", message.c_str());
    }
}


const char* NukeOIDNDenoise::input_label(int n, char*) const
{
    switch (n)
    {
        case 0: return "color";
        case 1: return "albedo";
        default: return "normal";
    }
}


void NukeOIDNDenoise::_validate(bool for_real)
{
    copy_info();
}


void NukeOIDNDenoise::getRequests(const DD::Image::Box& box, const DD::Image::ChannelSet& channels, int count, DD::Image::RequestOutput &reqData) const
{
    int nInputs = (int)getInputs().size();
    for (int i = 0; i < nInputs; i++) {
        const DD::Image::ChannelSet channels = input(i)->info().channels();
        input(i)->request(channels, count);
    }
}


void NukeOIDNDenoise::knobs(DD::Image::Knob_Callback f)
{
    DD::Image::Enumeration_knob(f, &m_deviceType, DeviceTypes, "Device Type");
    DD::Image::Tooltip(f,
                       "default: select the likely fastest device (same as physical device with ID 0)\n"
                       "cpu: CPU device\n"
                       "sycl: SYCL device (requires a supported Intel GPU)\n"
                       "cuda: CUDA device (requires a supported NVIDIA GPU)\n"
                       "hip: HIP device (requires a supported AMD GPU)\n"
                       "metal: Metal device (requires a supported Apple GPU)");
    
    DD::Image::Enumeration_knob(f, &m_filterType, FilterTypes, "Filter Type");
    DD::Image::Tooltip(f, "The RTLightmap is optimized for denoising HDR and normalized directional lightmaps");
    
    DD::Image::Bool_knob(f, &m_hdr, "HDR");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    DD::Image::Tooltip(f, "the main input image is HDR");
    
    DD::Image::Bool_knob(f, &m_srgb, "sRGB");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    DD::Image::Tooltip(f, "the main input image is encoded with the sRGB (or 2.2 gamma) curve (LDR only) or is linear; the output will be encoded with the same curve");
    
    DD::Image::Bool_knob(f, &m_directional, "Directional");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    DD::Image::Tooltip(f, "whether the input contains normalized coefficients of a directional lightmap");
    
    DD::Image::Bool_knob(f, &m_cleanAux, "Clean Aux");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    DD::Image::Tooltip(f, "the auxiliary feature (albedo, normal) images are noise-free");
    
    DD::Image::Float_knob(f, &m_inputScale, DD::Image::IRange(0.0, 10.0), "Input Scale");
    DD::Image::Tooltip(f, "scales values in the main input image before filtering, without scaling the output too");
    
    DD::Image::Enumeration_knob(f, &m_quality, QualityTypes, "Quality");
    
    DD::Image::Int_knob(f, &m_threads, DD::Image::IRange(0, 10), "Threads");
    DD::Image::Tooltip(f, "0 will set it automatically to get the best performance");
    
    DD::Image::Bool_knob(f, &m_affinity, "Affinity");
    DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    DD::Image::Tooltip(f, "enables thread affinitization if it is necessary for achieving optimal performance");
    
    DD::Image::Int_knob(f, &m_maxMem, DD::Image::IRange(0, 1000), "Max Memory(MB)");
    DD::Image::Int_knob(f, &m_timesTorun, DD::Image::IRange(0, 10), "Times to Run");
}


int NukeOIDNDenoise::knob_changed(DD::Image::Knob* k)
{
    if (k->is("Device Type") || k->is("Threads") || k->is("Affinity"))
    {
        setupDevice();
        return 1;
    }
    
    return 0;
}


void NukeOIDNDenoise::renderStripe(DD::Image::ImagePlane& outputPlane)
{
    if (aborted() || cancelled())
        return;
    
    albedo_connected = true;
    normal_connected = true;
    
    // albedo
    if (dynamic_cast<DD::Image::Black*>(input(1)))
    {
        albedo_connected = false;
    }
    
    // normal
    if (dynamic_cast<DD::Image::Black*>(input(2)))
    {
        normal_connected = false;
    }
    
    DD::Image::Format imageFormat = input0().format();

    m_width = imageFormat.width();
    m_height = imageFormat.height();
    
    auto bufferSize = m_width * m_height * m_numberOfInputChannels * sizeof(float);

    // Allocate memory for each buffer
    m_colorBuffer = m_device.newBuffer(bufferSize);
    m_albedoBuffer = m_device.newBuffer(bufferSize);
    m_normalBuffer = m_device.newBuffer(bufferSize);
    m_outputBuffer = m_device.newBuffer(bufferSize);
    
    setupFilter();
    
    // Acquire pointers to the float data of each buffer
    float* colorPtr = static_cast<float*>(m_colorBuffer.getData());
    float* albedoPtr = static_cast<float*>(m_albedoBuffer.getData());
    float* normalPtr = static_cast<float*>(m_normalBuffer.getData());
    float* outputPtr = static_cast<float*>(m_outputBuffer.getData());

    if (!colorPtr || !albedoPtr || !normalPtr)
    {
        // Handle error: Buffer allocation failed or pointers are null
        error("Buffer data is nullptr");
        return;
    }
    
    for (auto i = 0; i < node_inputs(); ++i)
    {
        if (aborted() || cancelled())
            return;

        Iop* inputIop = dynamic_cast<Iop*>(input(i));
        
        // Can't figure out how to trigger this behavior, doesn't matter if input is connected or not.
        if (inputIop == nullptr)
            continue;
       
        // Validate input just in case before further processing.
        if (!inputIop->tryValidate(true))
            continue;
    
        // Set our input bounding box, this is what our inputs can give us.
        DD::Image::Box imageBounds = inputIop->info();

        // We're going to clip it to our format.
        imageBounds.intersect(imageFormat);
        const int fx = imageBounds.x();
        const int fy = imageBounds.y();
        const int fr = imageBounds.r();
        const int ft = imageBounds.t();
        
        // Request input based on our format.
        inputIop->request(fx, fy, fr, ft, m_inputChannels, 0);

        // Fetch plane from input into the image plane.
        DD::Image::ImagePlane inputPlane(imageBounds, false, m_inputChannels, m_numberOfInputChannels);
        inputIop->fetchPlane(inputPlane);

        auto chanStride = inputPlane.chanStride();
    
        // Iterate over each channel and get pixel values.
        for (auto y = 0; y < ft; y++)
        {
            for (auto x = 0; x < fr; x++)
            {
                for (auto chanNo = 0; chanNo < m_numberOfInputChannels; chanNo++)
                {
                    const float* indata = &inputPlane.readable()[chanStride * chanNo];
                    // Nuke bottom left is (0, 0) and image top left is (0, 0)
                    size_t index = ((ft - y - 1) * fr + x) * m_numberOfInputChannels + chanNo;
                    // Write to the appropriate buffer based on the channel number
                    if (i == 0)
                    {
                        colorPtr[index] = indata[y * fr + x];
                    }
                    if (i == 1)
                    {
                        albedoPtr[index] = indata[y * fr + x];
                    }
                    if (i == 2)
                    {
                        normalPtr[index] = indata[y * fr + x];
                    }
                }
            }
        }
    }
    
    if (aborted() || cancelled())
        return;
    
    // Execute denoise filter
    executeFilter();
    
    // Copy final output into the image plane.
    outputPlane.writable();
    for (auto chanNo = 0; chanNo < m_numberOfInputChannels; chanNo++)
    {
        int c = -1;
        if (chanNo == 0) c = outputPlane.chanNo(DD::Image::Channel::Chan_Red);
        if (chanNo == 1) c = outputPlane.chanNo(DD::Image::Channel::Chan_Green);
        if (chanNo == 2) c = outputPlane.chanNo(DD::Image::Channel::Chan_Blue);
        if (c < 0)
        {
            continue;
        }
        for (auto j = 0u; j < m_height; j++)
        {
            for (auto i = 0u; i < m_width; i++)
            {
                size_t index = ((m_height - j - 1) * m_width + i) * m_numberOfInputChannels + chanNo;
                outputPlane.writableAt(i, j, c) = outputPtr[index];
            }
        }
    }
    
}
