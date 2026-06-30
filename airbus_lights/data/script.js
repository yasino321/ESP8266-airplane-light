function updateMaster() {
    let data = {
        masterOn: document.getElementById('masterOn').checked,
        masterBrightness: parseInt(document.getElementById('masterBrightness').value)
    };
    sendUpdate('/api/master', data);
}

function updateChannel(name) {
    let data = {
        state: document.getElementById(name + 'State').checked,
        brightness: parseInt(document.getElementById(name + 'Brightness').value)
    };
    sendUpdate('/api/channel/' + name, data);
}

function updateBeaconMode() {
    let isPulse = document.getElementById('beaconPulseMode').checked;
    document.getElementById('beaconModeText').innerText = isPulse ? 'Pulse' : 'Blink';
    sendUpdate('/api/beaconMode', { pulse: isPulse });
}

function sendUpdate(url, data) {
    fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    });
}

function fetchState() {
    fetch('/api/state')
        .then(response => response.json())
        .then(state => {
            document.getElementById('masterOn').checked = state.masterOn;
            document.getElementById('masterBrightness').value = state.masterBrightness;

            document.getElementById('navState').checked = state.nav.state;
            document.getElementById('navBrightness').value = state.nav.brightness;

            document.getElementById('strobeWingState').checked = state.strobeWing.state;
            document.getElementById('strobeWingBrightness').value = state.strobeWing.brightness;

            document.getElementById('strobeTailState').checked = state.strobeTail.state;
            document.getElementById('strobeTailBrightness').value = state.strobeTail.brightness;

            document.getElementById('beaconState').checked = state.beacon.state;
            document.getElementById('beaconBrightness').value = state.beacon.brightness;

            document.getElementById('beaconPulseMode').checked = state.beaconPulseMode;
            document.getElementById('beaconModeText').innerText = state.beaconPulseMode ? 'Pulse' : 'Blink';
        });
}

// Initial fetch
fetchState();
// Poll every 2 seconds to sync with encoder changes
setInterval(fetchState, 2000);
