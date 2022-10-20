# 金源引擎渲染器
## 结果对比
- 游戏截图
![](./Doc/Images/assault_game.jpg)
- 渲染器截图
![](./Doc/Images/assault_this_renderer.png)

## 基础功能
- 渲染
  - 读取并渲染map文件 ✅已完成
  - 读取并渲染rmf文件
    - 解析wad文件 ✅已完成
    - 解析spr文件 ✅已完成
    - 解析mdl文件 ⏸暂停，完成部分
  - 读取并渲染bsp文件(固体+实体) ✅已完成
    - 实现实体特殊渲染
      - 点实体渲染为方块 ✅已完成
      - 特殊点实体渲染
        - Sprite图标渲染 ✅已完成
        - 应用到场景中 ⏸暂停
      - 模型渲染 ⏸暂停
- 物理
  - 实现物理demo
    - 基础形状的定义
    - 刚体定义
    - 运动模拟
    - 碰撞检测与处理
- 编辑器
  - 物体选取
    - 高亮显示 
      - bounding box ❌已弃用
      - 外轮廓 ✅已完成
  - 物体变换
  - 实体设置
    - FGD配置 ⏸暂停
  - 保存文件
    - 导出obj ✅已完成
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
  - 图片IO：stb_image (.jpg, .bmp, .png, .tga...), tinyexr (.exr)
- （可选）Python，编译Shader的脚本，如果要绕过Python实现自动编译详见compileShader.py

- 安装
  - 首先下载安装Vulkan https://github.com/microsoft/vcpkg/blob/master/ports/vulkan/usage
  - 然后安装vcpkg https://vcpkg.io/en/getting-started.html 
  - 添加环境变量VCPKG_DEFAULT_TRIPLET=x64-windows
    - 这样会变成默认安装64位版，否则默认是32位
    - 不添加环境变量的话，需要每条install末尾要加上:x64-windows来下载64位版本，如
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
  > 注意因为Vulkan并非下载源码安装，而是会去寻找本地已安装的SDK，所以才需要自己下载安装，此外安装后需要检查VULKAN_SDK环境变量是否正确
  

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
#### 第一版：
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
#### 第二版
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
#### 第三版
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
    - **解决方法2：在批量更新状态时，暂时禁用节点更新（具体方法是让swapchain无效化，这样整个链条无效，链条无效时不会重建renderpass），最后做一个恢复与重建即可；重复触发的问题，PortSet自己记录EventId，重复的Id不触发**

#### image layout如何管理
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

### 问题：错误定位
  - 只靠Handle ID难以定位对应Vulkan对象的位置
  - **预想解决方法：每个对象创建和销毁之间为一个Scope，通过Scope可以形成一个树状图，可以打印出来，从而找到对象在何处创建的**
    - 需要考虑反射，物体知道自己的名称
    - 如何自动构造Scope？

### 问题：Actor设计
  - 目前场景中的物体包含了多个属性（变换、网格、物理），这些属性之间有引用，有必要引入Actor的概念了
#### 1 Actor属性
  - 在Unity中，变换是Actor的原生组件，必须包含；而在Unreal中，变换组件非必须，但大多数Actor自带
    - Unity没有根组件，而Unreal有
  - Actor
    - Transform
      - Translate
      - Rotate
      - Scale
    - Mesh (BasicShape/TriangleList)
      - MeshData (静态或动态生成)
    - Collider
#### 2 Actor嵌套的变换，以及世界坐标、局部坐标的区分
#### 3 Actor渲染
  - 还没有抽象出材质，暂时是Renderpass遍历所有Actor，固定渲染流程
  - 网格数据管理，应该统一上传到一个Buffer，按需获取，也许需要一个ActorDataBuffer来方便管理，可以集成到Scene里
    - 不对，每种pass对物体数据需求不同（如：有的需要法线，有的不需要），不能统一buffer

