# 一些对Vulkan的理解
## Vulkan结构


## Bindless
- [Bindless讨论，以VK_EXT_descriptor_buffer为出发点](https://www.khronos.org/blog/vk-ext-descriptor-buffer)
- **目标**：尽量减少描述符的更新，固定数量资源变成动态数量

## Shader Language: GLSL or HLSL?
- Vulkan着色器使用SPIR-V中间语言，最初是由类似GLSL语法生成SPIR-V，在Vulkan1.2加入了类似HLSL语法生成SPIR-V
  - 选择哪一个？
- GLSL类似C的风格，而HLSL类似C++，提供了很多结构管理的功能
- HLSL可以编辑更底层的信息，如buffer具体放在哪个寄存器
- 但相比GLSL，HLSL实现一个简单shader也需要更多的代码
  - GLSL定义输入、输出和binding是C风格的，类似#define
  - 而HLSL是C++风格的，需要用结构体，输入输出还要指定语义


## 左乘与右乘、行主序和列主序
- https://www.cnblogs.com/X-Jun/p/9808727.html

### 左乘和右乘
- 对一个向量，它可以表示为行向量 $(N,1)$ ，也可以表示为列向量$(1,N)$
  - 进行矩阵乘法时，行向量只能左乘，而列向量只能右乘，以MVP变换为例
    - 左乘：$\text{Homo}=\text{Proj}*\text{View}*\text{Model}*\text{Position}$
    - 右乘：$\text{Homo}=\text{Position}*\text{Model}*\text{View}*\text{Proj}$
  - GLSL和HLSL**都是左乘**

### 行主序和列主序
- 存储一个矩阵时，在内存上实际上是一个摊平的一维连续数组
- 以一个2x2的矩阵为例，假如它的数据是```[a, b, c, d]```，不同主序解析得到的矩阵不同：
  - **行主序**：先行后列$$\begin{bmatrix}a & b\\c &d \end{bmatrix}$$
  - **列主序**：先列后行$$\begin{bmatrix}a & c\\b &d \end{bmatrix}$$
  - 其实两者互为转置
  - C++是行主序
  - glsl、hlsl、glm都是列主序
    - hlsl可以手动强制行主序，但没必要
- 主序对CPU-GPU之间的数据传输影响不大，一般默认已经做了转置，矩阵乘法也是只考虑左乘右乘这种抽象概念即可，主序具体是什么不影响矩阵乘法、矩阵索引等，**所以大部分时间无需关心主序问题**
- 除非**涉及内存布局操作**时，HLSL有坑，如初始化一个矩阵：
- 假如我得到了三个两两垂直的轴$X,Y,Z$，我可以用他构造一个旋转矩阵，用于旋转整个坐标系的点到新坐标系

$$R=
\begin{bmatrix}
X\\Y\\Z
\end{bmatrix} \
= \
\begin{bmatrix}
X_0,& X_1,& X_2\\Y_0,& Y_1,& Y_2\\Z_0,& Z_1,& Z_2
\end{bmatrix}
$$
- 在glsl中需要这么初始化，很符合直觉的直接构造即可
  ```glsl
  mat3 R = mat3(X, Y, Z)
  ```
  - glsl在初始化时做了映射，类似这样
  ```glsl
  // https://github.com/nintymiles/CGLearning/blob/master/DevelopingNotes/Matrix-Initialization-In-GLSL-2019-12-20.md
  mat3(float, float, float,  // 作为矩阵的第一列
	   float, float, float,  // 作为矩阵的第二列
	   float, float, float); // 作为矩阵的第三列
  ```
- 而在hlsl里需要这么初始化，要转置
  ```hlsl
  float3x3 R = transpose(float3x3(X, Y, Z))
  ```
  - hlsl是老老实实映射到内存布局上，所以**需要额外的转置**
  ```hlsl
  float3x3(float, float, float, // 作为矩阵的第一行
	      float, float, float,  // 作为矩阵的第二行
	      float, float, float); // 作为矩阵的第三行
  ``` 


## render pass和command buffer
- [relation between render passes and command buffers](https://stackoverflow.com/questions/48521961/what-is-the-relation-between-render-passes-command-buffers-and-clearing-of-atta)
- 工作流
```
begin commandbuffer 
    begin renderpass
        bind stuff (pipeline, buffers, descriptor sets)
        draw
    end renderpass
end commandbuffer

create_submit_info
submit_to_graphics_queue
```
- command buffer之间互相独立，如果要用多个commandbuffer，他们不共享状态，所以一些命令要重新指定
  - 特别是begin renderpass，开销很大[出处](https://kylemayes.github.io/vulkanalia/dynamic/secondary_command_buffers.html)
    - 而且如果设置了clear，就没有意义了
- secondary command buffer
  - 相比primary，secondary不能单独submit到queue，而是通过primary的vkCmdExecuteCommands来运行
  - secondary可以继承状态，无需重新设置
  - 使用方法
    - begin renderpass的subpass要指定为VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS，inline和secondary不能在一个subpass混用，这样要么用多个subpass，要么全部都用secondary buffer
    - secondary buffer在begin时要设置inheritance info，usage用VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT


## Subpass的设计和使用
### Subpass的设计
- 在Vulkan中，一个render pass可以有多个subpass，并且设计了很多同步机制来提升运行效率
- subpass之间是共享同一个framebuffer的，每个subpass里可以做以下操作
  - 写入一个或多个attachment
  - 读取一个或多个attachment
    - 叫做input attachement
    - 但注意这是有限制的，只能读取同一采样位置的同一个像素
      - 对Tile-based GPU的提升比较大，因为它是分tile执行renderpass的，每个tile单独执行整个每个subpass，整个framebuffer在执行时不一定有效，所以才有这样的限制
- ~~由此可以看到，Vulkan的render pass内可以一次完成很复杂的操作，比如可以在一个render pass里完成shadow map，第一个subpass生成shadow map写入attachment，而第二个subpass读取这shadow map来渲染场景阴影~~
  - input attachment能节省带宽（on-chip）
  - 如果有只能采样一个像素的限制，那么似乎只在一些后处理以及渲染结果合并上有用？
    > 但实际测试采样多个像素是不会报错或者触发validation的，也许性能损失比较大？、
    > Image subresources used as attachments must not be accessed in any other way for the duration of a render pass instance. 所以这个用法是错误的，需要分多个render pass实现

### Subpass的使用
- subpass的设计这一块充分体现了Vulkan的“verbose”特点，有相当多且复杂的东西需要配置...
1. **subpass的配置(VkSubpassDescription)**
   - 在创建render pass时，就需要给出许多有关subpass的信息了，包括
     - 有多少个subpass
     - 每个subpass要写入哪些attachment，要读取哪些attachment
       - 同时也设置了目标image layout，vulkan似乎会在写入后、读取前帮我们做这些转换？
2. **dependency的配置(VkSubpassDependency)**
   - 在Vulkan中，subpass的执行顺序是不固定，并非按照第一个subpass、第二个subpass、...这样的顺序来执行的
   - vulkan提供了subpass dependency的机制来处理subpass之间的依赖
     - 而且粒度不是subpass和subpass的先后，而是subpass中各个stage的先后，非常细
   - 设置好依赖后，vulkan会帮我们调度，最大限度利用硬件
     - 尽可能并行执行操作
     - 在出现依赖时进行同步
   - 每个依赖需要设置以下内容
     - 哪个pass（dstPass）依赖于哪个pass（srcPass）
     - srcStageMask: 指定一些stages（就是渲染管线里的那些stage，比如顶点着色、片元着色），在srcPass运行完这些stage前，不能继续
     - dstAccessMask：指定一些stages，stage之前的stage是没有依赖的，任意运行，但直到遇到这里面的stage，就需要等待srcStage那边满足要求
     - srcAccessMask：指定一些访问操作，效果和srcStageMask类似
     - dstAccessMask：指定一些访问操作，和dstStageMask类似
   - 关于依赖和同步，一些额外的资料
     - https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/ 
     - [关于 Vulkan Tutorial 中同步问题的解释](https://zhuanlan.zhihu.com/p/350483554)
     - [一张图形象理解Vulkan Sub Pass](https://zhuanlan.zhihu.com/p/461097833)
3. **input attachment**
   - 作为input attachment的image需要添加用途：VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
4. **descriptor和shader**
   - descriptor要用VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
   - 使用input attachment的shader，不能像普通shader一样引入，有特殊的语法 
     ```hlsl
     layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
     // subpassInput 不需要sampler
     ...
     vec3 color = subpassLoad(inputColor).rgb;
     ``` 
   > 而实际测试，descriptor就用普通的image，shader也按普通的写法，不会报错或者触发validation，感觉可以绕过inp 


- 关于实现subpass的资料
  - [Vulkan input attachments and sub passes](https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/)