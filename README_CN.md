# LuaProto

[English](README.md) | [中文](README_CN.md)

> 使用 protobuf 的 reflection 机制，给 lua 提供访问宿主 proto 协议的能力

## 工具背景
在之前一个知名二次元 ip 游戏的项目时，项目组使用 c++ 开发，使用 protobuf 作为内/外部通信协议，自然按常规方法编译出 pb.cc pb.h 文件集成到项目中。此时想在 lua 中操作 proto 就不能使用 [pbc](https://github.com/cloudwu/pbc) 或 [lua-protobuf](https://github.com/starwing/lua-protobuf)，会造成 proto 信息冗余以及版本差异的问题。

## 注意
代码只在该项目中经过实测，特定的 protobuf 版本和平台。并不能保证完全没问题，请谨慎使用。
