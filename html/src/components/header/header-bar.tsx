import { h } from 'preact';
import { useEffect, useState } from 'preact/hooks';
import './header-bar.scss';
import { InfoModal } from './info-modal';
import { SettingsModal } from './settings-modal';

interface GpioItem {
    gpio: number;
    mode: number;
    dval: number;
    state: number;
    name: string;
    desc: string;
    color: string;
}

interface Props {
    gpioStates: boolean[];
    restUrl: string;
    rx: boolean;
    tx: boolean;
    wsConnected: boolean;
}

export function HeaderBar({ rx, tx, wsConnected, gpioStates, restUrl }: Props) {
    const [gpioItems, setGpioItems] = useState<GpioItem[]>([]);

    const [showInfo, setShowInfo] = useState(false);
    const [showSettings, setShowSettings] = useState(false);

    useEffect(() => {
        fetch(restUrl + '/gpio')
            .then(res => res.json())
            .then(data => setGpioItems(data))
            .catch(err => console.error('Failed to fetch GPIO data', err));
    }, []);

    useEffect(() => {
        if (!gpioItems.length || !gpioStates.length) return;
        setGpioItems(prev =>
            prev.map((item, index) => {
                if (item.mode === 0 && gpioStates[index] !== undefined) {
                    return { ...item, state: gpioStates[index] ? 1 : 0 };
                }
                return item;
            })
        );
    }, [gpioStates]);

    const handleButtonClick = async (item: GpioItem) => {
        if (!item.name || item.dval == null) {
            console.warn('Invalid name or dval of GPIO:', item);
            return;
        }

        const payload = { [item.name]: item.dval };
        console.log('Sending payload:', payload);

        try {
            const response = await fetch(restUrl + '/gpio', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(payload),
                // body: JSON.stringify({ "ps_on": 3500 }),
            });

            const text = await response.text();
            console.log(`Response: ${response.status} ${text}`);

            if (!response.ok) {
                throw new Error(`Request failed: ${response.status}`);
            }
        } catch (error) {
            console.error(`Error during request ${item.name}:`, error);
        }
    };

    const controlButtons = gpioItems
        .filter(item => item.mode === 1)
        .map((item, index) => (
            <button
                key={`btn-${index}-${item.name}`}
                onClick={() => handleButtonClick(item)}
                style={{
                    background: item.color ? item.color : '#333',
                }}
            >
                {item.desc || item.name}
            </button>
        ));

    const ledIndicators = gpioItems
        .filter(item => item.mode === 0)
        .map((item, index) => (
            <div
                key={`led-${index}-${item.name}`}
                className="status-led"
                title={item.desc || item.name}
                style={{
                    backgroundColor: item.state ? item.color : '#555',
                }}
            />
        ));

    return (
        <div className="header-bar">
            {controlButtons}
            <div style={{ marginLeft: 'auto', display: 'flex', gap: '12px', alignItems: 'center' }}>
                {ledIndicators}
                <div className="separator" />
                <div className="status-led" title="TX" style={{ backgroundColor: tx ? 'gold' : '#555' }} />
                <div className="status-led" title="RX" style={{ backgroundColor: rx ? '#34EBDE' : '#555' }} />
                <div
                    className="status-led"
                    title="WebSocket"
                    style={{ backgroundColor: wsConnected ? 'limegreen' : '#555' }}
                />
                <button onClick={() => setShowInfo(true)} className="icon-button" title="Information">
                    <svg
                        width="16"
                        height="16"
                        viewBox="0 0 24 24"
                        stroke="currentColor"
                        stroke-width="1.7"
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        fill="none"
                    >
                        <circle cx="12" cy="12" r="10" />
                        <line x1="12" y1="16" x2="12" y2="12" />
                        <circle cx="12" cy="8" r="1.5" />
                    </svg>
                </button>
                <button onClick={() => setShowSettings(true)} className="icon-button" title="Settings">
                    <svg
                        width="16"
                        height="16"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="1.7"
                        stroke-linecap="round"
                        stroke-linejoin="round"
                        viewBox="0 0 24 24"
                    >
                        <circle cx="12" cy="12" r="3" />
                        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09c.83 0 1.58-.5 1.84-1.28a1.65 1.65 0 0 0-.33-1.82L4.54 6.4a2 2 0 1 1 2.83-2.83l.06.06c.46.46 1.15.6 1.75.33a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09c0 .66.26 1.3.73 1.77a1.65 1.65 0 0 0 1.82.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82c.24.78 1 1.28 1.84 1.28H21a2 2 0 1 1 0 4h-.09c-.66 0-1.3.26-1.77.73z" />
                    </svg>
                </button>
            </div>
            {showInfo && <InfoModal onClose={() => setShowInfo(false)} restUrl={restUrl} />}
            {showSettings && <SettingsModal onClose={() => setShowSettings(false)} restUrl={restUrl} />}
        </div>
    );
}
