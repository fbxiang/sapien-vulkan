#include "vulkan_context.h"
#include "vulkan_renderer.h"
#include "scene.h"
#include "common/log.h"
#include "pass/gbuffer.h"
#include "camera.h"
#include "camera_controller.h"

#include "gui/gui.hpp"

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
  mat->updateDescriptorSets();
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
  auto objs = context.loadObjects("/home/fx/Scenes/conference/conference.obj", 0.01f);
  for (auto &obj : objs) {
    scene.addObject(std::move(obj));
  }
}

int main() {
  VulkanContext context;
  auto device = context.getDevice();
  auto renderer = context.createVulkanRenderer();
  auto scene = Scene(context.createVulkanScene());
  // LoadCube(context, scene);

  device.waitIdle();
  LoadRoom(context, scene);

  auto camera = context.createCamera();
  FPSCameraController cameraController(*camera, {0,0,-1}, {0,1,0});
  cameraController.setXYZ(0, 0.3, 3);

  renderer->resize(800, 600);

  std::vector<vk::Format> requestSurfaceImageFormat = {vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm,
    vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm};
  const vk::ColorSpaceKHR requestSurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

  VulkanWindow vwindow {
    context.getDevice(), context.getPhysicalDevice(),
    context.getSurface(), requestSurfaceImageFormat, requestSurfaceColorSpace};

  vwindow.initImgui(context.getWindow(), context.getInstance(),
                    context.getGraphicsQueueFamilyIndex(), context.getGraphicsQueue(),
                    context.getDescriptorPool(), context.getCommandPool());

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

  glfwSetWindowSizeCallback(context.getWindow(), glfw_resize_callback);

  while (!glfwWindowShouldClose(context.getWindow())) {

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
      vwindow.presentFrameWithImgui(context.getGraphicsQueue(), context.getPresentQueue(),
                                    sceneRenderSemaphore.get(), sceneRenderFence.get());
    } catch (vk::OutOfDateKHRError &e) {
      gSwapchainRebuild = true;
    }
    device.waitIdle();

    if (vwindow.isKeyDown('q')) {
      glfwSetWindowShouldClose(context.getWindow(), true);
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

  glfwDestroyWindow(context.getWindow());
  glfwTerminate();
  log::info("Finish");
}
