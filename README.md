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
  - 解析mdl文件 ⏸暂停，完成部分
- 读取并渲染bsp文件(固体+实体) ✅已完成
  - 实现实体特殊渲染
    - 点实体渲染为方块 
    - 特殊点实体渲染
      - 图标渲染 ✅已完成
    - 模型渲染 ⏸暂停
- 编辑器
  - 物体选取
    - 高亮显示 
      - bounding box ✅已完成
      - 外轮廓 🚀开发中
  - 物体变换
  - 实体设置
    - FGD配置 ⏸暂停
  - 保存文件
## 其他功能（画🍕）
- 高级渲染效果 ⏸暂停
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
### 问题：Renderpass之间的连接
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
  - 第三版
    - 好像，InputPort和InputSrcOutputPort基本一样...是不是可以不用那么在意In还是Out，直接把Port定义为链表？
    - Port只有两种，输出Port和中转Port
      - **输出Port**：保有view，是链表的头，可以表示第二版的OutputPort
      - **中转Port**：依赖另一个port，可以表示第二版的InputPort和InputSrcOutputPort
      - 这样依赖关系很明确形成了，不过还不能单输出连接多输入...做成树状比较好
      - 也能解决掉renderpassPos
    - **问题：如何判断是否clear和toPresent？**
      - clear的话，如果是自己新建的，就需要clear，自己新建的对应了SourceOutput
      - 但对于swapchain image，他也是需要clear的，但他的SourceOutput不属于任何一个pass，只能是第一个pass的输入来clear它...需要单独判断
      - 但这也会导致attachment依赖port，要创建attachment必须有port...想起Falcor的处理，可以创建internal，也许也可用这个思路，不过暂时没啥影响
      - **解决方法：给swapchain image单独设计一个mark，每个source节点都可以调用接口判断是否是swapchain image**
    - **问题：目前是init->attachement description ->create render pass，link是在全部init后做的....**
      - 好像port和link可以最早做，但是先全部创建、link、然后再init会有个问题，目前scenepass有两个不同的实现，可以动态切换，也就是说整个渲染图会改变，这个怎么处理？
      - 也就是说，怎么实现动态的连接呢？目前通过是否是head、tail的方法好像不太合适
      - 这样，首先init和link需要分开，init需要确认link是否有效，无效则暂时不创建attachment
        - 随后如果link有更新，hook更新，（在下一个loop，还是立刻？立刻的话有可能太频繁了，或者保存一下状态，如果状态未改变则无需重新创建）重新创建attachment
        - 判断状态是否改变，是在PortSet里做，还是renderpass自己检查？
        - 构造函数不应该调虚函数，所以虚函数获取port布局是不可的...那么link必须要在init后了
        - **解决方法：暂时没想到很好的方法，等后面把调用顺序理清楚后再来处理**
    - **问题：改成不区分In/Out了，怎么处理同时做输入和输出的情况？**
      - 此时input和output对应了同一个image，该使用哪一个？不能只通过判断父节点是不是swapchain...
      - 要不要引入changeflag？还是区分input和inputsrcoutput，但是change本身和port无关的....
      - 或者...这类IO不用两个Port，用同一个Port！这样就是统一的了！！！！
      - **解决方法：这类同时输入输出的情况，合并为一个port来处理**
    - **问题：再renderpass构建过程中，存在冗余的重建**
      - **解决方法1：引入状态保存，每次更新时检查哪些发生了变化，针对变化的地方更新**
        - 但这样依然存在冗余重建，比如多次重复调用了更新，但也许可以等全部调用完后统一用一次更新
      - 方案一：在批量设置时，提供暂时屏蔽更新的指令；问题是什么时候恢复更新很难确定...不好
      - 方案二：收到更新状态后延迟更新；问题是延迟到什么时候，谁来触发？
      - **暂时放着吧，后面统计下多余重建的数量，然后把renderpass的流程画一下，看看怎么处理最好**
      - 引入下PresentPort，用于确定一个link是否完整，presentport只在最后定义（或者mark也可以），这样可以避免冗余重建！
      - 或者也不需要PresentPort，给SourcePort加一个暂时无效化，等创建完后再置为有效就行了
      - **解决方法2：在批量更新状态时，暂时禁用节点更新（具体方法是让swapchain无效化，这样整个链条无效，链条无效时不会重建renderpass），最后做一个恢复与重建即可**
  - **image layout如何管理**
    - 一个image可做多种用途（采样输入，渲染目标），在使用时最好是对应的layout，layout可以手动在command里转换，在renderpass渲染时也会指定一次layout转换，确认每个attachment输入和输出的layout
    - **解决方法：在PortFormat里加入Usage的信息，并在renderpass里做相应的转换**
      - **问题：在Renderpass里转换layout，说明输出的layout是固定的，如果一个port输出连接到了多个输入，而这几个输入所需格式各不相同怎么办？**

