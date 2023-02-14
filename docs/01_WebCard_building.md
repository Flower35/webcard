
---

# **WebCard: Building instructions.**

---

## **Prerequisites**

Here is a list of required utilities:

* Building environment, such as:

  * default systems shells

    * ( Windows ): **`CMD.exe`**

    * ( Linux, macOS ): **`bash`**

  * ( Windows only ): **`MSYS2`**

* Build automation tool:

    * ( Windows ): **`mingw32-make`**

    * ( Linux, macOS ): **`make`**

* ( Windows only ) Resources converter: **`windres`**

* C compiler and linker:

    * ( Windows ): **`mingw64-gcc`**

    * ( Linux ): **`gcc`**

    * ( macOS ): **`clang`**

* WinSCard (*"Personal Computer/Smart Card"*) library:

    * ( Windows ): **`WinSCard.dll`**

    * ( Linux, macOS ): **`PCSC lite`**

&nbsp;

---

## **Obtaining tools**

Instructions for selected Operating Systems.

### **Microsoft Windows (32-bit and 64-bit)**

* **`MSYS2`** (*Software Distribution and Building Platform for Windows*) can be downloaded from: https://www.msys2.org/

    * Install and launch the `MSYS2` environment.

    * To install **`make`**, **`windres`** and **`gcc`** for 32-bit architectures, type:

        ```
        sudo pacman -Sy
        sudo pacman -S mingw-w64-i686-make mingw-w64-i686-gcc
        ```

    * To install **`make`**, **`windres`** and **`gcc`** for 64-bit architectures, type:

        ```
        sudo pacman -Sy
        sudo pacman -S mingw-w64-x86_64-make mingw-w64-x86_64-gcc
        ```

* If you choose not to use the **`MSYS2`** environment, then **`mingw32-make.exe`**, **`windres.exe`** and **`gcc.exe`** can be downloaded from https://www.mingw-w64.org/downloads/#mingw-builds

    * After extracting the files to a directory of your choice (*suggested: `C:\mingw64\`*), remember to add the **`bin`** directory to the **`PATH`** environment variable!

* The **`WinSCard.dll`** library is already bundled with the **Windows** Operating System.

### **Linux distributions (32-bit and 64-bit)**

* Launch terminal (shell).

    * To install **`make`** and **`gcc`**, type:

        ```
        sudo apt update
        sudo apt install make gcc
        ```

    * To install **`PCSC lite`** (development files), type:

        ```
        sudo apt install libpcsclite-dev
        ```

    * To fix the include directives in **`PCSC lite`** library (searching in current directory instead of **`INCLUDE`** directory), type:

        ```
        sudo perl -p -i.bak -e 's/#include <pcsclite.h>/#include "pcsclite.h"/g' /usr/include/PCSC/winscard.h
        sudo perl -p -i.bak -e 's/#include <wintypes.h>/#include "wintypes.h"/g' /usr/include/PCSC/pcsclite.h
        ```

### **macOS (Darwin)**

* All required utilities (**`make`**, **`clang`** and **`PCSC lite`**) should be already bundled with the **macOS** (MacOS X) Operating System.

&nbsp;

----

## **Building the `webcard` project.**

1. Starting from the repository directory, navigate to the **`native`** directory.

    ```
    cd native
    ```

2. Launch **`make`**.

    * on **Microsoft Windows**:

        ```
        mingw32-make release -B
        ```

    * on **Linux** and **macOS**:

        ```
        make release -B
        ```

3. The **`webcard`** executable (*Native Messaging Helper Application*) will appear in **`native/out`** directory.

&nbsp;

----
