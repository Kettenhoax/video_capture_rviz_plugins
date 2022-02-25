#include "video_capture_tool.hpp"
#include <rviz_common/display_context.hpp>
#include <rviz_common/view_manager.hpp>
#include <rviz_common/display_group.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/render_panel.hpp>
#include <QImage>
#include <QScreen>
#include <QGuiApplication>
#include <opencv2/imgproc.hpp>
#include <Ogre.h>


namespace video_capture_rviz_plugins
{

const size_t SCREEN_WIDTH = 1920;
const size_t SCREEN_HEIGHT = 1080;

VideoCaptureTool::VideoCaptureTool()
: Tool()
{
  shortcut_key_ = 'r';
  width_property_ = new rviz_common::properties::IntProperty(
    "width", 960,
    "Width of video in pixels", getPropertyContainer());

  tex_ = Ogre::TextureManager::getSingleton().createManual(
    "CaptureRenderTarget",
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    Ogre::TextureType::TEX_TYPE_2D,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    3,
    0,
    Ogre::PixelFormat::PF_R8G8B8,
    Ogre::TextureUsage::TU_RENDERTARGET);
}

VideoCaptureTool::~VideoCaptureTool()
{
}

void VideoCaptureTool::activate()
{
  auto w = width_property_->getInt();
  auto camera = context_->getViewManager()->getCurrent()->getCamera();
  double aspect_ratio = camera->getAspectRatio();
  auto width_screen_ratio = static_cast<double>(w) / static_cast<double>(SCREEN_WIDTH);
  video_height_ = static_cast<size_t>(static_cast<double>(w) / aspect_ratio);
  auto height_screen_ratio = static_cast<double>(video_height_) / static_cast<double>(SCREEN_HEIGHT);

  Ogre::RenderTexture * renderTexture = tex_->getBuffer()->getRenderTarget();
  if (renderTexture->getNumViewports() == 0) {
    renderTexture->addViewport(camera, 0, 0.0, 0.0, width_screen_ratio, height_screen_ratio);
    auto viewport = renderTexture->getViewport(0);
    viewport->setClearEveryFrame(true);

    Ogre::ColourValue clear_color;
    clear_color.r = 48.0 / 255.0;
    clear_color.g = 48.0 / 255.0;
    clear_color.b = 48.0 / 255.0;
    clear_color.a = 1.0;
    viewport->setBackgroundColour(clear_color);
    viewport->setOverlaysEnabled(false);
  }
  writer_.open("output.mp4", cv::VideoWriter::fourcc('h', 'v', 'c', '1'), 30.0, cv::Size(w, video_height_));
}

void VideoCaptureTool::deactivate()
{
  writer_.release();
}

void VideoCaptureTool::update(float, float)
{
  int w = width_property_->getInt();

  tex_->getBuffer()->getRenderTarget()->update();
  Ogre::HardwarePixelBufferSharedPtr buffer = tex_->getBuffer(0, 0);
  cv::Mat out_image(video_height_, w, CV_8UC3);

  buffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
  const Ogre::PixelBox & pb = buffer->getCurrentLock();
  // interpret buffer as OpenCV image matrix
  uint8_t * data = static_cast<uint8_t *>(pb.data);
  cv::Mat rendered_image(video_height_, w, CV_8UC3, data, SCREEN_WIDTH * 3);
  cv::cvtColor(rendered_image, out_image, cv::COLOR_RGB2BGR);
  buffer->unlock();
  writer_ << out_image;
}
} // namespace video_capture_rviz_plugins

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(video_capture_rviz_plugins::VideoCaptureTool, rviz_common::Tool)