### 问题：如何管理场景
  - 场景包含很多物体，每个物体有很多个面片，每个面片有自己的纹理
  - 而渲染时希望保证状态变化少，一劳永逸，需要做一些权衡
  - 比如目前为了不拆散物体，允许一个物体有多个纹理，这导致一个物体drawcall需要引用多张纹理
    - 解决方法就是把所有纹理全部传入shader，这个量级其实是比较大的
    - 可以的做法是分batch，一次引用几张纹理这样，限制传入数量，可以考虑实现
  - 另外还可以考虑顺便实现static batching，也是现代引擎常用的方法，减少draw call
  - 还有部分需求是网状结构，很难处理
    - 比如对于一批物体，一方面希望控制每个物体的可见性，因此不方便拆分物体；另一方面希望实现同一纹理网格的batching，需要拆分物体...

### 问题：如何管理渲染流程，自动串联pass
  - Command buffer的顺序管理
  - command buffer的更新
    - 场景、设置的变化导致重录
    - 依赖资源（图片，framebuffer）变化导致重录
    - 是否让command buffer监听变化？需要设计一套依赖资源的集合，和port有点像...
    - 多个资源的引用与更新.... 
      - 第一版用于renderpass，定义一个更新结构体，每次更新穿这个结构体，虚函数自己根据状态判断
      - 要不要也改成hook的形式

### 问题：Framebuffer的创建和更新
  - framebuffer目前需要到request前创建，它的更新怎么实现？
  - 要不要单独抽象一层，控制framebuffer的状态

### 问题：GUI事件处理
  - 部分事件需要全局的信息，需要在主函数处理，否则就需要把全局信息传入，产生耦合
  - 引入回调函数可以解决这个问题，但是大量的回调函数需要很多接口...
  - 可以引入事件系统，这需要涉及到
    - 事件列表的设计，是全局通用，还是GUI自己定义
    - hook和trigger
    - **事件参数如何传递？**

### 问题：renderpass重建，在何时？如何触发？
  - 第一版：
    - 用recreate虚函数表示，主要处理swapchain的变化，比如大小改变
    - 但现在引入了renderpass自身的变化，如果还是用recreate，不好区分谁变化了，导致多余的重建
    - 可以改成事件形式，或者加入参数告诉它是什么发生了变化，pass自己判断是否需要重建
    - 有些混乱了...
      - 一条线是init，按线性调用
      - 另一条线是update，随时调用
      - 但init过程里可能调用update，这样会有一个悖论
        - 如果对象A的重建依赖对象B，但对象B在init中创建，而对象A在init前收到update需要更新...
        - 如果忽略这次更新，等B创建好了，不再有更新了，只能手动调用创建
        - 不忽略，B还未创建好，出错
        - 如何处理？
        - 要不要，把所有依赖的对象串成链...这样甚至可以一键全部销毁
          - 但是会导致怎样的问题呢？
          - 可能是图状，销毁还需要拓扑排序...
          - 需要处理循环依赖问题，有没有可能存在？
    - 另外，recreate的概念应该消除掉了，默认就是部分重建
      - 完成，全部改成onUpdate
      - 问题。onUpdate需要recreate，需要先判断销毁，而真正destroy又要判断一遍..

