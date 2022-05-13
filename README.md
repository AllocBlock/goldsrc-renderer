# 金源引擎渲染器
## 结果对比
- i

## 基础功能
- 读取并渲染map文件 ✅已完成
- 读取并渲染rmf文件
  - 解析wad文件 ✅已完成
  - 解析spr文件 ✅已完成
  - 解析mdl文件 🚀开发中，完成部分
- 读取并渲染bsp文件(固体+实体) ✅已完成
  - 实现实体特殊渲染
    - 点实体渲染为方块 
    - 特殊点实体渲染
      - 图标渲染 ✅已完成
    - 模型渲染 🚀开发中
- 编辑器
  - 物体选取
  - 物体变换
  - 实体设置
    - FGD配置 🚀开发中
  - 保存文件
## 其他功能（画🍕）
- 高级渲染效果 🚀开发中
  - PBR
    - 实时PBR管线
    - IBL 图像照明
- bsp实体触发机制与效果
- 解析、播放地图音频
- 读取并渲染dem文件
## 依赖
- 所有C++依赖均通过vkpkg安装
  - 图形API：vulkan （官网下载安装配置环境变量VK_SDK_PATH为Vulkan根目录）
  - GUI库：glfw+imgui
    - file dialog文件选择框基于 https://github.com/AirGuanZ/imgui-filebrowser
    	- 进行了重构，删除了不需要的功能
  - 数学库：glm
  - 图片IO：stb_image
- （可选）Python，编译Shader的脚本，如果要绕过Python实现自动编译详见compileShader.py