### 问题：通用网格设计
  - 原始几何体（任意格式，面片或解析）、网格数据（三角形面片）、VertexBuffer之间的转换
  - 原始几何体可以生成一个网格数据，在内部实现
    - 网格数据是通用的，包含各类数据接口，如果原始几何体不包含某些数据，不填入即可
    - 每个属性对应一个数组
  - 网格数据到VertexBuffer
    - 问题1：两者结构不同，前者块存储，后者点存储，且后者只需要前者的部分数据
      - 如何把需求从pass传到scene，让scene帮忙完成？
        - PointData能否自动生成布局？目前只有内存布局，没有语义（每块内存对应了什么）
          - **目前是在PointData内部实现转换**
    - 问题2：多个网格数据可能需要合并到一个VertexBuffer

~~### 问题：网格和物理的对应关系~~
~~- 网格和碰撞体分离，一个网格可以包含一个碰撞体~~

### 问题：模型矩阵的更新
  - https://stackoverflow.com/questions/54103399/how-to-repeatedly-update-a-uniform-data-for-number-of-objects-inside-a-single-vu
  - 每个物体有不同的模型矩阵，而Uniform不能在pass begin后更新
  - 方案1：每个物体分配一个Uniform...
  - **方案2：Push constant，很方便**

### 问题：物理引擎设计
#### 问题：要保证物理引擎的稳定，可以使用固定间隔更新
  - 但需要单独的线程来处理，同时要同步渲染和物理之间的状态
  - **暂时先不独立更新，物理和渲染在同一个循环里更新**
#### 问题：碰撞体设计
  - 一个碰撞体可以是多种类型，包括基本形状、凸包、三角形面片
    - 还可以是几个碰撞体的组合
  - 碰撞体可以影响物体的Transform
    - 似乎有必要引入Actor的概念了
