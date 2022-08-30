# 金源引擎渲染器
## 结果对比
- 游戏截图
![](./Doc/Images/assault_game.jpg)
- 渲染器截图
![](./Doc/Images/assault_this_renderer.png)

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
- 所有C++依赖均通过vkpkg安装，使用64位版本
  - 图形API：vulkan （官网下载安装配置环境变量VULKAN_SDK为Vulkan根目录）
  - GUI库：glfw+imgui
    - file dialog文件选择框基于nativefiledialog
  - 数学库：glm
  - 图片IO：stb_image (.jpg, .bmp, .png, .tag...), tinyexr (.exr)
- （可选）Python，编译Shader的脚本，如果要绕过Python实现自动编译详见compileShader.py

- 安装
  - 首先安装vcpkg https://vcpkg.io/en/getting-started.html 
  - 添加环境变量VCPKG_DEFAULT_TRIPLET=x64-windows
    - 修改默认安装64位版，否则默认是32位
    - 否则每条install末尾要加上:x64-windows，如
    ```
    vcpkg install glm:x64-windows
    ```
  - 重启控制台，输入
  ```
  vcpkg install vulkan
  vcpkg install glm
  vcpkg install glfw3
  vcpkg install imgui[core,vulkan-binding,glfw-binding]
  vcpkg install stb
  vcpkg install tinyexr
  vcpkg install nativefiledialog

  vcpkg integrate install
  ```
  > 注意Vulkan并非下载源码安装，而是回去寻找本地已安装的SDK，因此需要自己下载安装后，配置好VULKAN_SDK环境变量
  > 详见：https://github.com/microsoft/vcpkg/blob/master/ports/vulkan/usage

## 架构图
- Vulkan调用流程图
![](./Doc/VulkanReferenceGraph.png)
- 数据流程图
![](./Doc/ClassIncludeGraph.png)
- 模块依赖
![](./Doc/ModuleDependency.png)


- TODO: Renderpass, pipeline 生命周期图

