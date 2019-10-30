<img width="350" src ="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/polarphp.png"/>

## 让 PHP 语言变得更加美好

<br/>

Read the English version of this document: [English](README.en_US.md)

阅读本文档其他语言版本: [English](README.en_US.md), **简体中文**.

## 为什么要做 polarphp 项目

随着`Go`和`NodeJS`的强势崛起，`PHP`的市场份额逐渐被蚕食，而`PHP`官方仍然坚守在`Web`编程领域，有些东西越是想守住就越守不住。`polarphp`借鉴`NodeJS`和`Go`的相关特性对`zendVM`重新封装，去掉`PHP`一些古老弃用的特性和强`Web`属性，通过实现一套新的运行时框架`libpdk`，将`PHP`语言打造成为一门真正的通用性脚本语言，赋能`PHP`，让其拥有异步编程，协程，线程，内置的`unicode`支持，标准的文件`IO`等等特性，让`PHP`程序员不仅仅能做`web`应用，也能从容面对真正的服务端应用。`polarphp`不是一门新的语言，而是`PHP`语言的除官方外的一个新的编译器及其运行时。

## 主要特性

- [ ] 兼容最新的`PHP`语言标准，移除废弃语言特性
- [ ] 内置`unicode`字符标准支持
- [ ] 全功能型运行时库支持，支持异步编程，多线程和协程等等编程模式
- [ ] 内置包管理器
- [ ] 内置文档生成器

## 开发计划

因为开发资源有限，开发计划暂定如下：

1. 实现自己的`PHP`编译器前端
2. 语言支持项目，语言测试框架，移植`LLVM`项目的`lit`测试框架
3. 使用`phplit`回归测试框架完成对`polarphp`编译器前端的测试
4. 定义`polarvm`的指令集，完成虚拟机的基础架构
5. 实现完整的虚拟机规范的指令集
6. 完成`polarphp` AST 到指令集的编译，暂时不定义 IR 层
7. 使用`phplit`对语言结构到指令集所有编译模块的测试
8. 实现一个最小化的运行时，暂时使用`PHP`内置的`GC`
9. 实现`PHP`语言标准库`libpdk`的底层架构 (`cpp`部分)
10. 实现人性化安装，尽量以最少的步骤进行`polarphp`的安装
11. 实现包管理器
12. 实现语言配套小工具，比如文档生成工具等等
13. 发动社区，实现一个功能完备的`PHP`标准库 (使用`PHP`代码配合`libpdk`底层支持进行实现)

## 开始体验

### 克隆`polarphp`项目库
```
git clone https://github.com/polarphp/polarphp.git
cd polarphp
git submodule init
git submodule update
git checkout v0.0.1-alpha
```
### 运行脚本
```
./devtools/scripts/build_polarphp.sh
```
这个时候脚本开始编译相关镜像，耗时比较长，请您耐心等待。等待编译完成，您运行：
```
docker images
```
这个时候请确认在输出中有如下镜像：
1. polarphp_base_env
2. polarphp_debug

如果没有问题，我们开始测试`polarphp`是否在镜像中正常运行。
```
docker run --rm -it polarphp_debug
```
进入容器后，输入我们的`polarphp`命令行程序
```
polar --version
```
如果您得到下面的输出：
```
polarphp 0.0.1-git (built: 2019-01-27 12:22)
Copyright (c) 2016-2018 The polarphp foundation (https://polar.foundation)
Zend Engine v3.3.0-dev, Copyright (c) 1998-2018 Zend Technologies
```
恭喜您，您已经成功编译了`polarphp`运行时环境。
在编译镜像的时候，我们在`~/temp/`文件夹中放入了一个测试脚本
```php
if (function_exists('\php\retrieve_version_str')) {
    echo "version str: " . \php\retrieve_version_str() . "\n";
}

if (function_exists('\php\retrieve_major_version')) {
    echo "major version: " . \php\retrieve_major_version() . "\n";
}

if (function_exists('\php\retrieve_minor_version')) {
    echo "minor version: " . \php\retrieve_minor_version() . "\n";
}

if (function_exists('\php\retrieve_patch_version')) {
    echo "patch version: " . \php\retrieve_patch_version() . "\n";
}

```
您可以运行一下命令：
```
polar ~/temp/main.php
```
如果没有错误，您将得到下面的输出：
```
version str: polarphp 0.0.1-git
major version: 0
minor version: 0
patch version: 1
```
感谢您测试`polarphp`，有什么问题，请扫描下面的微信二维码进群交流。

## 社区
目前我们暂时只针对中国的用户，所以采用了微信和`QQ`群的交流方式，下面是二维码，有兴趣的同学可以扫码加入：

> PS：扫码请注明来意，比如：学习`polarphp`或者`PHP`爱好者

</div>
<table>
  <tbody>
    <tr>
      <td align="center" valign="middle">
        <a href="https://www.oschina.net/" target="_blank">
         <img width = "200" src="https://raw.githubusercontent.com/qcoreteam/zendapi/master/assets/images/qq.png"/>
        </a>
      </td>
      <td align="center" valign="middle">
        <a href="https://gitee.com/?from=polarphp.org" target="_blank">
          <img width = "200" src="https://raw.githubusercontent.com/qcoreteam/zendapi/master/assets/images/wechat.png"/></div>
        </a>
      </td>
    </tr><tr></tr>
  </tbody>
</table>

### 目前有以下工作组

1. 语言核心团队
2. 标准库团队
3. 生态链项目团队
4. 文档团队
5. 官方网站维护团队

## 授权协议

`polarphp`在`php`语言项目之上进行二次开发，遵守`php`项目的协议，详情请看：[项目协议](/LICENSE)

## 贡献代码引导
===========================
- [CODING_STANDARDS](CODING_STANDARDS)
- [README.GIT-RULES](docs/README.GIT-RULES)
- [README.MAILINGLIST_RULES](docs/README.MAILINGLIST_RULES)
- [README.RELEASE_PROCESS](docs/README.RELEASE_PROCESS)

## 特别感谢
<!--特别感谢开始-->
<table height="100">
  <tbody>
    <tr>
      <td align="center" valign="middle">
        <a href="https://www.oschina.net/" target="_blank">
          <img width="177px" src="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/osc.svg?sanitize=true">
        </a>
      </td>
      <td align="center" valign="middle">
        <a href="https://gitee.com/?from=polarphp.org" target="_blank">
          <img width="177px" src="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/gitee.svg?sanitize=true">
        </a>
      </td>
      <td align="center" valign="middle">
        <a href="http://www.hacknown.com/" target="_blank">
          <img width="177px" src="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/hacknown.svg?sanitize=true">
        </a>
      </td>
    </tr><tr></tr>
  </tbody>
</table>
<!--特别感谢结束-->
