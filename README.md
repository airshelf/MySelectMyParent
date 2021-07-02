# SelectMyParent 魔改版 | 降权工具


### 说明
SelectMyParent是一款降权工具

降权通常是指从system权限降到普通用户权限（从管理员权限降到普通用户权限比较简单，方法很多），往往是为了操作当前用户的文件内容（如捕获桌面、操作注册表等）

本工具的功能，是将从system权限到admin权限

相比于原版SelectMyParent，本工具新增了下面几个功能

[+] 自动判断属于Admin的进程
[+] 自动判断当前用户，如果不是system，则会以本进程的身份运行
[+] 通过合并的方式拼接待运行程序，运行时释放


### 使用方法

vs2019编译测试，编译完成

输入命令
```
PluginPacker.exe abc.exe（这个是你要用的程序）
```

然后会在同级目录下产生abc_protected.exe

然后abc_protected.exe已经可以单独运行，且携带SelectMyParent的功能

