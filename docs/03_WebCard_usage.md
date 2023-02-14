
---

# **WebCard: Usage examples.**

---

## **Introduction**

The idea is that a Web developer, who wants to have low level access to smart cards in the machine where the browser is running, would add a **`webcard.js`** file to the page.

That script would in turn check if the extension is installed, and if not, prompt the user to install it.

```html
<head>
    <script type="application/javascript" src="webcard.js" defer></script>
    <script type="application/javascript" src="other-site-scripts.js" defer></script>
</head>
```

```js
// "content.js" script runs first (after given webpage has been loaded).
// Then, "webcard.js" is launched.
// Finally, some website-speicfic script can ba launched in a following way:
if (typeof navigator.webcard !== 'undefined')
{
    // Check if the "WebCard" extension has been installed.
    console.log(`Is WebCard ready? ${navigator.webcard.isReady}`);
}
```

&nbsp;

---

## **Using the extension**

The extension adds a **`webcard`** object to **`window.navigator`**, and through it **`navigator.webcard.readers()`** provides a list of the smart card **`Reader`** objects available in the machine.

### Reader object

Each **`Reader`** has the following fields:

* **`index: number`**

    * 0-based indexing, valid as long as the list has not been changed (*Native App has not reported any plugged/unplugged devices*).

* **`name: string`**

    * Smart Card Reader name (*usually unique for current list, because two readers of the same type/manufacturer will have different sub-index attached at the end of the name*).

* **`atr: string`**

    * non-empty string if there is a card inserted in the reader.

**`Reader`** has the following methods:

* **`connect(shared?: boolean): Promise`**

    * establishes a connection with the inserted card. **`shared`** is an optional argument to indicate if the connection should be exclusive or not. The default is `true`.

    * On success (fulfilled promise), returns the ATR of the inserted card.

* **`transcieve(apdu: string): Promise`**

    * sends the APDU passed as a hexidecimal string.

    * On success (fulfilled promise), returns the cAPDU response in a form of hexadecimal string.

* **`disconnect(): Promise`**

    * closes the connection with this reader.

    * Fulfilled promise indicates success.

### WebCard object

**`navigator.webcard`** has the following fields:

* **`isReady: boolean`**

    * Value set to `true` if the extension is installed in the web browser.

* **`pendingRequests: Map`**

    * Map of requests created in `send()` function, that must be resolved once a response from Native App is received.

**`navigator.webcard`** has the following methods:

* **`randomUid(): string`**

    * genarates an unique identifier, based on current datetime and a randomly generated number.

* **`send(cmdIdx: number, otherParams?: Array<any>): Promise`**

    * posts a `webcard request` message to the `content script`. The `i` field of the message is generated once when the `send()` function is called. `cmdIdx` is a required parameter (refer to **Native Messages** section below).

    * The result of a fulfilled promise depends on the type of command sent to the Native App.

* **`sendEx(cmdIdx: number, otherParams?: Array<any>): Object`**

    * same as **`send()`**, but rather than just returning a `Promise`, returns an object containing:

        ```
        { promise: Promise, uid: string }
        ```

* **`async readers(): Object`**

    * Asynchronous function (contains a *"Promise race"*). Returns an object containing three version categories:

        * `addon`: reported by content-script add-on;

        * `app`: reported by the running Native App;

        * `latest`: hardcoded in `webcard.js` page-script;

    ```
    { addon: string, app: string, latest: string }
    ```

* **`readers(): Promise`**

    * Sends the `{ webcard: 'request', i: ???, c: 1 }` message to the content-script, which passes `{ i: ???, c: 1 }` to the **Native App**.

    * On success (fulfilled promise), returns the list of Smart Card Readers connected to the machine.

* **`responseCallback(msg: object)`**

    * Deals with Native App responses. Can call user-defined callbacks for specific events. Should not be called directly!

### User-defined (optional) callbacks for WebCard

These fields can be assigned with user-defined functions (*Native App event callbacks*).

* **`cardInserted(readerId: number, atr: string)`**

    * Called when an ICC is inserted to a known reader.

* **`cardRemoved(readerId: number)`**

    * Called when an ICC is removed from a known reader.

* **`readersConnected(listOfNames: Array<string>)`**

    * Called when Native App detects a new smart card reader. **User should then issue the `navigator.webcard.readers()` call to update a local list of readers!** `listOfNames` lists the names of just-connected readers (*usually only one entry on the list*).

* **`readersDisconnected(listOfNames: Array<string>)`**

    * Called when Native App detects smart card reader removal. **User should then issue the `navigator.webcard.readers()` call to update a local list of readers!** `listOfNames` lists the names of just-disconnected readers (*usually only one entry on the list*).

