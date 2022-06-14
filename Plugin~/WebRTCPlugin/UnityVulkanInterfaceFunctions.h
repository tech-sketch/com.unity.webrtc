#pragma once

#include <IUnityGraphicsVulkan.h>
#include <memory>

namespace unity
{
namespace webrtc
{
    template<typename T>
    inline bool AddInterceptInitialization(T* instance, UnityVulkanInitCallback func, void* userdata, int priority)
    {
        return instance->AddInterceptInitialization(func, userdata, priority);
    }

    template<>
    inline bool AddInterceptInitialization(
        IUnityGraphicsVulkan* instance, UnityVulkanInitCallback func, void* userdata, int priority)
    {
        // IUnityGraphicsVulkan is not supported AddInterceptInitialization.
        return instance->InterceptInitialization(func, userdata);
    }

    class UnityGraphicsVulkan
    {
    public:
        virtual bool InterceptInitialization(UnityVulkanInitCallback func, void* userdata) = 0;
        virtual PFN_vkVoidFunction InterceptVulkanAPI(const char* name, PFN_vkVoidFunction func) = 0;
        virtual bool AddInterceptInitialization(UnityVulkanInitCallback func, void* userdata, int priority) = 0;
        virtual UnityVulkanInstance Instance() = 0;
        virtual void AccessQueue(UnityRenderingEventAndData callback, int eventId, void* userData, bool flush) = 0;
        virtual void ConfigureEvent(int eventID, const UnityVulkanPluginEventConfig* pluginEventConfig) = 0;
        virtual ~UnityGraphicsVulkan() = default;

        static std::unique_ptr<UnityGraphicsVulkan> Get(IUnityInterfaces* unityInterfaces);
    };

    template<typename T>
    class UnityGraphicsVulkanImpl : public UnityGraphicsVulkan
    {
    public:
        UnityGraphicsVulkanImpl(T* vulkanInterface)
            : vulkanInterface_(vulkanInterface)
        {
        }
        ~UnityGraphicsVulkanImpl() = default;

        bool InterceptInitialization(UnityVulkanInitCallback func, void* userdata) override
        {
            return vulkanInterface_->InterceptInitialization(func, userdata);
        }

        PFN_vkVoidFunction InterceptVulkanAPI(const char* name, PFN_vkVoidFunction func) override
        {
            return vulkanInterface_->InterceptVulkanAPI(name, func);
        }

        bool AddInterceptInitialization(UnityVulkanInitCallback func, void* userdata, int priority) override
        {
            return unity::webrtc::AddInterceptInitialization(vulkanInterface_, func, userdata, priority);
        }

        void AccessQueue(UnityRenderingEventAndData callback, int eventId, void* userData, bool flush) override
        {
            vulkanInterface_->AccessQueue(callback, eventId, userData, flush);
        }

        void ConfigureEvent(int eventId, const UnityVulkanPluginEventConfig* pluginEventConfig) override
        {
            vulkanInterface_->ConfigureEvent(eventId, pluginEventConfig);
        }

        UnityVulkanInstance Instance() override { return vulkanInterface_->Instance(); }

    private:
        T* vulkanInterface_;
    };
}
}
