
---

# **WebCard: Installation instructions.**

---

## **Prerequisites**

* Install a ***Web Browser*** of your choice. Currently supported Web Browsers are:

    * **Mozilla Firefox**

    * **Chromium**

    * **Google Chrome**

    * **Microsoft Edge**

* Build the ***Native Messaging Helper Application***.

    * Refer to the [Building instructions](01_WebCard_building.md).

&nbsp;

---

## **Sideloading the browser extension**

This development version requires manual installation of the unpacked extension.

### **Chromium** & **Google Chrome**

* Navigate to **`chrome://extensions`**.

* Enable **`Developer mode`** (toggle the switch in the top-right corner).

* Click on the **`[Load unpacked]`** button (top-left).

    * Select the **`extension/chromium`** directory (from the downloaded repository).

* Copy the 32-character **`ID`** value.

    * In a case when you can't see the whole value, click on the **`[Details]`** button.

* Open the **`install/ID_CHROMIUM.txt`** text file in a notepad, paste the 32-character ID, save changes.

### **Microsoft Edge**

* Navigate to **`edge://extensions`**.

* Enable **`Developer mode`** (toggle the switch on the left panel).

* Click on the **`[Load unpacked]`** button (top side).

    * Select the **`extension/chromium`** directory (from the downloaded repository).

* Copy the 32-character **`ID`** value.

    * In a case when you can't see the whole value, click on the **`[Details]`** button.

* Open the **`install/ID_CHROMIUM.txt`** text file in a notepad, paste the 32-character ID, save changes.

### **Mozilla Firefox**

* Navigate to **`about:debugging#/runtime/this-firefox`**.

* Click on the **`[Load Temporary Add-on]`** button (top side).

    * Select the **`extension/firefox/manifest.json`** file (from the downloaded repository).

    * The **`install/ID_FIREFOX.txt`** text file is already filled-in with **`webcard@cardid.org`** value, so you don't need to do anything else.

* Add-on loaded this way will disappear every time you restart Firefox! To install an unsigned zipped extension, you must use the **Firefox Developer Edition**:

    * Add all files from the **`extension/firefox`** directory to a ZIP archive, e.g. **`extension/firefox/firefox.zip`**.

    * In **Firefox Developer Edition**, navigate to **`about:config`**.

        * Set the **`xpinstall.signatures.required`** variable to **`false`**.

    * Then, navigate to **`about:addons`**.

    * Drag'n'drop the **`firefox.zip`** archive onto the **Firefox** window, accept the unsigned add-on installation.

&nbsp;

---

## **Installing the Native Messaging Host**

1. Starting from the repository directory, navigate to the **`install`** directory.

    ```
    cd install
    ```

2. Launch the **`install`** script.

    * on **Microsoft Windows**:

        ```
        install
        ```

    * on **Linux** and **macOS**:

        ```
        chmod u+x install.sh
        ./install.sh
        ```

3. Follow the on-screen instructions. The script will copy/create required files (and update registry values in case of **Microsoft Windows**).

&nbsp;

---

## **Enabling the "Personal Computer / Smart Card" services**

This step is required for **Linux** users only.

* To install **`PCSC lite`** (executable library and resource manager), type:

    ```
    sudo apt install libpcsclite1 pcscd pcsc-tools
    ```

* To launch the **`PC/SC Resource Manager (Daemon)`**, type:

    ```
    sudo service pcscd restart
    ```

&nbsp;

----
