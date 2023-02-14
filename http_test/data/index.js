// "index.js"
'use strict';

/******************************************************************************/
// Global variables.
let _readers;
let _calledListReaders = false;

/******************************************************************************/
// Detecting if the extension is installed.

(() =>
{
    let messages = [];

    // Safe cross-context messaging requires a non-local webpage.
    if ((typeof window.location.origin !== 'string') ||
    window.location.href.startsWith('file://'))
    {
        let info = `<p>
            Safe cross-context messaging requires a non-local webpage!
            </p><p>
            Please launch this &quot;Testing Webpage&quot;
            from a <b>HTTP server.</b>
        </p>`;
        messages.push(info);
    }

    if (typeof navigator.webcard === 'undefined')
    {
        let info = `<p>
            Please include the <b>&quot;webcard.js&quot;</b> script!
        </p>`;
        messages.push(info);
    }
    else
    {
        if (!navigator.webcard.isReady)
        {
            let url = navigator.webcard.installerUrl;
            let info = `<p>
                This page needs you to install the
                <a href="${url}">Smart Card Browser Extension</a>
                to access your Smart Card Readers.
            </p>`;
            messages.push(info);
        }
    }

    if (messages.length > 0)
    {
        let darkenMessages = document.createElement('div');

        messages.forEach((text) =>
        {
            let msgDiv = document.createElement('div');
            msgDiv.innerHTML = text;
            darkenMessages.appendChild(msgDiv);
        });

        let darken = document.createElement('div');
        darken.id = 'darken';
        darken.appendChild(darkenMessages);
        document.body.appendChild(darken);
    }

})();

/******************************************************************************/
function ClearOutput()
{
    document.getElementById('apduListOut').value = '';
    document.getElementById('info-box').innerHTML = '<br /><hr />';
}

/******************************************************************************/
async function OnIndexLoad()
{
    if (!navigator.webcard)
    {
        return;
    }

    let ver = await navigator.webcard.getVersions();

    let infoBox = document.getElementById('info-box');
    infoBox.innerHTML = `<i>
        Version reported by the Browser Extension: <b>${ver.addon}</b><br />
        Version reported by the Native Application: <b>${ver.app}</b><br />
        Latest version according to <u>webcard.js</u>: <b>${ver.latest}</b>
    </i><hr />`;

    ListReaders(true);
}

/******************************************************************************/
async function ListReaders(refresh)
{
    if (!_calledListReaders)
    {
        _calledListReaders = true;
    }
    else
    {
        alert('ListReaders() called asynchronously, ignoring...');
        return;
    }

    let readersTable = document.getElementById('readers-table');

    while (readersTable.firstChild)
    {
        readersTable.removeChild(readersTable.firstChild);
    }

    if (true === refresh)
    {
        _readers = await navigator.webcard.readers()
            .catch(() => undefined);
    }

    if (_readers)
    {
        if (_readers.length > 0)
        {
            let headerRow = document.createElement('tr');
            headerRow.innerHTML += '<td colspan="3"><b>Actions</b></td>';
            headerRow.innerHTML += '<td><b>Reader name</b></td>';
            headerRow.innerHTML += '<td><b>Reader status</b></td>';
            readersTable.append(headerRow);

            _readers.forEach((reader, index) =>
            {
                let tabRow = document.createElement('tr');

                tabRow.innerHTML += `<td>
                    <img
                        class="smartcard-image clickable-image"
                        src="resources/smartcard_conn.png"
                        onclick="TestReader(${index}, 1, 0, 0)"
                    />
                </td>`;

                tabRow.innerHTML += reader.atr ?
                    `<td>
                        <img
                            class="smartcard-image clickable-image"
                            src="resources/smartcard_on.png"
                            onclick="TestReader(${index}, 0, 1, 0)"
                        />
                    </td>` :
                    `<td>
                        <img
                            class="smartcard-image"
                            src="resources/smartcard_off.png"
                        />
                    </td>`;

                tabRow.innerHTML += `<td>
                    <img
                        class="smartcard-image clickable-image"
                        src="resources/smartcard_disconn.png"
                        onclick="TestReader(${index}, 0, 0, 1)"
                    />
                </td>`;

                tabRow.innerHTML += `<td>${reader.name}</td>`;

                let statusText = '• ';
                if (true === reader.connected)
                {
                    statusText += '<b>Connected!</b>';
                }
                else if (false === reader.connected)
                {
                    statusText += 'Disconnected';
                }
                else
                {
                    statusText += 'Undefined';
                }

                statusText += '<br />• ';
                statusText += reader.atr ? '<b>Card present!</b>' : 'Empty';

                tabRow.innerHTML += `<td>${statusText}</td>`;

                readersTable.append(tabRow);
            });
        }
        else
        {
            let headerRow = document.createElement('tr');
            headerRow.innerHTML = '<td><i>No Smart Card readers available...</i></td>';
            readersTable.append(headerRow);
        }
    }
    else
    {
        let headerRow = document.createElement('tr');
        headerRow.innerHTML = '<td><i>WebCard extension is inactive!</i></td>';
        readersTable.append(headerRow);
    }

    _calledListReaders = false;
}

