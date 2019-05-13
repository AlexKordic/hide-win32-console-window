# hide-win32-console-window
Tool for hiding "black square" of the console window.

Using Windows API we can start new process, a console application, and hide its "black" window. This can be done at process creation and avoid showing "black" window at all.

In [CreateProcess](https://docs.microsoft.com/en-us/windows/desktop/api/processthreadsapi/nf-processthreadsapi-createprocessw) function the `dwCreationFlags` parameter can have [CREATE_NO_WINDOW](https://docs.microsoft.com/en-us/windows/desktop/ProcThread/process-creation-flags) flag:

    The process is a console application that is being run
    without a console window. Therefore, the console handle
    for the application is not set. This flag is ignored if
    the application is not a console application

Here is a link to [hide-win32-console-window executable](https://github.com/AlexKordic/hide-win32-console-window/releases) using this method and [source code](https://github.com/AlexKordic/hide-win32-console-window/blob/master/batchscript_starter.cpp).

There is open question: **what to do with program's output when its window does not exist?** What if exceptions happen? Not a good solution to throw away the output. `hide-win32-console-window` uses anonymous pipes to redirect program's output to file created in current directory.

## Usage ##

**batchscript_starter.exe** *full/path/to/application* [*arguments to pass on*]

## Example running python script ##

    batchscript_starter.exe c:\Python27\python.exe -c "import time; print('prog start'); time.sleep(3.0); print('prog end');"

The output file is created in working directory named `python.2019-05-13-13-32-39.log` with output from the python command:

    prog start
    prog end

## Example running command ##

    batchscript_starter.exe C:\WINDOWS\system32\cmd.exe /C dir .

The output file is created in working directory named `cmd.2019-05-13-13-37-28.log` with output from CMD:


     Volume in drive Z is Storage
     Volume Serial Number is XXXX-YYYY
    
     Directory of hide_console_project\hide-win32-console-window
    
    2019-05-13  13:37    <DIR>          .
    2019-05-13  13:37    <DIR>          ..
    2019-05-13  04:41            17,274 batchscript_starter.cpp
    2018-04-10  01:08            46,227 batchscript_starter.ico
    2019-05-12  11:27             7,042 batchscript_starter.rc
    2019-05-12  11:27             1,451 batchscript_starter.sln
    2019-05-12  21:51             8,943 batchscript_starter.vcxproj
    2019-05-12  21:51             1,664 batchscript_starter.vcxproj.filters
    2019-05-13  03:38             1,736 batchscript_starter.vcxproj.user
    2019-05-13  13:37                 0 cmd.2019-05-13-13-37-28.log
    2019-05-13  04:34             1,518 LICENSE
    2019-05-13  13:32                22 python.2019-05-13-13-32-39.log
    2019-05-13  04:55                82 README.md
    2019-05-13  04:44             1,562 Resource.h
    2018-04-10  01:08            46,227 small.ico
    2019-05-13  04:44               630 targetver.h
    2019-05-13  04:57    <DIR>          x64
                  14 File(s)        134,378 bytes
                   3 Dir(s)  ???,???,692,992 bytes free

## Example shortcut for running .bat script ##

[![Shortcut for starting windowless .bat file][1]][1]

`Target` field:

    C:\batchscript_starter.exe C:\WINDOWS\system32\cmd.exe /C C:\start_wiki.bat

Directory specified in `Start in` field will hold output files.


  [1]: https://i.stack.imgur.com/MRzAP.jpg
