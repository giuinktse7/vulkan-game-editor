#include "qt_vulkan_info.h"

#include "core/graphics/device_manager.h"

QtVulkanInfo::QtVulkanInfo()
{
}

QtVulkanInfo::~QtVulkanInfo()
{
    VME_LOG_D("~QtVulkanInfo");
}

QtVulkanInfo::QtVulkanInfo(QQuickWindow *qml_window)
    : qml_window(qml_window)
{
    bool hasValidationLayer = QtVulkanInfo::checkValidationLayers(qml_window->vulkanInstance()->functions());
    if (hasValidationLayer)
    {
        vulkanInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
    }

    if (!vulkanInstance.layers().contains("VK_LAYER_KHRONOS_validation"))
    {
        VME_LOG_D("VK_LAYER_KHRONOS_validation not found");
    }

    auto extensions = qml_window->vulkanInstance()->supportedExtensions();
    VME_LOG_D("Extensions: " << extensions.size());
    for (int i = 0; i < extensions.size(); ++i)
    {
        qDebug() << extensions[i].name;
        // process items in numerical order by index
        // do something with "list[i]";
    }

    if (!vulkanInstance.create())
    {
        qWarning("Vulkan not available");
        ABORT_PROGRAM("No Vulkan available!");
    }
    else
    {
        VME_LOG_D("Vulkan instance created");
    }

    update();

    if (qml_window)
    {
        auto indices = DeviceManager::getQueueFamilies(*this);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = *indices.graphicsFamily;
        VkResult err = df->vkCreateCommandPool(m_dev, &poolInfo, nullptr, &commandPool);
        assert(err == VK_SUCCESS);
    }
}

bool QtVulkanInfo::checkValidationLayers(QVulkanFunctions *f)
{
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    uint32_t layerCount;
    f->vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    f->vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool layerFound = false;
    for (const char *layerName : validationLayers)
    {
        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                return true;
            }
        }
    }

    return layerFound;
}

void QtVulkanInfo::update()
{
    if (qml_window)
    {
        if (qml_window->vulkanInstance() != &vulkanInstance)
        {
            VME_LOG_D("Using custom vulkan instance");
            qml_window->setVulkanInstance(&vulkanInstance);
        }

        QSGRendererInterface *rif = qml_window->rendererInterface();
        QVulkanInstance *inst = reinterpret_cast<QVulkanInstance *>(
            rif->getResource(qml_window, QSGRendererInterface::VulkanInstanceResource));
        Q_ASSERT(inst && inst->isValid());

        m_physDev = *reinterpret_cast<VkPhysicalDevice *>(rif->getResource(qml_window, QSGRendererInterface::PhysicalDeviceResource));
        m_dev = *reinterpret_cast<VkDevice *>(rif->getResource(qml_window, QSGRendererInterface::DeviceResource));
        Q_ASSERT(m_physDev && m_dev);

        df = inst->deviceFunctions(m_dev);
        f = inst->functions();
    }
}

void QtVulkanInfo::frameReady()
{
    // if (window)
    // {
    //     window->frameReady();

    //     while (!window->waitingForDraw.empty())
    //     {
    //         window->waitingForDraw.front()();
    //         window->waitingForDraw.pop();
    //     }
    // }
}

void QtVulkanInfo::requestUpdate()
{
    // if (window)
    // {
    //     window->requestUpdate();
    // }
}

int QtVulkanInfo::maxConcurrentFrameCount() const
{
    // if (window)
    // {
    //     return window->MAX_CONCURRENT_FRAME_COUNT;
    // }
    // else
    // {
    // return 3;
    // }
    return _maxConcurrentFrameCount;
}

util::Size QtVulkanInfo::windowSize() const
{
    QSize size = qml_window->size();
    return util::Size(size.width(), size.height());
}

VkDevice QtVulkanInfo::device() const
{
    return m_dev;
}

VkPhysicalDevice QtVulkanInfo::physicalDevice() const
{
    return m_physDev;
}

VkCommandPool QtVulkanInfo::graphicsCommandPool() const
{
    return commandPool;
}

VkQueue QtVulkanInfo::graphicsQueue() const
{
    QSGRendererInterface *rif = qml_window->rendererInterface();
    VkQueue *queue = reinterpret_cast<VkQueue *>(rif->getResource(qml_window, QSGRendererInterface::CommandQueueResource));
    return *queue;
}

uint32_t QtVulkanInfo::graphicsQueueFamilyIndex()
{
    auto indices = DeviceManager::getQueueFamilies(*this);
    return *indices.graphicsFamily;
}

void QtVulkanInfo::setQmlWindow(QQuickWindow *qml_window)
{
    if (this->qml_window == qml_window)
    {
        return;
    }

    this->qml_window = qml_window;

    update();

    if (qml_window)
    {
        auto indices = DeviceManager::getQueueFamilies(*this);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = *indices.graphicsFamily;
        VkResult err = df->vkCreateCommandPool(m_dev, &poolInfo, nullptr, &commandPool);
        assert(err == VK_SUCCESS);
    }
}
