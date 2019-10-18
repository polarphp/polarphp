<img width="350" src ="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/polarphp.png?sanitize=true"/>

## Let the PHP language be great again 

## Why did I launch the `polarphp` project?

With the strong rise of `Go` and `NodeJS`, the market share of `PHP` has gradually been eroded, and the `PHP` official still sticks to the Web programming field. The more things you want to keep, the more you can't keep it. `polarphp` draws on the related features of `NodeJS` and `Go` to repackage `zendVM`, remove some of the old deprecated features and strong `Web` attributes of `PHP`, by implementing a new set of runtime framework `libpdk` , the `PHP` language is built into a true universal scripting language, empower `PHP`, let it have asynchronous programming, coroutine, thread, built-in `unicode` support, standard file `IO` and so on. So that the `PHP` programmer can not only do the `web` application, but also face the real server application. `polarphp` is not a new language, but a new compiler with runtime for the `PHP` language.

## Main features

- [ ] Compatible with the latest `PHP` language standard, removing obsolete language features
- [ ] Built-in unicode standard support
- [ ] Full-featured runtime library support, support for asynchronous programming, multi-threading and coroutine, etc.
- [ ] Built-in package manager
- [ ] Built-in document generator

## Development plan

Due to limited development resources, the development plan is tentatively scheduled as follows:

1. Compile `zend VM` with `cmake` to generate `polarphp` custom version of `PHP` language virtual machine
2. Language support project, language testing framework, porting `lit` test framework for `LLVM` project
3. Implement `polarphp` driver to implement `PHP` code from the command line
4. Regression testing of the `polarphp` virtual machine, tentatively running the language virtual machine related regression test of `PHP`
5. Implement the built-in function of `polarphp`
6. Publish the `docker` image of the core virtual machine
7. Integrate the `libpdk` runtime framework
8. Achieve user-friendly installation, try to install `polarphp` in a minimum of steps
9. Implementation package manager
10. Implement language widgets such as document generation tools, etc.

## Start the experience

> current we just test at macOS platform and you need run system in docker system.

### clone polarphp repository
```
git clone https://github.com/polarphp/polarphp.git
cd polarphp
git submodule init
git submodule update
git checkout v0.0.1-alpha
```
### run build script
```
./devtools/scripts/build_polarphp.sh
```
At this time, the script starts compiling the relevant image, which takes a long time. Please wait patiently. Wait for the compilation to complete, you run:
```
docker images
```
At this time, please confirm that there is the following docker image in the output:
1. polarphp_base_env
2. polarphp_debug

If there are no problems, we start testing whether polarphp is working properly in docker image.
```
docker run --rm -it polarphp_debug
```
After entering the container, enter our polarphp command line program:
```
polar --version
```
If you get the following output:
```
polarphp 0.0.1-git (built: 2019-01-27 12:22)
Copyright (c) 2016-2018 The polarphp foundation (https://polar.foundation)
Zend Engine v3.3.0-dev, Copyright (c) 1998-2018 Zend Technologies
```

Congratulations, you have successfully compiled the polarphp runtime environment. When compiling the image, we put a test script in the ~/temp/ folder.
```
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
You can run the following command:
```
polar ~/temp/main.php
```
If there are no errors, you will get the following output:

```
version str: polarphp 0.0.1-git
major version: 0
minor version: 0
patch version: 1
```
Thank you for testing polarphp, what is the problem, you can put it in the github issue list

## Community

At present, we only target Chinese users for the time being, so we use the communication method of WeChat and `QQ` group. The following is the QR code. Interested students can scan the code to join:

> PS:Please indicate the scan code, for example: learn `polarphp` or `PHP` enthusiasts

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

## Currently has the following working groups

1. Language core team
2. Standard library team
3. Ecosystem project team
4. Document team
5. Official website maintenance team

## License

`polarphp` is redeveloped on top of the `php` language project, following the agreement of the `php` project. For details, please see: [Project Agreement](/LICENSE)

## Contribution code guidance
===========================
- [CODING_STANDARDS](CODING_STANDARDS)
- [README.GIT-RULES](docs/README.GIT-RULES)
- [README.MAILINGLIST_RULES](docs/README.MAILINGLIST_RULES)
- [README.RELEASE_PROCESS](docs/README.RELEASE_PROCESS)

## Acknowledgement
<!--Acknowledgement begin-->
<table>
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
<!--Acknowledgement end-->
