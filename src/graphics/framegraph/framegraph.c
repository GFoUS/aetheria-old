#include "framegraph.h"

framegraph_framegraph* framegraph_create(framegraph_config config) {
    framegraph_framegraph* framegraph = malloc(sizeof(framegraph_framegraph));
    CLEAR_MEMORY(framegraph);
    framegraph->config = config;

    return framegraph;
}

void framegraph_destroy(framegraph_framegraph* framegraph) {
    for (u32 i = 0; i < framegraph->numImages; i++) {
        free(framegraph->images[i]);
    }
    free(framegraph->images);
    free(framegraph->passes);
    free(framegraph);
}

void framegraph_add_image(framegraph_framegraph* framegraph, const char* name, VkFormat format, bool multisampled) {
    framegraph_image* image = malloc(sizeof(framegraph_image));
    CLEAR_MEMORY(image);
    image->framegraph = framegraph;
    image->name = name;
    image->width = framegraph->config.width;
    image->height = framegraph->config.height;
    image->format = format;
    image->multisampled = multisampled;

    framegraph->images[framegraph->numImages] = image;
    framegraph->numImages++;
}

framegraph_image* get_image_from_name(framegraph_framegraph* framegraph, const char* name) {
    for (u32 i = 0; i < framegraph->numImages; i++) {
        if (strcmp(name, framegraph->images[i]->name) == 0) return framegraph->images[i];
    }

    return NULL;
}

void framegraph_add_pass(framegraph_framegraph* framegraph, framegraph_pass_config config) {
    framegraph_pass* pass = malloc(sizeof(framegraph_pass));
    CLEAR_MEMORY(pass);

    pass->framegraph = framegraph;
    pass->config = config;

    for (u32 i = 0; i < pass->config.numInputs; i++) {
        framegraph_image* input = get_image_from_name(framegraph, pass->config.inputs[i]);
        if (input == NULL) {
            FATAL("Invalid image name: %s", pass->config.inputs[i]);
        }
        input->reads[input->numReads] = pass;
        input->numReads++;
    }
    for (u32 i = 0; i < pass->config.numOutputs; i++) {
        framegraph_image* output = get_image_from_name(framegraph, pass->config.outputs[i]);
        if (output->write != NULL) {
            FATAL("Image %s has already been written to", output->name);
            return;
        } 
        output->write = pass;
    }

    if (pass->config.depth) {
        framegraph_image* depth = get_image_from_name(framegraph, pass->config.depth);
        if (depth->write == NULL) {
            depth->write = pass;
        } else {
            depth->reads[depth->numReads] = pass;
            depth->numReads++;
        }
    }

    if (pass->config.resolve) {
        framegraph_image* resolve = get_image_from_name(framegraph, pass->config.resolve);
        if (resolve->write == NULL) {
            resolve->write = pass;
        } else {
            resolve->reads[resolve->numReads] = pass;
            resolve->numReads++;
        }
    }

    framegraph->passes[framegraph->numPasses] = pass;
    framegraph->numPasses++;
}

void flatten_passes(framegraph_pass* pass, size_t* numPasses, framegraph_pass** passes) {
    for (u32 i = 0; i < pass->config.numInputs; i++) {
        framegraph_image* input = get_image_from_name(pass->framegraph, pass->config.inputs[i]);
        passes = realloc(passes, sizeof(framegraph_pass*) * (*numPasses));
        passes[*numPasses++] = input->write;
        flatten_passes(input->write, numPasses, passes);
    }
}

framegraph_pass** get_pass_list(framegraph_framegraph* framegraph) {
    framegraph_pass** passesWithDuplicatesReversed = malloc(sizeof(framegraph_pass*));
    framegraph_pass* lastPass = NULL;
    for (u32 i = 0; i < framegraph->numPasses; i++) {
        if (framegraph->passes[i]->config.resolve && strcmp("backbuffer", framegraph->passes[i]->config.resolve) == 0) {
            lastPass = framegraph->passes[i];
            break;
        }
    }
    if (lastPass == NULL) {
        FATAL("There needs to be a pass that resolves to backbuffer");
    }
    passesWithDuplicatesReversed[0] = lastPass;

    size_t numPassesWithDuplicates = 1;
    flatten_passes(lastPass, &numPassesWithDuplicates, passesWithDuplicatesReversed);

    framegraph_pass** passesWithDuplicates = malloc(sizeof(framegraph_pass*) * numPassesWithDuplicates);
    for (u32 i = 0; i < numPassesWithDuplicates; i++) {
        passesWithDuplicates[numPassesWithDuplicates - (i + 1)] = passesWithDuplicatesReversed[i];
    }

    u32 passesPtr = 0;
    framegraph_pass** passes = malloc(sizeof(framegraph_pass*) * 256);
    CLEAR_MEMORY_ARRAY(passes, 256);
    for (u32 i = 0; i < numPassesWithDuplicates; i++) {
        bool unique = true;
        for (u32 j = 0; j < passesPtr; j++) {
            if (passesWithDuplicates[i] == passes[j]) {
                unique = false;
            }
        }
        if (unique) {
            passes[passesPtr] = passesWithDuplicates[i];
            passesPtr++;
        }
    }
    
    free(passesWithDuplicatesReversed);
    free(passesWithDuplicates);

    return passes;
}

void framegraph_compile(framegraph_framegraph* framegraph) {
    framegraph_pass** passes = get_pass_list(framegraph);
    INFO("Flattened passes");

    // TODO: Reflect info out of shaders to make renderpass, pipeline and descriptor info
    // TODO: Make physical resources (perhaps use a special allocator that reuses images)
    // TODO: Build renderpass barriers
}
void framegraph_create_resources(framegraph_framegraph* framegraph, vulkan_context* ctx) {}
VkCommandBuffer framegraph_record(framegraph_framegraph* framegraph, vulkan_context* ctx) {
    return vulkan_command_pool_get_buffer(ctx->commandPool);
}