## 遇到的有价值的问题
- Renderpass链接的问题
  - 最初因为pass比较少，pass只有单入单出，不需要关心输入输出的流向在主函数里控制即可，更新也在主函数做
  - 后来因为pass变多，链接变得很长，重复代码太多了，此外，每次更新（如改变窗口大小）时都要手动重新连接，如果有遗漏也难以发现问题，于是寻求办法解决
  - 思路是给pass设计输入输出口，类似Falcor的设计，把输入输出口连接起来形成树状结构，更新时按照树状结构传播下去即可，并且输入是否准备好，输出是否指定了端口，更新时可能只需要部分更新（如更换一张纹理）等都可以实现
  - 因为有Swapchain的存在，每个link还需要好几份....但有的情况下又只需要一份，这种区分也不好做
  - 还有就是，Vulkan在开启pass时需要指定load状态，这些在没有依赖关系图的情况下很难处理好，因为不知道这张纹理是直接载入还是先清空
  - 第一版：
    - 设计Port和Link
    - Port表示一个Pass的输入输出需求，包括名称、格式、大小
    - Link代表了一个实际的链接状态，基于Pass的所有Port生成，为每个Port保存连接到它的纹理信息，并存储更新状态
      - Link是一个状态控制器，一旦有连接信息改变就会要求更新，但是还是没解决如何通知链路上全部节点的问题...
        - 可以把更新隐藏？要求每次使用前调用接口获取，实时拿到输入输出，下一次迭代准备这样做
    ```C++
    struct SPort
    {
        std::string Name;
        VkFormat Format;
        VkExtent2D Extent;
    };

    class CRenderPassPort
    {
    public:
        void addInput(std::string vName, VkFormat vFormat, VkExtent2D vExtent);
        void addOutput(std::string vName, VkFormat vFormat, VkExtent2D vExtent);
    private:
        std::vector<SPort> m_InputSet;
        std::vector<SPort> m_OutputSet;
    };

    enum class EPortType
    {
        INPUT,
        OUTPUT
    };

    struct SLink
    {
        std::string TargetName;
        VkImageView ImageView;
        EPortType Type;
        size_t Index;
    };

    class CRenderPassLink
    {
    public:
        CRenderPassLink() = delete;
        CRenderPassLink(const CRenderPassPort& vPorts) : m_Ports(vPorts) {}

        void link(std::string vTargetName, VkImageView vImageView, EPortType vType, size_t vIndex = 0);
        VkImageView getImage(std::string vTargetName, EPortType vType, size_t vIndex = 0) const;

        bool isUpdated() const { return m_Updated; }
        void setUpdateState(bool vState) { m_Updated = vState; }

    private:
        const CRenderPassPort& m_Ports;
        std::vector<SLink> m_LinkSet;
        bool m_Updated = false;
    };
    ```
  - 第二版
    - 为了保证实时，可以把Link改成独立链接，每个Port对应一个Link，每个Link链接一个或多个Port
      - Link可以只连接一个Port，作为原生输入或输出，此时由Link负责创建一个纹理，或是由用户指定一个纹理（比如交换链那里）
      - 是否要求Link创建后不可修改？感觉没问题，需要新的link只能重新创建，避免被误改
      - Pass，Port，Link需要互相知晓吗？如果不，要怎么构造依赖树，从而自动按顺序执行Pass？
        - 连接Port：在主程序完成，理论上可以保存为一个有向图
        - 更新Port：如何通知依赖这个Port的input port？
        - 如何处理Present image？下面单独讨论下
      - 另外注意Pass重建时，Link应该不需要重新生成
      - 比较复杂的一个问题，交换链图片的流通，两个思路
        - 思路一：交换链从头开始流转，可以同时作为pass的输入和输出，这样避免创建过渡纹理节约空间，但是如何处理这种情况？输出要怎么知道需要以输入为自己的目标，要怎么知道绘制前先clear还是说直接绘制？
          - 这类port单独处理，叫做InputSrcOutput
        - 思路二：交换链只在最后，要么单独一个output节点，要么就多做一次拷贝，这样所有的output（除了交换链output）都自己创建自己的纹理，需要保留原信息的就做一次拷贝（Falcor似乎就是这样的？），开销大但很直观
      - 另一个问题，如何通知发生了更新，以及如何消除更新状态？
        - 通知发生了更新，要么自己存储更新状态，外部不断查询，发现更新然后处理；要么引入hook，每次更新后调用hook..
        - 消除更新状态，因为同一个port可能被多个地方使用，不能随意清除更新状态
          - 第一个通知方法：可以存储更新时间戳，每个依赖自己存储当前使用的时间戳，对比时间戳来判断是否更新
          - 第二个通知方法：hook天生支持多个依赖，不需要修改什么，不过hook的方式是异步的，不一定好处理
      - 还有就是，一些特殊大小，如窗口大小怎么表示，怎么更新？
        - 通用匹配
    - Port的设计
      - 输入和输出的Port会有区别，输入依赖输出，输出可以自己指定纹理，或是自动生成纹理
      - 输入和输出如果想相互知晓，就要保持互相的指针，这需要互相知道类型，知道类型也能保证不会错误连接（比如In到In，Out到Out，或者In和Out反了）

- 如何管理场景
  - 场景包含很多物体，每个物体有很多个面片，每个面片有自己的纹理
  - 而渲染时希望保证状态变化少，一劳永逸，需要做一些权衡
  - 比如目前为了不拆散物体，允许一个物体有多个纹理，这导致一个物体drawcall需要引用多张纹理
    - 解决方法就是把所有纹理全部传入shader，这个量级其实是比较大的
    - 可以的做法是分batch，一次引用几张纹理这样，限制传入数量，可以考虑实现
  - 另外还可以考虑顺便实现static batching，也是现代引擎常用的方法，减少draw call
  - 还有部分需求是网状结构，很难处理
    - 比如对于一批物体，一方面希望控制每个物体的可见性，因此不方便拆分物体；另一方面希望实现同一纹理网格的batching，需要拆分物体...

- 如何管理渲染流程
  - Command buffer的顺序管理
  - command buffer的更新
    - 场景、设置的变化导致重录
    - 依赖资源（图片，framebuffer）变化导致重录
    - 是否让command buffer监听变化？需要设计一套依赖资源的集合，和port有点像...

- GUI事件处理
  - 部分事件需要全局的信息，需要在主函数处理，否则就需要把全局信息传入，产生耦合
  - 引入回调函数可以解决这个问题，但是大量的回调函数需要很多接口...
  - 可以引入事件系统，这需要涉及到
    - 事件列表的设计，是全局通用，还是GUI自己定义
    - hook和trigger
    - 事件参数如何传递？
