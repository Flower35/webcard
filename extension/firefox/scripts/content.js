// "Smart Card Extension (WebCard)": "content.js"

/******************************************************************************/

const webcardVersion = '0.3.1';

let backgroundPort;

let banner = document.createElement('div');
banner.id = "webcard-install-banner";
document.documentElement.appendChild(banner);

/******************************************************************************/

function SafeTargetOrigin() {
  let href = window.location.href;
  return href.startsWith('file://') ? '*' : href;
}

/******************************************************************************/

function backgroundPortOnMessage(msg) {
  if (msg.webcard && (msg.webcard == 'alert')) {
    window.alert(msg.info);
  } else {
    // Requested version check?
    if (msg.verNat) {
      msg.verExt = webcardVersion;
    }
    // Forward the response back to the a page script on some tab.
    msg.webcard = 'response';
    window.postMessage(msg, SafeTargetOrigin());
  }
}

/******************************************************************************/

function windowWebcardRequest(event) {
  // Called after any `window.postMessage()` (content script, page scripts).
  // We want to only accept messages from ourselves.
  if (event.source != window) {
    return;
  }
  if (event.data.webcard && (event.data.webcard == 'request')) {
    if (!backgroundPort) {
      backgroundPort = browser.runtime.connect({name: "webcard"});
      backgroundPort.onMessage.addListener(backgroundPortOnMessage);
    }
    delete event.data.webcard;
    backgroundPort.postMessage(event.data);
  }
}

window.addEventListener("message", windowWebcardRequest, false);

/******************************************************************************/
