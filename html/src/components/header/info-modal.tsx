import { h } from 'preact';
import { useEffect, useState } from 'preact/hooks';
import './info-modal.scss';

interface Props {
    restUrl: string;
    onClose: () => void;
}

export function InfoModal({ restUrl, onClose }: Props) {
    const [info, setInfo] = useState<any>(null);

    useEffect(() => {
        fetch(restUrl + '/whoami')
            .then(res => res.json())
            .then(setInfo)
            .catch(err => console.error('Failed to fetch info:', err));
    }, []);

    const handleReset = async () => {
        try {
            await fetch(restUrl + '/reset');
            onClose();
        } catch (error) {
            console.error('Reset failed:', error);
        }
    };

    const renderRow = (label: string, value: string | number | undefined) => (
        <tr>
            <td>{label}</td>
            <td>{value != null ? String(value) : '-'}</td>
        </tr>
    );

    return (
        <div className="modal-overlay">
            <div className="modal-content">
                <h2>Device information</h2>
                {!info ? (
                    <p>Loading...</p>
                ) : (
                    <table className="info-table">
                        <tbody>
                            {renderRow('Name', info.pretty_name)}
                            {renderRow('Hostname', info.hostname)}
                            {renderRow('Board', info.board)}
                            {renderRow('SoC', info.soc?.type)}
                            {renderRow('Chip ID', info.soc?.chipId)}
                            {renderRow('SDK/Core', info.soc?.sdk)}
                            {renderRow('Firmware', `${info.software?.implementation} ${info.software?.version}`)}
                            {renderRow('Voltage (V)', info.health?.vccVoltage)}
                            {renderRow('Free Heap (B)', info.health?.heapFree)}
                            {renderRow('WiFi SSID', info.net?.ssid)}
                            {renderRow('IP', info.net?.ip)}
                            {renderRow('MAC', info.net?.macAddr)}
                            {renderRow('RSSI (dBm)', info.net?.rssi)}
                        </tbody>
                    </table>
                )}
                <div className={`modal-buttons ${!info ? 'single' : ''}`}>
                    {info && <button onClick={handleReset}>Reset board</button>}
                    <button onClick={onClose}>Cancel</button>
                </div>
            </div>
        </div>
    );
}
