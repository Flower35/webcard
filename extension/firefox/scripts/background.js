// "Smart Card Extension (WebCard)": "background.js"

/******************************************************************************/

const hostName = 'org.cardid.webcard.native';

let nativePort;
let contentPorts = new Map();

/******************************************************************************/

function nativePortOnMessage(msg) {
  console.log("<< " + JSON.stringify(msg));
  if (msg.i) {
    /* Message response with specific unique identifier. */
    let destination = msg.i.match(/(\d+)\.(.+)/);
    let contentPort = contentPorts.get(destination[1]);
    if (contentPort) {
      msg.i = destination[2];
      contentPort.postMessage(msg);
    }
  } else {
    /* Message originating from the native app. */
    /* Broadcast to all content ports. */
    contentPorts.forEach((port) => {
      port.postMessage(msg);
    });
  }
}

/******************************************************************************/

function connectWithNativeApp(contentPort) {
  nativePort = browser.runtime.connectNative(hostName);
  nativePort.onDisconnect.addListener((p) => {
    nativePort = null;
    if (p.error) {
      info = `NativeApp disconnected: ${p.error.message}`;
      console.log(info);
      contentPort.postMessage({webcard: 'alert', info: info});
    }
  });
  if (nativePort) {
    nativePort.onMessage.addListener(nativePortOnMessage);
  }
}

/******************************************************************************/

browser.runtime.onConnect.addListener((contentPort) => {
  // Accept `browser.runtime.connect()` only from the "Smart Card Extension (WebCard)".
  if (contentPort.name !== 'webcard') {
    return;
  }
  contentPorts.set(contentPort.sender.tab.id.toString(), contentPort);
  contentPort.onMessage.addListener((msg) => {
    msg.i = contentPort.sender.tab.id + '.' + msg.i;
    console.log(">> " + JSON.stringify(msg));
    if (!nativePort) {
      connectWithNativeApp(contentPort);
    }
    if (nativePort) {
      nativePort.postMessage(msg);
    }
  });
  if (!nativePort) {
    connectWithNativeApp(contentPort);
  }
});

/******************************************************************************/
