# 遇到的有价值的问题
## 问题：Renderpass之间的连接
  - 最初因为pass比较少，pass只有单入单出，不需要关心输入输出的流向在主函数里控制即可，更新也在主函数做
  - 后来因为pass变多，链接变得很长，重复代码太多了，此外，每次更新（如改变窗口大小）时都要手动重新连接，如果有遗漏也难以发现问题，于是寻求办法解决
  - 思路是给pass设计输入输出口，类似Falcor的设计，把输入输出口连接起来形成树状结构，更新时按照树状结构传播下去即可，并且输入是否准备好，输出是否指定了端口，更新时可能只需要部分更新（如更换一张纹理）等都可以实现
  - 因为有Swapchain的存在，每个link还需要好几份....但有的情况下又只需要一份，这种区分也不好做
  - 还有就是，Vulkan在开启pass时需要指定load状态，这些在没有依赖关系图的情况下很难处理好，因为不知道这张纹理是直接载入还是先清空
### 第一版：
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
### 第二版
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
### 第三版
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
  - **问题：就算按上述方法做依旧非常慢...另外顺序很混乱，遇到了bug：打断点慢运行不触发，而无断点会触发的情况**
    - 这个是不是eventid名称生成的粒度不够？...太快了所以出问题...换一个思路吧不用时间了
  - **问题：hook如何销毁？重新连接后原来的hook失效了**
    - 在外部clear

### image layout如何管理
  - 一个image可做多种用途（采样输入，渲染目标），在使用时最好是对应的layout，layout可以手动在command里转换，在renderpass渲染时也会指定一次layout转换，确认每个attachment输入和输出的layout
  - **解决方法：在PortFormat里加入Usage的信息，并在renderpass里做相应的转换**
    - **问题：在Renderpass里转换layout，说明输出的layout是固定的，如果一个port输出连接到了多个输入，而这几个输入所需格式各不相同怎么办？**

### 3.5版
  - swapchain大部分对renderpass隐藏，变成通用的sourceport，唯一需要的先验是swapchain的image数量，这个单独设计一个update attribute即可
    - 如果想把imagenum也隐掉（application每次都要调接口更新renderpass，而renderpass对application基类是未知的...），可以单独设计swapchain的port，让renderpass持有...?
    - 可有的pass不依赖swapchain image num？....好像image num永远被依赖
    - 临时给app加一个虚函数用来更新image num吧
  - **问题：Port内部存不存储名称？**
    - 不存储，外部用map管理，检索快但顺序乱了，不方便debug
    - 存储，检索麻烦了，另外port名称对port来说是否是自身的属性？...
    - 存储吧
  - **问题：image extent的问题，屏幕extent先验**
    - 首先不能只靠input port拿extent，有些pass没有input port，就需要知道屏幕尺寸
      - 这两种pass的更新策略不同！前者依赖input，后者依赖屏幕尺寸

## 问题：如何管理场景
  - 场景包含很多物体，每个物体有很多个面片，每个面片有自己的纹理
  - 而渲染时希望保证状态变化少，一劳永逸，需要做一些权衡
  - 比如目前为了不拆散物体，允许一个物体有多个纹理，这导致一个物体drawcall需要引用多张纹理
    - 解决方法就是把所有纹理全部传入shader，这个量级其实是比较大的
    - 可以的做法是分batch，一次引用几张纹理这样，限制传入数量，可以考虑实现
  - 另外还可以考虑顺便实现static batching，也是现代引擎常用的方法，减少draw call
  - 还有部分需求是网状结构，很难处理
    - 比如对于一批物体，一方面希望控制每个物体的可见性，因此不方便拆分物体；另一方面希望实现同一纹理网格的batching，需要拆分物体...

## 问题：Framebuffer的创建和更新
  - framebuffer目前需要到request前创建，它的更新怎么实现？
  - 要不要单独抽象一层，控制framebuffer的状态

## 问题：GUI事件处理
  - 部分事件需要全局的信息，需要在主函数处理，否则就需要把全局信息传入，产生耦合
  - 引入回调函数可以解决这个问题，但是大量的回调函数需要很多接口...
  - 可以引入事件系统，这需要涉及到
    - 事件列表的设计，是全局通用，还是GUI自己定义
    - hook和trigger
    - **事件参数如何传递？**


## 问题：Renderpass设计相关
### 问题：如何管理渲染流程，自动串联pass
  - Command buffer的顺序管理
  - command buffer的更新
    - 场景、设置的变化导致重录
    - 依赖资源（图片，framebuffer）变化导致重录
    - 是否让command buffer监听变化？需要设计一套依赖资源的集合，和port有点像...
    - 多个资源的引用与更新.... 
      - 第一版用于renderpass，定义一个更新结构体，每次更新穿这个结构体，虚函数自己根据状态判断
      - 要不要也改成hook的形式
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
    - **新问题：每次update时，renderpass都会检测是否需要重建，如果前后状态不一致则需要重建；但过程中pass会先断开连接再恢复连接，即使最初和最终的layout相同，但中间过程的layout不相同，导致先销毁再创建，而这次重建时多余的...如何处理**

## 问题：destroy和recreate如何方便的处理，指针置空的问题
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

## 问题：link上update的传递方向
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
  - **新问题：image和link两种update设计是否合理？**
    - 目前PortSet只监听了link update
    - 如果image update了也需要重建，但image是有数量的，每次更新其中一个image，一方面每次都触发更新，另一方面还没完全更新部分handle是无效的，如何处理？
    - **解决：重新set前先clear使其为invalid状态，加上对PortSet image update的监听**
    - renderpass的更新，实际输入和输出的更新可以分开...是否单独设计接口，而非直接用port的事件机制？

