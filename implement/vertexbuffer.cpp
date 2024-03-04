#include "../scene_viewer.hpp"

void SceneViewer::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * scene_config.get_mesh_vertex_count();
    std::cout << "Vertex buffer size: " << scene_config.get_mesh_vertex_count() << std::endl;
    createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);
}

void SceneViewer::copyVertexToBuffer() {
    // based on current valid vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * frame_vertices_static[currentFrame].size();
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, frame_vertices_static[currentFrame].data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, vertexBufferMemory);
}

void SceneViewer::copyAllMeshVertexToBuffer() {
    // each mesh is unique, we just apply differnt draw calls
    static_vertices.clear();
    static_vertices.resize(scene_config.get_mesh_vertex_count());

    std::cout << "Mesh vertex count: " << scene_config.get_mesh_vertex_count() << std::endl;

    int prev = 0;
    for (int inner_id = 0; inner_id < scene_config.cur_mesh; inner_id++) {
        std::shared_ptr<sconfig::Mesh> meshPtr = scene_config.id2mesh[scene_config.innerId2meshId[inner_id]];

        // we should assign material here too, as lambertian
        int material_id = meshPtr->material_id;
        std::shared_ptr<sconfig::Material> materialPtr = scene_config.id2material[material_id];
        // to be assigned
        float mat_idx = 0.0f; float mat_type = 0.0f;
        if (materialPtr->matetial_type == MaterialType::lambertian) {
            std::shared_ptr<sconfig::Lambertian> lambertianPtr = std::get<std::shared_ptr<sconfig::Lambertian>>(materialPtr->matetial_detail);
            std::string& filename = std::get<std::string>(lambertianPtr->albedo);
            if (lambertianPtr->albedo_type == TextureType::textureCube) {
                mat_type = 1;
                mat_idx = scene_config.textureCube2Idx[filename];
            }
            else {
                mat_type = 0;
                mat_idx = scene_config.texture2D2Idx[filename];
                std::cout << "file name: " << filename << " idx: " << mat_idx << std::endl;
            }
        }
        if (materialPtr->matetial_type == MaterialType::pbr) {
            throw std::runtime_error("PBR material is not supported yet!");
        }
        
        int vertex_count = meshPtr->vertex_count;
        // std::cout << "Inner vertex count: " << vertex_count << std::endl;
        for (int i = 0; i < vertex_count; i++) {
            static_vertices[prev + i] = {
                .pos = meshPtr->positions[i],
                .normal = meshPtr->normals[i],
                .color = meshPtr->colors[i],
                .texCoord = meshPtr->texcoords[i],
                .mappingIdxs = {mat_idx, 0.0f, 0.0f, mat_type}
            };
        }
        meshInnerId2Offset[inner_id] = prev;
        prev += vertex_count;
    }

    VkDeviceSize bufferSize = sizeof(Vertex) * scene_config.get_mesh_vertex_count();
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, static_vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, vertexBufferMemory);
}

uint32_t SceneViewer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


void SceneViewer::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}


// ----------- Declaration of help function ----------------
void SceneViewer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // create a buffer
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    // make allocation for the buffer
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
    };
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    // bind the buffer with the memory
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void SceneViewer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    
    VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    
    endSingleTimeCommands(commandBuffer);
}