#### 问题：如何统一管理和更新场景物理状态
  - 每个物体有物理状态、材质和碰撞体
    - 物理状态表述物体的内在属性，如质量、质点
    - 材质表述和其他物体交互时的作用大小
    - 碰撞体表示其他物体交互时作用力的方向
    - 可以参考Unity(rigidbody, [unity物理引擎介绍](https://blog.csdn.net/qq_43545653/article/details/109700388))或Unreal的设计
      - Unity
        - Gameobject
          - Rigidbody
            - Mass...
          - Collider
            - Shape
            - Physical Material
              - friction
              - bounce...
    - 目前的结构
      - PhysicalState
        - PhysicalMaterial
        - Collider 暂时只允许一个

#### 问题：刚体旋转的表示和更新
  - 角速度用什么表示？单位四元数不能表示大小，非单位就不方便旋转了
  - 如何更新角速度？因为变成3D问题了，角加速度产生的新角速度怎么合并？

#### 问题：模拟准确性的验证
  - 圆周运动
    - 显式欧拉无法处理，出现数值爆炸
    - 验证混合方法的稳定性

### 问题：代码重构
  - 独立开发的好处：可以方便的重构，并且能在重构过程中暂停其他组件开发，以迭代的方式重构，无需考虑其他模块，等全部重构完成再去更新使用
    - 这样不需要受其他地方设计的限制，因为不需要时刻保证其他模块能正常运行

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

### 算法：法线模型变换矩阵
- 结论$$M=\text{inverse}(\text{Model})^T$$

### 技术：物理模拟
#### 概念
  ##### 物体分类
  - 静态：不会移动
  - 动态：可以移动，受力影响
  - Trigger：用于触发，不会阻挡
  - Kinematic：不遵守动力学，由游戏玩法逻辑控制

  ##### 物体形状
  - 球
  - 胶囊
  - 盒子
  - 凸多面体（凸包、水密）
  - 三角面（水密，一般只能静态）
  - 高度场

  ##### 属性/物理材质
  - 质量/密度
  - 质心
  - 摩擦力Friction与弹性Restitution

  ##### 力Force、冲量Impulse
  - 显式欧拉，隐式欧拉，半隐式欧拉（推荐、稳定易算）

  ##### 刚体动力学Rigid body Dynamics
  - 姿态 Orientation 矩阵、四元数
  - 角速度 Angular velocity
    - 三维，同时表示轴向和大小
  - 角加速度 Angular acceleration
  - 转动惯量/张量 Inertia tensor
  - 角动量 Angular momentum
  - 力矩 Torque

  ##### 碰撞检测
  - Broad Phase + Narrow Phase
    - 包围盒粗检测+精确计算与交点信息
    - Broad Phase
      - BVH
        - 更新成本低
      - Sort and Sweep
        - AABB，直接判断三轴区间是否重叠
        - 所有AABB整体排序，可以局部更新
    - Narrow Phase
      - 基本形状
        - 球与球：省略
        - 球与胶囊：两端半球+球与圆柱
        - 胶囊与胶囊：类似
      - Minkwski Difference-based methods
        - Minkwski Sum：空间中给定点集A和B，两集合内点两两相加得到的集合
          - 几何意义：参考 [104动画，【10.游戏引擎中物理系统的基础理论和算法 | GAMES104-现代游戏引擎：从入门到实践】 【精准空降到 1:22:46】](https://www.bilibili.com/video/BV16U4y117VU?share_source=copy_web&vd_source=b22720ef0ca41a79a4e3eb477a2fa563&t=4966)
          - 对凸多面体，其和等价于两个多面体顶点和的集合的凸包
        - Minkwski Difference
          - A+(-B)
          - 如果两个凸多面体有重叠，则A-B一定过原点！
        - GJK Algorithm
          - 基本思想：通过迭代，先寻找最有可能的三角形，随后继续迭代，从而加速，无需遍历整个物体
          - 一定程度能表示穿越厚度
      - Separating Axis Theorem (SAT) 分离轴
        - 2D
          - 对空间上的不相交的两个凸多面体，能找到一根轴，两个物体在轴上的投影是不重叠的
          - 由此，遍历A上每条边作为分离轴，测试是否分离，一旦找到一个满足条件，则说明不相交；如果A上未找到，再遍历B找一次
        - 3D
          - 遍历投影面
          - 只判断A、B自己的面不完善，还需要测试A的每条边叉乘B的每条边形成的面

  ##### 碰撞处理
  - 资料
    - [3D collision detection 提供很多场景的碰撞检测方法](http://www.miguelcasillas.com/?mcportfolio=collision-detection-c)
  - 分离重叠
    - Penalty Force
      - 很少用于游戏
      - 力度大、时间小的力
  - 求解约束
    - 拉格朗日
    - 迭代
      - 给定冲量
      - 计算约束，是否满足
      - 如果不满足，计算新的冲量
      - 继续计算约束...

  ##### Scene Query
  - Raycast
    - 光线与物体求交
    - Multiple hits
    - Closest hit
    - Any hit
  - Sweep
    - 类似Raycast，用一个体来扫描
  - Overlap
    - 给定一个形状，判断哪些有碰撞

  ##### 优化
  - Collision Group：分组管理
  - Island
    - 每个Island可以单独处理，其他Island直接Sleep
  - Continuous Collision Detection (CCD)
    - 连续碰撞检测
    - 两帧之间直接穿过物体：tunnelling
    - 可以加厚物体来避免...
    - CCD
      - 检测“安全”步长，动态调整模拟步长
  - Deterministic Simulation
    - 联网游戏，需要保证现象相同
      - 同步物理状态
    - same old+same input=same new
    - 固定物理模拟步长
    - 固定求解方法
    - 浮点精度问题

#### 刚体
- 刚体：刚体:在外力作用下，形状和大小都不发生变化的物体
  - 刚体即看作任意两质点间距离保持不变的特殊质点组
  - 理想模型

##### 刚体运动学
- [Bilibili: 刚体运动学和动力学](https://www.bilibili.com/video/BV1RW411L7MB)
- [GAMES103-基于物理的计算机动画入门](https://www.bilibili.com/video/BV12Q4y1S73g)
- 刚体运动可拆分为平动和转动
- 转动
  - 定轴转动：如风扇
  - 非定轴转动：如陀螺
- 定轴转动
  - 每一质点均作圆周运动，圆面为转动平面
  - 任一质点运动 $\Delta \theta, \vec{\omega} ,\vec{\alpha}$ （角位移、角速度、角加速度）均相同，但 $\vec{v} ,\vec{a}$ （速度、加速度）不同;
  - 运动描述仅需一个坐标，每个质点统一
  - 角坐标 $\theta=\theta(t)$ 
    - 标量逆时针为正
  - 角位移 $\Delta\theta=\theta(t+\Delta t) - \theta(t)$ 
  - 角速度矢量 $$\omega=\lim_{\Delta t\to 0}\frac{\Delta \theta}{\Delta t}=\frac{\mathrm{d}\theta}{\mathrm{d}t}$$
    - 方向：右手螺旋，与旋转轴平行
    - 有正负
  - 角加速度：$\vec{\alpha}=\frac{\mathrm{d}\omega}{\mathrm{d}t}$ 
  - 分析质点
    - 线速度：$\vec{v}=r\omega\vec{e_t}$
    - 加速度分为两个方向
      - $a_t=r\alpha$
      - $a_n=\omega v=r\omega^2$

##### 刚体动力学
- 力矩：转动的原因
  - =力臂叉乘力
  - 方向和转轴平行
  - 若力在转动平面内
    - 过作用点P作PO垂直转轴，O成为转点，$\vec{r}=\overrightarrow{OP}$
    - $\vec{M}=\vec{r}\times \vec{F}$
      - $M=Fr\sin\theta=Fd$，d是力臂
  - 否则分解为$\vec{F}=\vec{F_z}+\vec{F_\perp}$
    - $\vec{M_z}=\vec{r}\times \vec{F_\perp}$
  - 对刚体的自由转动，其转动中心始终是质心
    - 在定轴转动时，力矩可以直接相加，因为转动惯量相同...

- 转动定律
  - 简单情况：只分析单个质元
    - 在$\vec{F}$的作用下，得到力矩$\vec{M}$
    - 可得到$M=mr^2\alpha$，可见F产生的是角加速度
  - 放大到整体：
    - 每个质元受外力矩和内力矩影响
    - 整个刚体的力矩等于这些质元所受力矩的和
    - 注意刚体整体内力矩相互抵消，可以消除，得到
      - $$M=(\sum{\Delta m_jr_j^2})\alpha$$
      - 定义$J=(\sum{\Delta m_jr_j^2})$，它与外力无关，是刚体的内在属性，称为**转动惯量**或**惯性矩**
  - **转动定律：$M=J\alpha$，连接了受力和运动**
    - 单位$\mathrm{kg\cdot m^2}$
    - 转动惯量和刚体本身、转动轴有关
    - 刚体定轴转动的角加速度与它所受的合外力矩成正比，与刚体的转动惯量成反比.
    - 和牛顿第二定律类比

  - 平行轴定理
    - 质量为m的刚体，如果对其**质心轴**的转动惯量为$J_C$,则对任与该轴平行，相距为d的转轴的转动惯量为 $$J_O=J_C+md^2$$
    - 这也说明，沿质心旋转转动惯量最小

- 角动量/动量矩
  - [【科普】两分钟告诉你角动量是什么？](https://www.bilibili.com/video/BV19b41157u2)
  - 类似动量
  - $L=mvr$
  - 类似动量守恒，也有角动量守恒：物体所受合外力矩为零时，角动量守恒
  - 质量和半径变小时，速度增加

- 惯性张量
  - 刚体对固定轴有一个**转动惯量**，对固定点有一个**惯量张量**
  - 3D下，惯性张量是一个3x3矩阵，对应三根惯量主轴
  - 沿任意轴旋转可以分解到惯量主轴上处理？

- 角度不能相加（顺序相关），角速度可以，角加速度呢？
- 空气阻力
  - 公式： $$F = \frac{1}{2}CρSV^2$$
  - 系数表
    - 垂直平面体风阻系数大约1.0
    - 球体风阻系数大约0.5
    - 一般轿车风阻系数0.28-0.4
    - 好些的跑车在0.25
    - 赛车可以达到0.15
    - 飞禽在0.1-0.2
    - 飞机达到0.08
    - 目前雨滴的风阻系数最小
    - 在0.05左右


##### 四元数
- 资料
  - [3Blue1Brown：四元数的可视化](https://www.bilibili.com/video/BV1SW411y7W1)
  - [四元数和三维转动，可互动的探索式视频（请看链接）](https://www.bilibili.com/video/BV1Lt411U7og)
- 概念
  - 复数的扩展
  - 一个实部+三个虚部
  - 当实部=0时，三个虚部称为vector，是vector的由来
- 乘法
  - 计算：分配率
  - 几何意义：缩放+旋转
  - 不满足交换律
- 旋转
  - 对点$P$，旋转轴$\mathrm{a}i+\mathrm{b}j+\mathrm{c}k$，旋转角为α
  - 构造$q=\cos(\frac{\alpha}{2}) + \sin(\frac{\alpha}{2})(\mathrm{a}i+\mathrm{b}j+\mathrm{c}k)$
  - $$P'=q\cdot p\cdot q^{-1}$$
  - $q^{-1}$是 $q$的共轭
    - $q=w+\mathrm{a}i+\mathrm{b}j+\mathrm{c}k, q^{-1}=w-\mathrm{a}i-\mathrm{b}j-\mathrm{c}k$
    - $q=\cos(\frac{\alpha}{2}) + \sin(\frac{\alpha}{2})(\mathrm{a}i+\mathrm{b}j+\mathrm{c}k), q^{-1}=\cos(-\frac{\alpha}{2}) + \sin(-\frac{\alpha}{2})(\mathrm{a}i+\mathrm{b}j+\mathrm{c}k), $
    - 消除实部变化带来的扭曲？
- 插值 Slerp
- 混合
  - [四元数应用——顺序无关的旋转混合](https://zhuanlan.zhihu.com/p/28330428)


#### 碰撞处理
- Penalty Force
  - Quadratic
  - Log
- Impulse
- Constraint

#### 实现
- 资料
  - [游戏物理引擎(一) 刚体动力学](https://zhuanlan.zhihu.com/p/109532468)
  - [Video Game Physics Tutorial - Part I: An Introduction to Rigid Body Dynamics](https://www.toptal.com/game/video-game-physics-part-i-an-introduction-to-rigid-body-dynamics)
  - [刚体姿态的数学表达(一)：有限转动张量](https://blog.sciencenet.cn/blog-3452605-1303738.html)
  - [震惊! 转动惯量张量和一个刚体运动的结论竟做出这种事⋯](https://zhuanlan.zhihu.com/p/52915086)
- 循环
  - 分析受力
    - 重力
      - 可以开关
    - 瞬时力
      - 很多引擎中的addForce接口，这个力只应用一帧
  - 更新速度和位置
  - 碰撞检测
  - 解决约束
  - 渲染

---
## TODO
  - Debug scope功能
  - 目前swapchain的extent变化是renderpass在做，而不是port在做....会不会不太好
  - 自动排列commandbuffer
  - wad第一次搜寻找不到，重试后找到的问题
  - 绘制port相关的流程图
  - 最小化时，extent=(0,0)的处理
  - 动态shader编译
  - 三角形全部改为逆时针！
  - 可视化碰撞点、力度、法向
  - 碰撞分board和narrow
  - 公用shader的管理（放置、查找、编译）
  - pipeline生命周期、renderpass生命周期