## 问题：在PortSet的基础上，如果pass之间有资源依赖，需要如何处理？
  - 例：shadowmap中，shade pass依赖shadow pass的shadow map，当port有效时，创建顺序是不定的，不能保证shadow pass先于shade pass
  - 比较复杂，目前先绕过：用port的hook监测更新，然后每次更新都尝试更新pipeline，但这样监测也有触发顺序的问题，是先重建还是先更新？而更新依赖于重建

## 问题：设计一个可自定义数据的MeshData结构，可以添加指定格式的数组
  - 作为一个中间结构，目标是让输入输出分离不依赖，但同时也需要同步输入和输出...自定义的话输出需要输入端的先验，是否违反设计原则了

## 问题：错误定位
  - 只靠Handle ID难以定位对应Vulkan对象的位置
  - **预想解决方法：每个对象创建和销毁之间为一个Scope，通过Scope可以形成一个树状图，可以打印出来，从而找到对象在何处创建的**
    - 需要考虑反射，物体知道自己的名称
    - 如何自动构造Scope？
    - 问题：有些构造不是线性的，不能只用个stack处理

## 问题：事件系统
  - 如何设计一个方便的事件系统？可能需要处理以下问题
    - 方便的定义事件，每个类所需事件不同，不想混到一起处理：宏来处理？
    - 事件参数的自定义，这会影响事件函数签名，不好处理：可变参数？
    - 一个思路：每类事件对应一个模板实例，专门处理一类事件，而需要处理这些事件的类就加这些事件的成员变量
      - 问题是接口怎么办呢？要转发到成员变量上...用宏自动生成一下转发一下？
    - 另一个思路：多继承，模板可以传字符串，但好像函数名称没办法..

## 问题：Actor设计
  - 目前场景中的物体包含了多个属性（变换、网格、物理），这些属性之间有引用，有必要引入Actor的概念了
### 1 Actor属性
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
### 2 Actor嵌套的变换，以及世界坐标、局部坐标的区分
### 3 Actor渲染
  - 还没有抽象出材质，暂时是Renderpass遍历所有Actor，固定渲染流程
  - 网格数据管理，应该统一上传到一个Buffer，按需获取，也许需要一个ActorDataBuffer来方便管理，可以集成到Scene里
    - 不对，每种pass对物体数据需求不同（如：有的需要法线，有的不需要），不能统一buffer

## 问题：通用网格设计
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

~~## 问题：网格和物理的对应关系~~
~~- 网格和碰撞体分离，一个网格可以包含一个碰撞体~~

## 问题：模型矩阵的更新
  - https://stackoverflow.com/questions/54103399/how-to-repeatedly-update-a-uniform-data-for-number-of-objects-inside-a-single-vu
  - 每个物体有不同的模型矩阵，而Uniform不能在pass begin后更新
  - 方案1：每个物体分配一个Uniform...
  - **方案2：Push constant，很方便**

## 问题：物理引擎设计
### 问题：要保证物理引擎的稳定，可以使用固定间隔更新
  - 但需要单独的线程来处理，同时要同步渲染和物理之间的状态
  - **暂时先不独立更新，物理和渲染在同一个循环里更新**
### 问题：碰撞体设计
  - 一个碰撞体可以是多种类型，包括基本形状、凸包、三角形面片
    - 还可以是几个碰撞体的组合
  - 碰撞体可以影响物体的Transform
    - 似乎有必要引入Actor的概念了
### 问题：如何统一管理和更新场景物理状态
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

### 问题：刚体旋转的表示和更新
  - 角速度用什么表示？单位四元数不能表示大小，非单位就不方便旋转了
  - 如何更新角速度？因为变成3D问题了，角加速度产生的新角速度怎么合并？

### 问题：模拟准确性的验证
  - 圆周运动
    - 显式欧拉无法处理，出现数值爆炸
    - 验证混合方法的稳定性

## 问题：Shader管理和编译
  - 静态编译的问题：build才编译，有时代码不变shader修改了无法自动编译
  - 管理问题：公有shader的路径和编译如何处理？
  - 问题：include的路径管理，需要配合编译器的参数
    - https://www.reddit.com/r/opengl/comments/r2iqx4/what_even_is_gl_google_include_directive/
      - 建议用GL_ARB_shading_language_include，
      - GL_GOOGLE_include_directive and GL_GOOGLE_cpp_style_line_directive are essentially worse specified versions of the ARB extension designed for glslang (the offline SPIR-V reference compiler).
      - 但Khrono Vulkan的glslangValidator好像不支持
    - https://github.com/KhronosGroup/glslang/issues/249
    - 还是用 GL_GOOGLE_include_directive 吧
    - 注意-I要在-V之前指定，-l已经包含在-V里了
  - 问题：避免重复编译，需要有缓存机制，只在发生修改时重新编译
  - 引入动态编译
  - 命令行的调用方法
    - std::system + >重定向输出
  - 问题：include变了如何重新编译
    - 得做一个include分析器....那不如干脆不用GL_GOOGLE_include_directive

## 问题：代码重构
  - 独立开发的好处：可以方便的重构，并且能在重构过程中暂停其他组件开发，以迭代的方式重构，无需考虑其他模块，等全部重构完成再去更新使用
    - 这样不需要受其他地方设计的限制，因为不需要时刻保证其他模块能正常运行

## 暂时不考虑重构了，实现功能优先，然后过程里考虑重构！