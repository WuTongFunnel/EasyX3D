基于 EasyX 图形库开发的 3D 软件渲染引擎。

A 3D software rendering engine developed based on the EasyX graphics library. 

采用C++ 实现软件渲染管线，完成 3D 到 2D 的透视投影、坐标变换与基础图形绘制。其中鼠标控制、全屏切换、WASD 移动、鼠标滚轮等输入处理相关函数由豆包编写，其余功能均由本人独立开发。

Implements a pure software rendering pipeline in C++, including 3D-to-2D perspective projection, coordinate transformation and basic graphics drawing. Functions related to mouse control, fullscreen switching, WASD movement and mouse wheel input are written by Doubao. All other features are independently developed by myself.

本项目基于 EasyX 2D 图形库与 Visual Studio 2022 开发

This project is developed based on the EasyX 2D Graphics Library and Visual Studio 2022.

使用说明

1.安装 Visual Studio

2.安装 EasyX 图形库：https://easyx.cn

3.打开 .sln 解决方案直接编译运行


Usage Instructions

1.Install Visual Studio

2.Install the EasyX Graphics Library: https://easyx.cn

3.Open the .sln solution and compile to run directly

介绍视频：

introduction video:

<img width="2560" height="1600" alt="image" src="https://github.com/user-attachments/assets/f7733e94-47b0-458f-a03f-545fea236c3c" />

更新：2.0版本把原来的线框渲染逻辑，改成了顶点着色器和片段着色器逻辑，现在的渲染是以三角形为单位了，可以给三角形上色，但是我没做深度缓冲