### 问题：destroy和recreate如何方便的处理，指针置空的问题
  - 目前是IVulkanHandle+智能指针解决
    - 这样必须先创建一个实例，还要判断指针是否存在
    - 可以抛弃掉指针否？引用的时候也不要填指针对象，而是直接存handle，保持vulkan原生
    - 对于一组类似的实例，也提供一个解决方案吧，免得全部都要维护
    - 尝试了抛弃指针，会有很多问题
      - 虽然不用指针，就不需要考虑创建对象和指针置空的问题，但会有新的问题
      - 首先，如果一个对象依赖另一个对象，而且需要那个保存对象，引用没法实现，还是需要指针
      - 其次，拷贝的问题也不好解决，容易影响到正常使用
      - 要不改成只允许指针
        - 置空用destroy实现
          - 模板？还是父类？
        - 创建用指定的new，不允许自己调用
        - 但是不太好实现..
          - 尝试定义父类，私有构造函数，make为友元，但是每个子类都要自己定义一下友元，不方便..
          - 暂时放着吧，但还是都改回指针
  - **解决方法：依旧使用智能指针，引入HandleSet便于数组集体销毁，引入模板函数辅助销毁和置空（destroyAndClear），模板函数判断有效（同时判断为空以及isValid函数返回值**


### 问题：link上update的传递方向
  - image update向尾部传递
  - link update向整条链传递
    - 如何保证不重复传递？
      - 告诉port是前方还是后方传递的，避免重复
    - 对外界而言，link update只触发一次要怎么实现？
      - 依旧需要触发每个节点的link update，否则节点有自己的hook
      - 要不要把linkupdate单独做一个实例，link上的所有port公用...但是要怎么管理？
      - 不好处理...让外界来过滤吧，所有都出发，但是带上前方还是后方传递的参数，可以知道第一个触发的是谁，就能只在第一次触发时更新..
        - 不行，再portset里处理的话，如果造成update的port是外部port，起点不在内部，那么就不会触发更新，造成错误...这就对应了上面提到的那个问题，不能只那一个节点触发更新，链路上所有节点都需要触发更新...要解决重复触发的问题，要么加更新事件ID区分，要么就用事件轮询...
      - 这里还有个触发顺序的问题，先触发邻接的update，还是先触发hook的update？
        - 最佳时先全部port的update，再全部hook的update...但是每个函数里的邻接与hook是顺序的，不可拆分
        - 暂时先邻接吧
  - **解决方法：目前link是树状的，image update向子节点传播，link update向父节点和子节点传递（参数带有传播来源节点，从而避免重复触发）**

### 暂时不考虑重构了，实现功能优先，然后过程里考虑重构！

## 一些用到的技术、算法
### 算法：光线与三角形求交
1. 重心坐标+射线方程联立求解
   - https://blog.csdn.net/MC_007/article/details/115743848
   - 用重心坐标表示三角形平面上的点：
     - $$P=(1-u-v)A+uB+vC$$
     - 对三角形内的任意一个点，满足$0\le u,v,u+v\le 1$
   - 原点+方向表示光线
     - $$P=O+tD$$
   - 联立可得变量t,u,v的三元一次方程组，求解

### 技术：物理模拟
#### 人物走动
---
## TODO
  - Debug scope功能
  - 目前swapchain的extent变化是renderpass在做，而不是port在做....会不会不太好
  - 新问题，layout的转换...
    - pass1有一个output，输入是attachment_optimized，而输出应该是什么？要看后续的使用，可以是attachment_optimized（继续写入），也可以是shader_read_optimized（读取）...
    - 有点复杂，暂时不管
    - outline整理
    - 自动排列commandbuffer
    - 更准确的raycast（per triangle）
    - wad第一次搜寻找不到，重试后找到的问题