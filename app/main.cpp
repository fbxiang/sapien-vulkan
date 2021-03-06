#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/internal/vulkan_renderer.h"
#include "sapien_vulkan/internal/vulkan_renderer_for_editor.h"
#include "sapien_vulkan/scene.h"
#include "sapien_vulkan/common/log.h"
#include "sapien_vulkan/pass/axis.h"
#include "sapien_vulkan/pass/gbuffer.h"
#include "sapien_vulkan/pass/transparency.h"
#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/pass/composite.h"
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

void LoadSponza(VulkanContext &context, Scene &scene) {
  scene.setAmbientLight({0.3, 0.3, 0.3, 1});
  scene.addDirectionalLight({{0,-1,-0.1,1}, {1,1,1,1}});

  scene.addPointLight({{0.5, 0.3, 0,1}, {1,0,0,1}});
  scene.addPointLight({{  0, 0.3, 0,1}, {0,1,0,1}});
  scene.addPointLight({{-0.5, 0.3, 0,1}, {0,0,1,1}});

  auto objs = context.loadObjects("../assets/sponza/sponza.obj", {0.001f, 0.001f, 0.001f});
  for (auto &obj : objs) {
    scene.addObject(std::move(obj));
  }
}

void LoadCustom(VulkanContext &context, Scene &scene) {
  // scene.addObject(context.loadCube({0.1, 0.1, 0.1}));
  auto obj = context.loadSphere();
  obj->mTransform.scale = {0.1, 0.1, 0.1};
  obj->mTransform.position = {0, 0.3, 0};

  PBRMaterialUBO mat;
  mat.additionalTransparency = 0.5;
  obj->updateMaterial(mat);

  scene.addObject(std::move(obj));
  // scene.addObject(context.loadYZPlane({1, 1}));
  // auto obj = context.loadCapsule(0.1, 0.1);
  // scene.addObject(std::move(obj));
}

int main() {
  VulkanContext context;
  auto device = context.getDevice();
  VulkanRendererConfig config;
  config.customTextureCount = 1;
  // auto renderer = context.createVulkanRendererForEditor(config);
  auto renderer = context.createVulkanRenderer();
  auto m = glm::mat4(1);
  m[0][0] = 0.1;
  m[1][1] = 0.1;
  m[2][2] = 0.1;
  // renderer->addAxes(m);
  // m[3][1] = 3;
  // renderer->addAxes(m);
  // m[3][1] = 1;
  // renderer->addStick(m);
  // m[3][1] = 2;
  // renderer->addStick(m);
  auto scene = Scene(context.createVulkanScene());
  LoadCustom(context, scene);
  LoadSponza(context, scene);

  auto camera = context.createCamera();

  FPSCameraController cameraController(*camera, {0,0,-1}, {0,1,0});
  cameraController.setXYZ(0, 0.3, 0);
  cameraController.setRPY(0, 0, 1.5);

  renderer->resize(800, 600);

  auto vwindow = context.createWindow();

  vwindow->initImgui(context.getDescriptorPool(), context.getCommandPool());

  vwindow->updateSize(800, 600);
  camera->aspect = 4.f/3.f;
  device.waitIdle();

  vk::UniqueSemaphore sceneRenderSemaphore = context.getDevice().createSemaphoreUnique({});

  vk::UniqueFence sceneRenderFence =
      context.getDevice().createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
  vk::UniqueCommandBuffer sceneCommandBuffer = createCommandBuffer(context.getDevice(),
                                                                   context.getCommandPool(),
                                                                   vk::CommandBufferLevel::ePrimary);

  glfwSetWindowSizeCallback(vwindow->getWindow(), glfw_resize_callback);

  int count = 0;
  while (!vwindow->isClosed()) {
    count += 1;

    if (gSwapchainRebuild) {
      gSwapchainRebuild = false;
      device.waitIdle();
      vwindow->updateSize(gSwapchainResizeWidth, gSwapchainResizeHeight);
      renderer->resize(vwindow->getWidth(), vwindow->getHeight());
      device.waitIdle();
      camera->aspect = static_cast<float>(vwindow->getWidth()) / vwindow->getHeight();
      continue;
    }

    // get the next image
    try {
      vwindow->newFrame();
    } catch (vk::OutOfDateKHRError &e) {
      gSwapchainRebuild = true;
      device.waitIdle();
      continue;
    }

    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    // wait for previous frame to finish
    {
      device.waitForFences(sceneRenderFence.get(), VK_TRUE, UINT64_MAX);
      device.resetFences(sceneRenderFence.get());
    }

    // draw
    {
      sceneCommandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
      renderer->render(sceneCommandBuffer.get(), scene, *camera);
      renderer->display(sceneCommandBuffer.get(), vwindow->getBackBuffer(),
                        vwindow->getBackBufferFormat(), vwindow->getWidth(), vwindow->getHeight());
      sceneCommandBuffer->end();

      auto imageAcquiredSemaphore = vwindow->getImageAcquiredSemaphore();
      vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo info(1, &imageAcquiredSemaphore, &waitStage, 1, &sceneCommandBuffer.get(),
                          1, &sceneRenderSemaphore.get());
      context.getGraphicsQueue().submit(info, {});
    }

    auto swapchain = vwindow->getSwapchain();
    auto fidx = vwindow->getFrameIndex();

    vk::PresentInfoKHR info(1, &sceneRenderSemaphore.get(), 1, &swapchain, &fidx);
    try {
      vwindow->presentFrameWithImgui(context.getGraphicsQueue(), vwindow->getPresentQueue(),
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


    if (vwindow->isKeyDown('q')) {
      vwindow->close();
    }

    if (vwindow->isMouseKeyDown(1)) {
      auto [x, y] = vwindow->getMouseDelta();
      float r = 1e-3;
      cameraController.rotate(0, -r * y, -r * x);
    }

    constexpr float r = 1e-3;
    if (vwindow->isKeyDown('w')) {
      cameraController.move(r, 0, 0);
    }
    if (vwindow->isKeyDown('s')) {
      cameraController.move(-r, 0, 0);
    }
    if (vwindow->isKeyDown('a')) {
      cameraController.move(0, r, 0);
    }
    if (vwindow->isKeyDown('d')) {
      cameraController.move(0, -r, 0);
    }

    // if (count == 1000) {
    //   renderer->switchToNormal();
    // }
    // if (count == 2000) {
    //   renderer->switchToDepth();
    // }
    // if (count == 3000) {
    //   renderer->switchToDepth();
    // }
  }
  device.waitIdle();

  log::info("Finish");
}
