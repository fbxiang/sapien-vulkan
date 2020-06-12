#include "vulkan_context.h"
#include "vulkan_renderer.h"
#include "scene.h"
#include "common/log.h"
#include "pass/gbuffer.h"
#include "camera.h"

#include "gui.hpp"

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
  obj->mTransform.position = {0.5, 0, 0};
  obj->mTransform.scale = {0.1, 0.1, 0.2};
  scene.addObject(std::move(obj));
}

// void initImgui(VulkanContext &context, VulkanRenderer &renderer, ImGuiVulkanWindow &vulkanWindow) {

//   // Create Framebuffers
//   int w, h;
//   glfwGetFramebufferSize(context.getWindow(), &w, &h);
//   glfwSetFramebufferSizeCallback(context.getWindow(), glfw_resize_callback);
//   ImGuiVulkanWindow *wd = &vulkanWindow;
//   wd->mSurface = context.getSurface();

//   // Select Surface Format
//   std::vector<vk::Format> requestSurfaceImageFormat = {vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm,
//                                                        vk::Format::eB8G8R8Unorm, vk::Format::eR8G8B8Unorm};
//   const vk::ColorSpaceKHR requestSurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
//   wd->mSurfaceFormat = SelectSurfaceFormat(context.getPhysicalDevice(), wd->mSurface, requestSurfaceImageFormat,
//                                            requestSurfaceColorSpace);

//   // Select Present Mode
// #ifdef IMGUI_UNLIMITED_FRAME_RATE
//   std::vector<vk::PresentModeKHR> present_modes = {vk::PresentModeKHR::eMailbox,
//                                                    vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eFifo};
// #else
//   std::vector<vk::PresentModeKHR> present_modes = {vk::PresentModeKHR::eFifo};
// #endif
//   wd->mPresentMode = SelectPresentMode(context.getPhysicalDevice(), wd->mSurface, present_modes);

//   CreateImGuiVulkanWindow(context.getInstance(), context.getPhysicalDevice(),
//                           context.getDevice(), &vulkanWindow,
//                           context.getGraphicsQueueFamilyIndex(), nullptr, w, h, 2);

//   IMGUI_CHECKVERSION();
//   ImGui::CreateContext();
//   ImGuiIO &io = ImGui::GetIO();
//   // TODO: set controls

//   // ImGui::StyleColorsDark();

//   ImGui_ImplGlfw_InitForVulkan(context.getWindow(), true);
//   ImGui_ImplVulkan_InitInfo init_info = {};
//   init_info.Instance = context.getInstance();
//   init_info.PhysicalDevice = context.getPhysicalDevice();
//   init_info.Device = context.getDevice();
//   init_info.QueueFamily = context.getGraphicsQueueFamilyIndex();
//   init_info.Queue = context.getGraphicsQueue();

//   init_info.PipelineCache = VK_NULL_HANDLE;
//   init_info.DescriptorPool = context.getDescriptorPool();
//   init_info.Allocator = VK_NULL_HANDLE;
//   init_info.MinImageCount = 2;
//   init_info.ImageCount = 2;
//   init_info.CheckVkResultFn = check_vk_result;
//   ImGui_ImplVulkan_Init(&init_info, vulkanWindow.mRenderPass.get());

//   OneTimeSubmit(context.getDevice(), context.getCommandPool(), context.getGraphicsQueue(),
//                 [](vk::CommandBuffer cb) { ImGui_ImplVulkan_CreateFontsTexture(cb); });
// }


int main() {
  VulkanContext context;
  auto device = context.getDevice();
  auto renderer = context.createVulkanRenderer();
  auto scene = Scene(context.createVulkanScene());
  LoadCube(context, scene);

  auto camera = context.createCamera();
  camera->position = {0, 0, 3};

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

  device.waitIdle();

  vk::UniqueSemaphore sceneRenderSemaphore = context.getDevice().createSemaphoreUnique({});

  vk::UniqueFence sceneRenderFence =
      context.getDevice().createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
  vk::UniqueCommandBuffer sceneCommandBuffer = createCommandBuffer(context.getDevice(),
                                                                   context.getCommandPool(),
                                                                   vk::CommandBufferLevel::ePrimary);

  glfwSetWindowSizeCallback(context.getWindow(), glfw_resize_callback);

  while (!glfwWindowShouldClose(context.getWindow())) {
    glfwPollEvents();
    if (glfwGetKey(context.getWindow(), GLFW_KEY_Q)) {
      glfwSetWindowShouldClose(context.getWindow(), true);
    }

    if (gSwapchainRebuild) {
      gSwapchainRebuild = false;
      device.waitIdle();
      vwindow.recreateSwapchain(gSwapchainResizeWidth, gSwapchainResizeHeight);
      vwindow.recreateImguiResources(context.getGraphicsQueueFamilyIndex());
      renderer->resize(vwindow.getWidth(), vwindow.getHeight());
      device.waitIdle();
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
  }
  device.waitIdle();

  glfwDestroyWindow(context.getWindow());
  glfwTerminate();
  log::info("Finish");
}
