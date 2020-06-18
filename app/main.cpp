#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/internal/vulkan_renderer.h"
#include "sapien_vulkan/scene.h"
#include "sapien_vulkan/common/log.h"
#include "sapien_vulkan/pass/gbuffer.h"
#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/camera.h"
#include "sapien_vulkan/camera_controller.h"

#include "sapien_vulkan/gui/gui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "sapien_vulkan/common/stb_image_write.h"

#include <chrono>

using namespace svulkan;

static bool gSwapchainRebuild = true;
static int gSwapchainResizeWidth = 800;
static int gSwapchainResizeHeight = 600;

static void glfw_resize_callback(GLFWwindow *, int w, int h) {
  gSwapchainRebuild = true;
  gSwapchainResizeWidth = w;
  gSwapchainResizeHeight = h;
}

void LoadCube(VulkanContext &context, Scene &scene) {
  auto mat = context.createMaterial();
  mat->setProperties({});

  auto mesh = VulkanMesh::CreateCube(context.getPhysicalDevice(), context.getDevice(),
                                     context.getCommandPool(), context.getGraphicsQueue());
  auto vobj = std::make_unique<VulkanObject>(
      context.getPhysicalDevice(), context.getDevice(),
      context.getDescriptorPool(), context.getDescriptorSetLayouts().object.get());
  vobj->setMesh(mesh);
  vobj->setMaterial(mat);
  auto obj = std::make_unique<Object>(std::move(vobj));
  obj->mTransform.position = {0, 0, 0};
  obj->mTransform.scale = {0.1, 0.1, 0.2};
  scene.addObject(std::move(obj));
}

void LoadRoom(VulkanContext &context, Scene &scene) {
  auto objs = context.loadObjects("/home/fx/Scenes/conference/conference.obj", {0.001f, 0.001f, 0.001f});
  for (auto &obj : objs) {
    scene.addObject(std::move(obj));
  }
}

void LoadSponza(VulkanContext &context, Scene &scene) {
  scene.setAmbientLight({0.3, 0.3, 0.3, 1});
  scene.addDirectionalLight({{0,-1,-0.1,1}, {1,1,1,1}});

  scene.addPointLight({{0.5, 0.3, 0,1}, {1,0,0,1}});
  scene.addPointLight({{  0, 0.3, 0,1}, {0,1,0,1}});
  scene.addPointLight({{-0.5, 0.3, 0,1}, {0,0,1,1}});

  auto objs = context.loadObjects("/home/fx/Scenes/sponza/sponza.obj", {0.001f, 0.001f, 0.001f});
  for (auto &obj : objs) {
    scene.addObject(std::move(obj));
  }
}

void LoadCustom(VulkanContext &context, Scene &scene) {
  // scene.addObject(context.loadCube({0.1, 0.1, 0.1}));
  // scene.addObject(context.loadSphere(0.1));
  // scene.addObject(context.loadYZPlane({1, 1}));
  auto obj = context.loadCapsule(0.1, 0.1);
  obj->mTransform.position = {0, 1, 0};
  scene.addObject(std::move(obj));
}