&nbsp;

----

## Native Messages

### JSON messages sent to Native App

```
{ i: string, c: number, r: integer, a: string, p: number }
```

* `i`: unique message identifier.

* `c`: command:

    * `1` => list readers

    * `2` => connect

    * `3` => disconnect

    * `4` => transcieve

    * `10` => check version

* `r`: index of a reader in readers list.

    * send only for commands `2` and `3`.

* `a`: hexadecimal cAPDU to send to the card.

    * sent only for command `3`.

* `p`: additional parameter.

    * for command `2` => share mode (`2` or `1`) for connect; otherwise unused.

### JSON messages received from Native App

```
{ i: string, e: number, r: number, n: Array<string>, d: Array | string }
```

* `i`: unique message identifier, to link the response.

    * Empty (undefined) string on reader events.

* `e`: reader event:

    * `1` => card inserted

    * `2` => card removed

    * `3` => reader connected

    * `4` => reader disconnected

* `r`: reader index for reader events `1` and `2`.

* `n`: reader names for events `3` and `4`.

* `d`: data associated with the response:

    * if `c = 1` was sent, it is an array of reader objects, each entry in the array contains:

        * `n: string` => reader's name.

        * `a: string` => card's ATR (if a card exists, otherwise an empty string).

    * if `c = 2` was sent, or `e = 1` was received => card's ATR (Answer to Reset).

    * if `c = 4` was sent => hexadecimal rAPDU.

### Messages grouped by commands

* Command `0`: Just pinging the **Native App**:

    * Request:

        * `c: number = 0`

        * `i: string` => unique request ID.

    * Response:

        * `i: string` => matches the request ID.

* Command `1`: **List readers**.

    * Request:

        * `c: number = 1`.

        * `i: string` => unique request ID.

    * Response:

        * `i: string` => matches the request ID.

        * `d: Array` => list of readers, in the same order as in **Native App**'s list of reader states. Each object on the list contains two properties:

            * `n: string` => Smart Card Reader's name;

            * `a: string` => card's ATR (if a card exists, otherwise an empty string);

* Command `2`: **Connect** to a reader (must have any card inserted).

    * Request:

        * `c: number = 2`.

        * `i: string` => unique request ID.

        * `r: number` => reader's index (from the list of readers).

        * `p: number` => `2`: shared mode, `1`: exclusive mode.

    * Response:

        * `i: string` => matches the request ID.

        * `d: string` => card's ATR.

* Command `3`: **Disconnect** from a reader.

    * Request:

        * `c: number = 3`

        * `i: string` => unique request ID.

        * `r: number` => reader's index (from the list of readers).

    * Response (*required to resolve a JavaScript Promise*):

        * `i: string` => matches the request ID.

* Command `4`: **Transcieve** (*connection must have been established*).

    * Request (*transmit APDU*):

        * `c: number = 4`

        * `i: string` => unique request ID.

        * `r: number` => reader's index (from the list of readers).

        * `a: string` => hexadecimal cAPDU (each byte represented as two characters: `0-9,A-F`).

    * Response (*receive APDU*):

        * `i: string` => matches the request ID.

        * `d: string` => card's ATR.

* Command `10`: **Version check**.

    * Request:

        * `c: number = 10`

        * `i: string` => unique request ID.

    * Response:

        * `i: string` => matches the request ID.

        * `verNat: string = '0.4.0'`.

### Messages grouped by events

* Event `1`: **Card inserted**.

    * `e: number = 1`.

    * `r: number` => reader's index (from the list of readers).

    * `d: string` => card's ATR.

* Event `2`: **Card removed**.

    * `e: number = 2`.

    * `r: number` => reader's index (from the list of readers).

* Event `3`: **New reader connected**.

    * `e: number = 3`.

    * `n: Array<string>` => names of newly detected readers (*this array will usually contain just one entry*).

* Event `4`: **ANy reader disconnected**.

    * `e: number = 4`.

    * `n: Array<string>` => names of just disconnected readers (*this array will usually contain just one entry*).

* It is advised to refresh a local list of readers once events `3` or `4` have been received, because "current" reader indices (from a web script) will no longer match the list known to the Native App...

&nbsp;

----

## WebCard testing

Having the WebCard (Web Browser extension + Native App) installed, to open the Test Page:

* Launch a HTTP sever.

    * for example, running **`python3 -m http.server`** in the **`http_test`** folder.

* Navigate to **`https://localhost:8000`**.

&nbsp;

----
