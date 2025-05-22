# LuaProto

[English](README.md) | [中文](README_CN.md)

> A tool that provides Lua with access to host proto protocols using protobuf's reflection mechanism

## Background
During a previous project for a well-known ACG IP game, the team used C++ for development and protobuf for internal/external communication protocols, naturally integrating the compiled pb.cc and pb.h files into the project. In this context, using [pbc](https://github.com/cloudwu/pbc) or [lua-protobuf](https://github.com/starwing/lua-protobuf) for handling proto in Lua would cause proto information redundancy and version inconsistency issues.

## Note
The code has only been tested in this specific project with particular protobuf versions and platforms. Complete reliability cannot be guaranteed, please use with caution.