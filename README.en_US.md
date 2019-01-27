<img width="350" src ="https://raw.githubusercontent.com/polarphp/polarphp/master/assets/images/polarphp.png?sanitize=true"/>

Read this in other languages: **English**, [简体中文](README.zh_CN.md).

## Why did I launch the `polarphp` project?

With the strong rise of `Go` and `NodeJS`, the market share of `PHP` has gradually been eroded, and the `PHP` official still sticks to the Web programming field. The more things you want to keep, the more you can't keep it. `polarphp` draws on the related features of `NodeJS` and `Go` to repackage `zendVM`, remove some of the old deprecated features and strong `Web` attributes of `PHP`, by implementing a new set of runtime framework `libpdk` , the `PHP` language is built into a true universal scripting language, empower `PHP`, let it have asynchronous programming, coroutine, thread, built-in `unicode` support, standard file `IO` and so on. So that the `PHP` programmer can not only do the `web` application, but also face the real server application. `polarphp` is not a new language, but a runtime container for the `PHP` language.

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
- [CODING_STANDARDS](docs/CODING_STANDARDS)
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
