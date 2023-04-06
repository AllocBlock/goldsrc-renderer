# 一些对Vulkan的理解
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


### render pass和command buffer
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