# 金源引擎渲染器
## 基础功能
- 读取并渲染map文件 ✅已完成
- 读取并渲染rmf文件
  - *解析wad文件* ✅已完成
  - 解析spr文件 🚀开发中
  - 解析mdl文件
- 读取并渲染bsp文件(固体+实体) ✅已完成
  - 实现实体特殊渲染
- 编辑器
  - 物体选取
  - 物体变换
  - 实体设置
    - FGD配置
  - 保存文件
## 其他功能（画🍕）
- 高级渲染效果 🚀开发中
- bsp实体触发机制与效果
- 解析、播放地图音频
- 读取并渲染dem文件
## 库
- 图形API：vulkan
- GUI库：glfw+imgui
  - file dialog文件选择框基于 https://github.com/AirGuanZ/imgui-filebrowser
  	- 进行了重构，删除了不需要的功能
- 数学库：glm
- 图片IO：stb_image