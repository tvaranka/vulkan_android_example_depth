#include <jni.h>
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <string>
#include <iostream>
#include <vector>
#include <fstream>

#define TAG "MY_TAG"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,    TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,     TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,     TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,    TAG, __VA_ARGS__)

/// GLOBAL VARIABLES
VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkPhysicalDevice physicalDevice;
uint32_t queueFamilyIndex = 0;
VkDevice device;
VkQueue queue;
VkDescriptorSetLayout setLayout;
VkPipelineLayout pipelineLayout;

struct pushConstants {
    uint32_t w;
    uint32_t h;
};

VkPipeline pipeline;
VkDescriptorPool descriptorPool;
VkDescriptorSet descriptorSet;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
VkDeviceMemory memory;

const bool enableValidationLayers = false;
///

std::vector<const char *> getRequiredExtensions() {

    std::vector<const char *> extensions;

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vector Add";
    appInfo.engineVersion = VK_API_VERSION_1_0;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_API_VERSION_1_0;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        LOGE("failed to create instance!");
    }
}

bool isDeviceSuitable(VkPhysicalDevice device){
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies){
        if(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT){
            queueFamilyIndex = i;
            return true;
        }
        i++;
    }

    return false;
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOGE("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        LOGE("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties gpuProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &gpuProperties);

    LOGD("Using device: %s", gpuProperties.deviceName);
}

void createLogicalDeviceAndQueue(){
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;


    deviceCreateInfo.enabledLayerCount = 0;

    if(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS){
        LOGE("failed to create logical device!");
    }

    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}

void createBindingsAndPipelineLayout(uint32_t bindingsCount){
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    for(uint32_t i = 0;i<bindingsCount;i++){
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = i;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBinding.descriptorCount = 1;
        layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindings.push_back(layoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
    setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    setLayoutCreateInfo.pBindings = layoutBindings.data();

    if(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &setLayout) != VK_SUCCESS){
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &setLayout;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.size = sizeof(pushConstantRange);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    if(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout)){
        LOGE("failed to create pipeline layout");
    }
}
std::vector<char> readFile(AAssetManager* mgr, const std::string &filename) {
    //AAsset* file = AAssetManager_open(mgr, ("shaders/" + filename).c_str(), AASSET_MODE_BUFFER);
    AAsset* file = AAssetManager_open(mgr, "shaders/image_blur.comp.spv", AASSET_MODE_BUFFER);
    if (!file) {
        LOGE("failed to open file");

    }
    std::vector<char> buffer(AAsset_getLength(file));
    if(AAsset_read(file, buffer.data(), buffer.size()) != buffer.size()) {
        LOGE("failed to read file");
    }
    AAsset_close(file);
    return buffer;
}

void createComputePipeline(AAssetManager* mgr, const std::string& shaderName){
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    auto shaderCode = readFile(mgr, shaderName);

    shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());
    shaderModuleCreateInfo.codeSize = shaderCode.size();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS){
        LOGE("failed to create shader module");
    }

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineCreateInfo.stage.module = shaderModule;

    pipelineCreateInfo.stage.pName = "main";
    pipelineCreateInfo.layout = pipelineLayout;

    if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS){// TODO
        LOGE("failed to create compute pipeline");
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}

void createBuffers(std::vector<VkBuffer> &buffers, uint32_t num_buffers, uint64_t buffer_size){
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(float) * buffer_size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

    buffers.resize(num_buffers);

    for(VkBuffer& buff : buffers){
        if(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buff) != VK_SUCCESS){
            LOGE("failed to create buffers");
        }
    }
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    LOGE("failed to find suitable memory type!");
}

void allocateBufferMemoryAndBind(const std::vector<VkBuffer> &buffers, std::vector<uint32_t> &offsets){
    VkDeviceSize requiredMemorySize = 0;
    uint32_t typeFilter = 0;

    for(const VkBuffer& buff : buffers){
        VkMemoryRequirements bufferMemoryRequirements;

        vkGetBufferMemoryRequirements(device, buff, &bufferMemoryRequirements);
        requiredMemorySize += bufferMemoryRequirements.size;

        if(bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0){
            requiredMemorySize += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.alignment;
        }
        typeFilter |= bufferMemoryRequirements.memoryTypeBits;
    }

    uint32_t memoryTypeIndex = findMemoryType(typeFilter, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requiredMemorySize;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
        LOGE("failed to allocate buffer memory");
    }

    VkDeviceSize offset = 0;

    for(const VkBuffer& buff : buffers){
        offsets.push_back(static_cast<uint32_t>(offset));

        VkMemoryRequirements bufferMemoryRequirements;
        vkGetBufferMemoryRequirements(device, buff, &bufferMemoryRequirements);

        if(vkBindBufferMemory(device, buff, memory, offset) != VK_SUCCESS){
            LOGE("failed to bind buffer memory");
        }

        offset += bufferMemoryRequirements.size;
        if(bufferMemoryRequirements.size % bufferMemoryRequirements.alignment != 0){
            offset += bufferMemoryRequirements.alignment - bufferMemoryRequirements.size % bufferMemoryRequirements.alignment;
        }
    }

}