int main() {
  VulkanContext context;
  auto device = context.getDevice();
  auto renderer = context.createVulkanRenderer();
  auto scene = Scene(context.createVulkanScene());
  LoadCustom(context, scene);
  LoadSponza(context, scene);

  auto camera = context.createCamera();
  FPSCameraController cameraController(*camera, {0,0,-1}, {0,1,0});
  cameraController.setXYZ(0, 0.3, 0);
  cameraController.setRPY(0, 0, 1.5);

  renderer->resize(800, 600);

  std::vector<vk::Format> requestSurfaceImageFormat = {vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm,
    vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm};
  const vk::ColorSpaceKHR requestSurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

  VulkanWindow vwindow {
    context.getInstance(), context.getDevice(), context.getPhysicalDevice(),
    context.getGraphicsQueueFamilyIndex(), requestSurfaceImageFormat, requestSurfaceColorSpace};

  vwindow.initImgui(context.getDescriptorPool(), context.getCommandPool());

  vwindow.recreateSwapchain(800, 600);
  vwindow.recreateImguiResources(context.getGraphicsQueueFamilyIndex());
  camera->aspect = 4.f/3.f;
  device.waitIdle();

  vk::UniqueSemaphore sceneRenderSemaphore = context.getDevice().createSemaphoreUnique({});

  vk::UniqueFence sceneRenderFence =
      context.getDevice().createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
  vk::UniqueCommandBuffer sceneCommandBuffer = createCommandBuffer(context.getDevice(),
                                                                   context.getCommandPool(),
                                                                   vk::CommandBufferLevel::ePrimary);

  glfwSetWindowSizeCallback(vwindow.getWindow(), glfw_resize_callback);

  while (!vwindow.isClosed()) {

    if (gSwapchainRebuild) {
      gSwapchainRebuild = false;
      device.waitIdle();
      vwindow.recreateSwapchain(gSwapchainResizeWidth, gSwapchainResizeHeight);
      vwindow.recreateImguiResources(context.getGraphicsQueueFamilyIndex());
      renderer->resize(vwindow.getWidth(), vwindow.getHeight());
      device.waitIdle();
      camera->aspect = static_cast<float>(vwindow.getWidth()) / vwindow.getHeight();
      continue;
    }

    // get the next image
    try {
      vwindow.newFrame();
    } catch (vk::OutOfDateKHRError &e) {
      gSwapchainRebuild = true;
      device.waitIdle();
      continue;
    }

    // wait for previous frame to finish
    {
      device.waitForFences(sceneRenderFence.get(), VK_TRUE, UINT64_MAX);
      device.resetFences(sceneRenderFence.get());
    }

    // draw
    {
      sceneCommandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
      renderer->render(sceneCommandBuffer.get(), scene, *camera);
      renderer->display(sceneCommandBuffer.get(), vwindow.getBackBuffer(),
                        vwindow.getBackBufferFormat(), vwindow.getWidth(), vwindow.getHeight());
      sceneCommandBuffer->end();

      auto imageAcquiredSemaphore = vwindow.getImageAcquiredSemaphore();
      vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo info(1, &imageAcquiredSemaphore, &waitStage, 1, &sceneCommandBuffer.get(),
                          1, &sceneRenderSemaphore.get());
      context.getGraphicsQueue().submit(info, {});
    }

    auto swapchain = vwindow.getSwapchain();
    auto fidx = vwindow.getFrameIndex();

    vk::PresentInfoKHR info(1, &sceneRenderSemaphore.get(), 1, &swapchain, &fidx);
    try {
      vwindow.presentFrameWithImgui(context.getGraphicsQueue(), vwindow.getPresentQueue(),
                                    sceneRenderSemaphore.get(), sceneRenderFence.get());
    } catch (vk::OutOfDateKHRError &e) {
      gSwapchainRebuild = true;
    }
    device.waitIdle();

    // {
    //   auto albedo = renderer->downloadAlbedo();
    //   std::vector<uint8_t> img(albedo.size());
    //   for (uint32_t i = 0; i < albedo.size(); ++i) {
    //     img[i] = static_cast<uint8_t>(std::clamp(albedo[i] * 255, 0.f, 255.f));
    //   }
    //   stbi_write_png("albedo.png", 800, 600, 4, img.data(), 4 * 800);
    // }
    // {
    //   auto normal = renderer->downloadNormal();
    //   std::vector<uint8_t> img(normal.size());
    //   for (uint32_t i = 0; i < normal.size(); ++i) {
    //     img[i] = static_cast<uint8_t>(std::clamp(normal[i] * 255, 0.f, 255.f));
    //   }
    //   stbi_write_png("normal.png", 800, 600, 4, img.data(), 4 * 800);
    // }
    // {
    //   auto position = renderer->downloadPosition();
    //   std::vector<uint8_t> img(position.size());
    //   for (uint32_t i = 0; i < position.size(); ++i) {
    //     img[i] = static_cast<uint8_t>(std::clamp(position[i] * 255, 0.f, 255.f));
    //   }
    //   stbi_write_png("position.png", 800, 600, 4, img.data(), 4 * 800);
    // }
    // {
    //   auto depth = renderer->downloadDepth();
    //   std::vector<uint8_t> img(depth.size());
    //   for (uint32_t i = 0; i < depth.size(); ++i) {
    //     img[i] = static_cast<uint8_t>(std::clamp(depth[i] * 255, 0.f, 255.f));
    //   }
    //   stbi_write_png("depth.png", 800, 600, 1, img.data(), 800);
    // }


    if (vwindow.isKeyDown('q')) {
      vwindow.close();
    }

    if (vwindow.isMouseKeyDown(1)) {
      auto [x, y] = vwindow.getMouseDelta();
      float r = 1e-3;
      cameraController.rotate(0, -r * y, -r * x);
    }

    constexpr float r = 1e-3;
    if (vwindow.isKeyDown('w')) {
      cameraController.move(r, 0, 0);
    }
    if (vwindow.isKeyDown('s')) {
      cameraController.move(-r, 0, 0);
    }
    if (vwindow.isKeyDown('a')) {
      cameraController.move(0, r, 0);
    }
    if (vwindow.isKeyDown('d')) {
      cameraController.move(0, -r, 0);
    }
  }
  device.waitIdle();

  log::info("Finish");
}