/******************************************************************************/
async function TestReader(index, connect, transcieve, disconnect)
{
    const startTime = performance.now();
    const startDate = new Date();
    const apduListTextareaIn = document.getElementById('apduListIn');
    let apduListTextareaOut = document.getElementById('apduListOut');
    let output = startDate.toISOString().replace('T', ' ') + '\n\n';
    let failed = false;
    let reader = _readers[index];

    if (!failed && connect)
    {
        await reader.connect(true)
            .then((atr) =>
            {
                console.log(`Connection established (${reader.name}, ${atr})`);
            })
            .catch(() =>
            {
                console.log(`Could not connect to the reader! (${reader.name})`);
                failed = true;
            });

        output += (`> connect (${reader.name})\n`);

        if (failed)
        {
            output += '< ⚠️\n\n';
        }
        else
        {
            output += '< 🆗\n\n';
            reader.connected = true;
            await ListReaders(false);
        }
    }

    if (!failed && transcieve)
    {
        let apduList = apduListTextareaIn.value.match(/^[0-9A-Fa-f]+$/gm);
        if (apduList)
        {
            for (let i = 0; i < apduList.length; i++)
            {
                output += (`> ${apduList[i]}\n`);

                await reader.transcieve(apduList[i])
                    .then((response) =>
                    {
                        output += (`< ${response}\n\n`);
                    })
                    .catch(() =>
                    {
                        console.log(`No response from IC Card! (${reader.name})`);
                        output += '< ⚠️\n\n';
                        failed = true;
                    });

                if (failed) break;
            }
        }
        else
        {
            output += '# ⚠️ No valid APDUs in the list!\n\n';
        }
    }

    if (!failed && disconnect)
    {
        await reader.disconnect()
            .then(() => {
                console.log(`Connection closed (${reader.name})`);
            })
            .catch(() => {
                console.log('Could not disconnect from the reader!');
                failed = true;
            });

        output += `> disconnect (${reader.name})\n`;

        if (failed)
        {
            output += '< 🤨\n\n';
        }
        else
        {
            output += '< 🆗\n\n';
            _readers[index].connected = false;
            await ListReaders(false);
        }
    }

    const elapsedTime = performance.now() - startTime;
    apduListTextareaOut.value = output + `⏳ ${elapsedTime.toFixed(2)} ms\n`;
}

/******************************************************************************/
async function PingPong()
{
    const startTime = performance.now();
    const startDate = new Date();
    let apduListTextareaOut = document.getElementById('apduListOut');
    let output = `${startDate.toISOString().replace('T', ' ')}\n\n`;
    let pong;

    await navigator.webcard.send(0)
        .then(() =>  {pong = 'Pong! 👍';})
        .catch(() => {pong = '⚠️';});
    output += (`> Ping...  ⏳\n> ${pong}\n\n`);

    const elapsedTime = performance.now() - startTime;
    apduListTextareaOut.value = output + `⏳ ${elapsedTime.toFixed(2)} ms\n`;
}

/******************************************************************************/
function ShowInfoAndListReaders(info, refresh)
{
    let infoBox = document.getElementById('info-box');
    console.log(info);
    infoBox.innerHTML = `<i>${info}</i><hr />`;
    ListReaders(refresh);
}

/******************************************************************************/
// Setting up the WebCard Event Callbacks.
if (typeof navigator.webcard !== 'undefined')
{
    navigator.webcard.cardInserted = (readerId, atr) =>
    {
        _readers[readerId].atr = atr;

        ShowInfoAndListReaders(
            `Card inserted: ${_readers[readerId].name}`,
            false);
    }

    navigator.webcard.cardRemoved = (readerId) =>
    {
        _readers[readerId].atr       = undefined;
        _readers[readerId].connected = false;

        ShowInfoAndListReaders(
            `Card removed: ${_readers[readerId].name}`,
            false);
    }

    navigator.webcard.readersConnected = (listOfNames) =>
    {
        let names = ((typeof listOfNames === 'object') &&
            (typeof listOfNames.join === 'function')) ?
                listOfNames.join('; ') : '---';
        let info = `Readers changed (more: ${names})`;
        ShowInfoAndListReaders(info, true);
    }

    navigator.webcard.readersDisconnected = (listOfNames) =>
    {
        let names = ((typeof listOfNames === 'object') &&
            (typeof listOfNames.join === 'function')) ?
                listOfNames.join('; ') : '---';
        let info = `Readers changed (less: ${names})`;
        ShowInfoAndListReaders(info, true);
    }
}

/******************************************************************************/