void allocateDescriptorSets(const std::vector<VkBuffer>& buffers){
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(buffers.size());

    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &poolSize;
    if(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS){
        LOGE("failed to create descriptor pool");
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &setLayout;

    if(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS){
        LOGE("failed to allocate descriptor sets");
    }


    std::vector<VkWriteDescriptorSet> descriptorSetWrites(buffers.size());
    std::vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());

    uint32_t i = 0;
    for(const VkBuffer& buff : buffers){
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = descriptorSet;
        writeDescriptorSet.dstBinding = i;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        VkDescriptorBufferInfo buffInfo{};
        buffInfo.buffer = buff;
        buffInfo.offset = 0;
        buffInfo.range = VK_WHOLE_SIZE;
        bufferInfos[i] = buffInfo;

        writeDescriptorSet.pBufferInfo = &bufferInfos[i];
        descriptorSetWrites[i] = writeDescriptorSet;
        i++;
    }

    vkUpdateDescriptorSets(device, descriptorSetWrites.size(), descriptorSetWrites.data(), 0, nullptr);
}

void createCommandPoolAndBuffer(){
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

    if(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS){
        LOGE("failed to create command pool");
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType =
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS){
        LOGE("failed to allocate command buffer");
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_vulkan_1android_1depth_MainActivity_depth(
        JNIEnv* env,
        jobject,
        jobject assetManager,
        jobject img0,
        jobject img1) {
    AndroidBitmapInfo bmpInfo={0};
    if (AndroidBitmap_getInfo(env, img0, &bmpInfo) < 0) {
        LOGE("failed to load bitmap info");
    }
    char *data0, *data1;
    if (AndroidBitmap_lockPixels(env, img0, (void**)&data0)) {
        LOGE("failed to load bitmap pixels");
    }
    if (AndroidBitmap_lockPixels(env, img1, (void**)&data1)) {
        LOGE("failed to load bitmap pixels");
    }
    //Pixels in RGBA
    //make grayscale
    LOGD("Height, width: %d, %d, %d", bmpInfo.height, bmpInfo.width, bmpInfo.format);
    uint32_t w = bmpInfo.width;
    uint32_t h = bmpInfo.height;
    const uint32_t elements = w * h;
    char data_gray0 [elements];
    char data_gray1 [elements];
    //copy data to grey image
    for (int i = 0; i < elements; i++) {
        data_gray0[i] = data1[4 * i];
    }
    //vulkan setup
    createInstance();
    pickPhysicalDevice();
    createLogicalDeviceAndQueue();

    uint32_t bindingsCount = 2;
    createBindingsAndPipelineLayout(bindingsCount);

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    createComputePipeline(mgr, "image_blur.comp.spv");


    std::vector<VkBuffer> buffers;
    std::vector<uint32_t> offsets;

    createBuffers(buffers, 2, elements);
    allocateBufferMemoryAndBind(buffers, offsets);
    allocateDescriptorSets(buffers);

    createCommandPoolAndBuffer();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    pushConstants pC{.w = w, .h = h};

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pC);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(commandBuffer, (w+31)/32, (h+31)/32, 1);
    vkEndCommandBuffer(commandBuffer);
    char *d_data = nullptr;
    if(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&d_data)) != VK_SUCCESS){
        LOGE("failed to map device memory");
    }
    auto d_a = reinterpret_cast<float*>(d_data + offsets[0]);
    auto d_b = reinterpret_cast<float*>(d_data + offsets[1]);

    for(uint32_t i = 0; i < elements; i++){
        d_a[i] = data_gray0[i];
        d_b[i] = 0;
    }

    vkUnmapMemory(device, memory);

    //start time
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    //end time

    if(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void **>(&d_data)) != VK_SUCCESS){
        LOGE("failed to map device memory");
    }
    d_a = reinterpret_cast<float*>(d_data + offsets[0]);

    d_b = reinterpret_cast<float*>(d_data + offsets[1]);

    vkUnmapMemory(device, memory);

    //copy data from gray back to RGBA
    for (int i = 0; i < elements; i++) {
        data0[4 * i] = d_b[i];
        data0[4 * i + 1] = d_b[i];
        data0[4 * i + 2] = d_b[i];
    }
    AndroidBitmap_unlockPixels(env, img0);
    //release vulkan
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkFreeMemory(device, memory, nullptr);
    for(VkBuffer& buff : buffers){
        vkDestroyBuffer(device, buff, nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vulkan_1android_1depth_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}