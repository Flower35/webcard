// "webcard.js"
'use strict';

/******************************************************************************/

function SafeTargetOrigin() {
  let href = window.location.href;
  return href.startsWith('file://') ? '*' : href;
}

/******************************************************************************/

function RandomUid() {
  return Date.now().toString(36) + Math.random().toString(36).substr(2,5);
}

/******************************************************************************/

function Reader(index, name, atr) {
  this.index     = index;
  this.name      = name;
  this.atr       = atr;
  this.connected = undefined;

  this.connect = function(shared) {
    return navigator.webcard.ping(
      2,
      { r: this.index, p: shared ? 2 : 1 });
  }

  this.disconnect = function() {
    return navigator.webcard.ping(
      3,
      { r: this.index });
  }

  this.transcieve = function(apdu) {
    return navigator.webcard.ping(
      4,
      { r: this.index, a: apdu });
  }

  this.getAttribs = function() {
    return navigator.webcard.ping(
      5,
      { r: this.index });
  }
}

/******************************************************************************/

function WebCard() {
  this.nativeCallMap = new Map();
  console.log('Starting WebCard...');

  let banner = document.getElementById('webcard-install-banner');

  if (!banner) {
    banner = document.createElement('div');
    banner.id = "webcard-install-banner";
    banner.style = "width:100%;top:0;left:0;position:fixed;"
      + "background-color:rgba(250,250,210,0.75);border-bottom:1px solid gold;z-index:1030;"
    let html = '<p style="margin:8pt;font:10pt Helvetica;color:black;">';
    html += 'In order to provide a better login experience, this page uses your smart card.<br \\>Please install the latest ';
    html += '<a href="' + WebCard.remoteURL + '/webcard.msi">';
    html += 'Smart Card Browser Extension</a> and restart your browser.</p>';
    banner.innerHTML = html;
    document.body.appendChild(banner);
  } else {
    this.loaded = true;
  }

  this.ping = function(cmdIdx, otherParams) {
    if (!this.loaded) {
      return new Promise(function(_, reject) {
        reject();
      });
    }
    let context = this;
    return new Promise(function(resolve, reject) {
      let uid = RandomUid();
      context.nativeCallMap.set(
        uid,
        { c: cmdIdx, resolve: resolve, reject: reject });
      window.postMessage(
        { webcard: 'request', i: uid, c: cmdIdx, ...otherParams },
        SafeTargetOrigin());
    });
  }

  this.readers = function() {
    return this.ping(1);
  }

  this.response = function(msg) {
    if (!msg) {
      return;
    }
    if (msg.e) {
      switch (msg.e) {
        case 1: // Card inserted
          if (navigator.webcard.cardInserted) {
            navigator.webcard.cardInserted(msg.r, msg.d);
          }
          break;
        case 2: // Card removed
          if (navigator.webcard.cardRemoved) {
            navigator.webcard.cardRemoved(msg.r);
          }
          break;
        case 3: // Readers changed (more, some plugged)
          if (navigator.webcard.readersChanged) {
            navigator.webcard.readersChanged(true);
          }
          break;
        case 4: // Readers changed (less, some unplugged)
          if (navigator.webcard.readersChanged) {
            navigator.webcard.readersChanged(false);
          }
          break;
      }
      return;
    }
    if (!msg.i) {
      // Impossible to resolve
      return;
    }
    let pending = this.nativeCallMap.get(msg.i);
    if (!pending) {
      // No match for given response UID
      return;
    }
    if (msg.incomplete) {
      // Response marked as incomplete (error on the Native App side)
      pending.reject();
    } else {
      switch (pending.c) {
        case 1:  // List readers
          if (msg.d) {
            let readers_list = [];
            msg.d.forEach((element, index) => {
              readers_list.push(new Reader(index, element.n, element.a));
            });
            pending.resolve(readers_list);
          } else {
            pending.reject();
          }
          break;
        case 2: // Connect
        case 4: // Transcieve
          if (msg.d) {
            pending.resolve(msg.d);
          } else {
            pending.reject();
          }
          break;
        case 3: // Disconnect
          pending.resolve();
          break;
        case 10: // Get Version
          pending.resolve(msg);
          break;
        default: // Unknown command (possibly just a ping)
          pending.resolve();
      }
    }
    // Dealt with this promise
    this.nativeCallMap.delete(msg.i);
  }
};

/******************************************************************************/

if (typeof navigator.webcard === 'undefined') {
  navigator.webcard = new WebCard();
}

/******************************************************************************/

function windowWebcardReponse(event) {
  // Called after any `window.postMessage()` (content script, page scripts).
  // We want to only accept messages from ourselves.
  if (event.source != window) {
    return;
  }
  if (event.data.webcard && (event.data.webcard == 'response')) {
    navigator.webcard.response(event.data);
  }
}

window.addEventListener('message', windowWebcardReponse, false);

/******************************************************************************/
