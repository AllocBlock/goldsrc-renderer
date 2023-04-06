# 一些用到的技术、算法
## 算法：光线与三角形求交
1. 重心坐标+射线方程联立求解
   - https://blog.csdn.net/MC_007/article/details/115743848
   - 用重心坐标表示三角形平面上的点：
     - $$P=(1-u-v)A+uB+vC$$
     - 对三角形内的任意一个点，满足$0\le u,v,u+v\le 1$
   - 原点+方向表示光线
     - $$P=O+tD$$
   - 联立可得变量t,u,v的三元一次方程组，求解

## 算法：法线模型变换矩阵
- 结论$$M=\text{inverse}(\text{Model})^T$$

## 技术：Bloom
- https://catlikecoding.com/unity/tutorials/advanced-rendering/bloom/
- https://blog.csdn.net/jxw167/article/details/85691401
- https://zhuanlan.zhihu.com/p/161887607
- 算法分三步
   1. 亮度过滤：保留高光，其他去除
      - 0.2125 * R + 0.7154 * G + 0.0721 * B
   2. 模糊
      - 高斯或mipmap 
   3. 合并

## 技术：ray cast求交
- 先coarse后fine，提升效率
  - Coarse
    - AABB、OBB、k-dop
  - Fine
    - 三角形求交
- 建树
  - bsp
  - bvh
  - kd-tree

## 技术：文本渲染
### 参考
- [游戏中的Text Rendering(文本渲染) - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/143871184)

### 概念
- Character 字符：抽象的概念，代表了一个文字
- Glyph 字形：具体的现状
  - Anchor 用于对齐文字的锚点，比如agf三个字母不能居中对齐，需要引入锚点
  - Advance 非等宽字符需要的，光标移动距离
- Font 字体：从Character到Glyph的映射

### 文本表示
#### Font Atlas
- 所有文字存储在一张图集上
  - ![](https://pic4.zhimg.com/80/v2-c738674d74ee141e68376d56927dc0fb_720w.webp)
- 问题：放大后会模糊

#### SDF
- 依旧使用图集，不过存储的是SDF
- 优点：放大不会模糊，很多文字特效容易实现
- [msdf-atlas-gen 生成Font Atlas的工具](https://github.com/Chlumsky/msdf-atlas-gen)
- [Font Texture Generator Online](https://evanw.github.io/font-texture-generator/)
- [SDF渲染](https://www.cnblogs.com/mmc1206x/p/11965064.html)

## 文本渲染
- 每个字符生成一个矩形Mesh和其对应字体的UV
- 模拟光标的移动，每个Mesh按顺序排列在2D空间里
- 模型变换平移到3D空间中

## 技术：物理模拟
### 概念
  #### 物体分类
  - 静态：不会移动
  - 动态：可以移动，受力影响
  - Trigger：用于触发，不会阻挡
  - Kinematic：不遵守动力学，由游戏玩法逻辑控制

  #### 物体形状
  - 球
  - 胶囊
  - 盒子
  - 凸多面体（凸包、水密）
  - 三角面（水密，一般只能静态）
  - 高度场

  #### 属性/物理材质
  - 质量/密度
  - 质心
  - 摩擦力Friction与弹性Restitution

  #### 力Force、冲量Impulse
  - 显式欧拉，隐式欧拉，半隐式欧拉（推荐、稳定易算）

  #### 刚体动力学Rigid body Dynamics
  - 姿态 Orientation 矩阵、四元数
  - 角速度 Angular velocity
    - 三维，同时表示轴向和大小
  - 角加速度 Angular acceleration
  - 转动惯量/张量 Inertia tensor
  - 角动量 Angular momentum
  - 力矩 Torque

  #### 碰撞检测
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

  #### 碰撞处理
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

  #### Scene Query
  - Raycast
    - 光线与物体求交
    - Multiple hits
    - Closest hit
    - Any hit
  - Sweep
    - 类似Raycast，用一个体来扫描
  - Overlap
    - 给定一个形状，判断哪些有碰撞

  #### 优化
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

### 刚体
- 刚体：刚体:在外力作用下，形状和大小都不发生变化的物体
  - 刚体即看作任意两质点间距离保持不变的特殊质点组
  - 理想模型

#### 刚体运动学
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

#### 刚体动力学
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


#### 四元数
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


### 碰撞处理
- Penalty Force
  - Quadratic
  - Log
- Impulse
- Constraint

### 实现
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
    - 瞬时角加速度
      - 角速度和角加速度可以叠加
  - 更新速度和位置
  - 碰撞检测
  - 解决约束
    - Penalty force效果不好
  - 渲染

---

## 色彩空间、PBR
- ![色彩空间为什么那么空？](https://www.bilibili.com/video/BV19e4y1y7Mo)

### Tonemap