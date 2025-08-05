import { h } from 'preact';
import { useEffect, useState } from 'preact/hooks';
import './settings-modal.scss';

interface SttyConfig {
    baudrate: number;
    bits: number;
    parity: number | null;
    stop: number;
}

interface Props {
    restUrl: string;
    onClose: () => void;
}

export function SettingsModal({ restUrl, onClose }: Props) {
    const [config, setConfig] = useState<SttyConfig>({
        baudrate: 115200,
        bits: 8,
        parity: null,
        stop: 1,
    });

    useEffect(() => {
        fetch(restUrl+'/stty')
            .then(res => res.json())
            .then(data => setConfig(data))
            .catch(err => console.error('Failed to load UART config:', err));
    }, []);

    const handleSubmit = () => {
        fetch(restUrl+'/stty', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config),
        })
            .then(res => {
                if (!res.ok) throw new Error('Failed to save config');
                onClose();
            })
            .catch(err => console.error('Failed to save UART config:', err));
    };

    const handleChange = (field: keyof SttyConfig, value: string) => {
        setConfig(prev => ({
            ...prev,
            [field]: field === 'parity' ? (value || null) : parseInt(value),
        }));
    };

    return (
        <div className="modal-overlay">
            <div className="modal-content">
                <h2>UART Settings</h2>
                <div className="form-grid">
                    <label>Baudrate:</label>
                    <select value={config.baudrate} onChange={e => handleChange('baudrate', e.currentTarget.value)}>
                        {[300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 576000, 921600, 1000000, 2000000].map(rate => (
                            <option key={rate} value={rate}>{rate}</option>
                        ))}
                    </select>

                    <label>Data bits:</label>
                    <select value={config.bits} onChange={e => handleChange('bits', e.currentTarget.value)}>
                        {[5, 6, 7, 8].map(bit => (
                            <option key={bit} value={bit}>{bit}</option>
                        ))}
                    </select>

                    <label>Parity:</label>
                    <select value={config.parity ?? ''} onChange={e => { const val = e.currentTarget.value; setConfig(prev => ({ ...prev, parity: val === '' ? null : parseInt(val, 10), })); }}>
                        <option value="">Brak</option>
                        <option value="0">Even</option>
                        <option value="1">Odd</option>
                    </select>

                    <label>Stop bits:</label>
                    <select value={config.stop} onChange={e => handleChange('stop', e.currentTarget.value)}>
                        {[0, 1, 2].map(stop => (
                            <option key={stop} value={stop}>{stop}</option>
                        ))}
                    </select>
                </div>

                <div className="modal-buttons">
                    <button onClick={handleSubmit}>OK</button>
                    <button onClick={onClose}>Cancel</button>
                </div>
            </div>
        </div>
    );
}
