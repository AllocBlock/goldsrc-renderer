# 金源引擎渲染器
## 功能
- 读取并渲染map文件 ✅已完成
- 渲染框架，UI与交互 🚀开发中
- ~~高级渲染效果~~
- ~~读取并渲染rmf文件~~
  - *解析wad文件* ✅已完成
  - ~~解析tga文件~~
  - ~~解析mdl文件~~
  - ~~解析spr文件~~
- ~~读取并渲染bsp文件(仅固体)~~ ✅已完成
## 可能不会开发的功能
- ~~读取并渲染bsp文件(完整)~~ ✅已完成
  - ~~实体触发机制与效果~~
  - ~~解析音频文件~~
- ~~读取并渲染dem文件~~
## 库
- 图形API：vulkan
- GUI库：glfw+imgui
  - file dialog文件选择框基于 https://github.com/AirGuanZ/imgui-filebrowser
  	- 进行了重构，删除了不需要的功能
- 数学库：glm
- 图片读取：stb_image