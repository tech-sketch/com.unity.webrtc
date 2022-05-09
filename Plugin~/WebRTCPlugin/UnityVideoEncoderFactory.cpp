#include "pch.h"

#include <tuple>

#include "api/video_codecs/builtin_video_encoder_factory.h"

#include "GraphicsDevice/GraphicsUtility.h"
#include "UnityVideoEncoderFactory.h"

#if CUDA_PLATFORM
#include "Codec/NvCodec/NvCodec.h"
#include <cuda.h>
#endif

#if UNITY_OSX || UNITY_IOS
#import "sdk/objc/components/video_codec/RTCDefaultVideoEncoderFactory.h"
#import "sdk/objc/native/api/video_encoder_factory.h"
#elif UNITY_ANDROID
#include "Android/AndroidCodecFactoryHelper.h"
#include "Android/Jni.h"
#endif

namespace unity
{
namespace webrtc
{
    using namespace ::webrtc::H264;
    typedef std::tuple<std::string, webrtc::VideoEncoderFactory*> VideoEncoderFactoryPair;

    const char kInternalCodecVendorName[] = "internal";
    const char kNvidiaCodecVendorName[] = "nvidia";
    const char kAppleCodecVendorName[] = "apple";
    const char kGoogleCodecVendorName[] = "google";
    const char kIntelCodecVendorName[] = "intel";
    const char kMicrosoftCodecVendorName[] = "microsoft";

    VideoEncoderFactoryPair CreateNativeEncoderFactory(IGraphicsDevice* gfxDevice)
    {
#if UNITY_OSX || UNITY_IOS
        webrtc::VideoEncoderFactory* factory =
            webrtc::ObjCToNativeVideoEncoderFactory([[RTCDefaultVideoEncoderFactory alloc] init]).release();
        return std::make_tuple(std::string(kGoogleCodecVendorName), factory);
#elif UNITY_ANDROID
        if (IsVMInitialized())
        {
            webrtc::VideoEncoderFactory* factory = CreateAndroidEncoderFactory().release();
            return std::make_tuple(std::string(kAppleCodecVendorName), factory);
        }
#elif CUDA_PLATFORM
        if (gfxDevice->IsCudaSupport())
        {
            CUcontext context = gfxDevice->GetCUcontext();
            NV_ENC_BUFFER_FORMAT format = gfxDevice->GetEncodeBufferFormat();
            webrtc::VideoEncoderFactory* factory = new NvEncoderFactory(context, format);
            return std::make_tuple(std::string(kNvidiaCodecVendorName), factory);
        }
#endif
        return std::make_tuple<std::string, webrtc::VideoEncoderFactory*>(nullptr, nullptr);
    }

    VideoEncoderFactory* FindFactory(const VideoEncoderFactoryMap& factories, const webrtc::SdpVideoFormat& format)
    {
        auto it = format.parameters.find("vendor");
        std::string vendor = it == format.parameters.end() ? nullptr : it->second;

        auto it2 = factories.find(vendor);
        return it2->second.get();
    }

    UnityVideoEncoderFactory::UnityVideoEncoderFactory(IGraphicsDevice* gfxDevice)
    {

        factories_.emplace(kInternalCodecVendorName, new webrtc::InternalEncoderFactory());

        VideoEncoderFactoryPair pair = CreateNativeEncoderFactory(gfxDevice);
        std::string vendor = std::get<0>(pair);
        if (!vendor.empty())
        {
            VideoEncoderFactory* factory = std::get<1>(pair);
            factories_.emplace(vendor, factory);
        }
    }

    UnityVideoEncoderFactory::~UnityVideoEncoderFactory() = default;

    std::vector<webrtc::SdpVideoFormat> UnityVideoEncoderFactory::GetSupportedFormats() const
    {
        std::vector<SdpVideoFormat> supported_codecs;

        for (const auto& pair : factories_)
        {
            for (const webrtc::SdpVideoFormat& format : pair.second->GetSupportedFormats())
            {
                webrtc::SdpVideoFormat newFormat = format;
                if (!pair.first.empty())
                    newFormat.parameters.emplace("vendor", pair.first);
                supported_codecs.push_back(newFormat);
            }
        }

        // Set video codec order: default video codec is VP8
        auto findIndex = [&](webrtc::SdpVideoFormat& format) -> long
        {
            const std::string sortOrder[4] = { "VP8", "VP9", "H264", "AV1X" };
            auto it = std::find(std::begin(sortOrder), std::end(sortOrder), format.name);
            if (it == std::end(sortOrder))
                return LONG_MAX;
            return std::distance(std::begin(sortOrder), it);
        };
        std::sort(
            supported_codecs.begin(),
            supported_codecs.end(),
            [&](webrtc::SdpVideoFormat& x, webrtc::SdpVideoFormat& y) -> int { return (findIndex(x) < findIndex(y)); });
        return supported_codecs;
    }

    webrtc::VideoEncoderFactory::CodecInfo
    UnityVideoEncoderFactory::QueryVideoEncoder(const webrtc::SdpVideoFormat& format) const
    {
        VideoEncoderFactory* factory = FindFactory(factories_, format);
        RTC_DCHECK(format.IsCodecInList(factory->GetSupportedFormats()));
        return factory->QueryVideoEncoder(format);
    }

    std::unique_ptr<webrtc::VideoEncoder>
    UnityVideoEncoderFactory::CreateVideoEncoder(const webrtc::SdpVideoFormat& format)
    {
        VideoEncoderFactory* factory = FindFactory(factories_, format);
        return factory->CreateVideoEncoder(format);
    }
}
}
