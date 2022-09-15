// "index.js"
'use strict';

/******************************************************************************/

let _readers;
let _calledListReaders = false;

/******************************************************************************/

function ClearOutput() {
  document.getElementById('apduListOut').value = '';
  document.getElementById('info-box').innerHTML = '<br /><hr />';
}

/******************************************************************************/

async function OnIndexLoad() {
  const timeoutPromise = new Promise((_, reject) =>
    setTimeout(() => reject(), 5000)
  );

  const race = Promise.race([timeoutPromise, navigator.webcard.ping(10)]);

  const versions = await race
    .then(msg => [msg.verExt, msg.verNat])
    .catch(() => [undefined, undefined]);

  let infoBox = document.getElementById('info-box');
  let output = '<i>';
  output += ('Version reported by the Browser Extension: <b>' + versions[0] + '</b><br/>');
  output += ('Version reported by the Native Application: <b>' + versions[1] + '</b>');
  output += '</i><hr />';
  infoBox.innerHTML = output;

  ListReaders(true);
}

/******************************************************************************/

async function ListReaders(refresh) {
  if (!_calledListReaders) {
    _calledListReaders = true;
  } else {
    alert('`ListReaders()` called asynchronously, ignoring...');
    return;
  }

  let readers_table = document.getElementById('readers-table');

  while (readers_table.firstChild) {
    readers_table.removeChild(readers_table.firstChild);
  }

  if (true === refresh) {
    _readers = await navigator.webcard.readers()
      .catch(() => undefined);
  }

  if (_readers) {
    let header_row = document.createElement('tr');
    header_row.innerHTML += '<td colspan="4"><b>Actions</b></td>';
    header_row.innerHTML += '<td><b>Reader name</b></td>';
    header_row.innerHTML += '<td><b>Reader status</b></td>';
    readers_table.append(header_row);

    _readers.forEach((reader, index) => {
      // @ console.log(reader);
      let tab_row = document.createElement('tr');
      tab_row.innerHTML += ('<td><img class="smartcard-image clickable-image" src="resources/smartcard_conn.png" onclick="TestReader(' + index + ', 1, 0, 0)" /></td>');
      tab_row.innerHTML += reader.atr ?
        ('<td><img class="smartcard-image clickable-image" src="resources/smartcard_on.png" onclick="TestReader(' + index + ', 0, 1, 0)" /></td>') :
        ('<td><img class="smartcard-image" src="resources/smartcard_off.png" /></td>');
        tab_row.innerHTML += ('<td><img class="smartcard-image clickable-image" src="resources/smartcard_disconn.png" onclick="TestReader(' + index + ', 0, 0, 1)" /></td>');
        tab_row.innerHTML += ('<td><img class="smartcard-image clickable-image" src="resources/smartcard_attrib.png" onclick="GetReaderAttribs(' + index + ')" /></td>');
      tab_row.innerHTML += ('<td>' + reader.name + '</td>');
      let status_text = '• ';
      if (true === reader.connected) {
        status_text += '<b>Connected!</b>';
      } else if (false === reader.connected) {
        status_text += 'Disconnected';
      } else {
        status_text += 'Undefined';
      }
      status_text += '<br>• ';
      status_text += reader.atr ? '<b>Card present!</b>' : 'Empty';
      tab_row.innerHTML += ('<td>' + status_text + '</td>');
      readers_table.append(tab_row);
    });
  } else {
    let header_row = document.createElement('tr');
    header_row.innerHTML += '<td><i>WebCard extension is inactive!</i></td>';
    readers_table.append(header_row);
  }

  _calledListReaders = false;
}

/******************************************************************************/

async function TestReader(index, connect, transcieve, disconnect) {
  const startTime = performance.now();
  const startDate = new Date();
  const apduListTextareaIn = document.getElementById('apduListIn');
  let apduListTextareaOut = document.getElementById('apduListOut');
  let output = startDate.toISOString().replace('T', ' ') + '\n\n';
  let failed = false;
  let reader = _readers[index];

  if (!failed && connect) {
    await reader.connect(true)
      .then(atr => {
        console.log('Connection established (' + reader.name + ', ' + atr + ')');
      })
      .catch(() => {
        console.log('Could not connect to the reader! (' + reader.name + ')');
        failed = true;
      });
    output += ('> connect (' + reader.name + ')\n');
    if (failed) {
      output += '< ⚠️\n\n';
    } else {
      output += '< 🆗\n\n';
      reader.connected = true;
      await ListReaders(false);
    }
  }

  if (!failed && transcieve) {
    let apduList = apduListTextareaIn.value.match(/^[0-9A-Fa-f]+$/gm);
    if (apduList) {
      for (let i = 0; i < apduList.length; i++) {
        output += ('> ' + apduList[i] + '\n');
        await reader.transcieve(apduList[i])
          .then(response => {
            output += ('< ' + response + '\n\n');
          })
          .catch(() => {
            console.log('No response from IC Card! (' + reader.name + ')');
            output += '< ⚠️\n\n';
            failed = true;
          });
        if (failed) break;
      }
    } else {
      output += '# ⚠️ No valid APDUs in the list!\n\n';
    }
  }

  if (!failed && disconnect) {
    await reader.disconnect()
      .then(() => {
        console.log('Connection closed (' +reader.name + ')');
      })
      .catch(() => {
        console.log('Could not disconnect from the reader!');
        failed = true;
      });
    output += '> disconnect (' + reader.name + ')\n';
    if (failed) {
      output += '< 🤨\n\n';
    } else {
      output += '< 🆗\n\n';
      _readers[index].connected = false;
      await ListReaders(false);
    }
    _readers[index].connected = false;
    await ListReaders(false);
  }

  const elapsedTime = performance.now() - startTime;
  apduListTextareaOut.value = output + `⏳ ${elapsedTime.toFixed(2)} ms\n`;
}

/******************************************************************************/

async function GetReaderAttribs(index) {}

/******************************************************************************/

async function PingPong() {
  const startTime = performance.now();
  const startDate = new Date();
  let apduListTextareaOut = document.getElementById('apduListOut');
  let output = startDate.toISOString().replace('T', ' ') + '\n\n';
  let pong;

  await navigator.webcard.ping(0)
    .then(() =>  {pong = 'Pong! 👍';})
    .catch(() => {pong = '⚠️';});
  output += ('> Ping...  ⏳\n> ' + pong + '\n\n');

  const elapsedTime = performance.now() - startTime;
  apduListTextareaOut.value = output + `⏳ ${elapsedTime.toFixed(2)} ms\n`;
}

/******************************************************************************/

function ShowInfoAndListReaders(info, refresh) {
  let infoBox = document.getElementById('info-box');
  console.log(info);
  infoBox.innerHTML = '<i>' + info + '</i><hr />';
  ListReaders(refresh);
}

/******************************************************************************/

if (typeof navigator.webcard !== 'undefined') {
  navigator.webcard.cardInserted = function(reader_idx, atr) {
    _readers[reader_idx].atr = atr;
    ShowInfoAndListReaders(
      ('Card inserted: ' + _readers[reader_idx].name),
      false);
  }
  navigator.webcard.cardRemoved = function(reader_idx) {
    _readers[reader_idx].atr = undefined;
    _readers[reader_idx].connected = false;
    ShowInfoAndListReaders(
      ('Card removed: ' + _readers[reader_idx].name),
      false);
  }
  navigator.webcard.readersChanged = function(moreOrLess) {
    let info = 'Readers changed (' + (moreOrLess ? 'more' : 'less') + '!)';
    ShowInfoAndListReaders(info, true);
  }
}

/******************************************************************************/
