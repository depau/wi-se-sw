import { h, Component } from 'preact';

import { ITerminalOptions, ITheme } from '@xterm/xterm';
import { ClientOptions, Xterm } from './terminal';
import { HeaderBar } from './header/header-bar';

declare const module: {
    hot?: {
        accept: () => void;
    };
};

if (module.hot) {
    import('preact/debug');
}

const host = window.location.host;
const path = window.location.pathname.replace(/[/]+$/, '');
const restUrl = [window.location.protocol, '//', host, path].join('');
const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
const wsUrl = [wsProtocol, '//', host, path, '/ws', window.location.search].join('');

const clientOptions = {
    rendererType: 'webgl',
    disableLeaveAlert: false,
    disableResizeOverlay: false,
    titleFixed: null,
} as ClientOptions;
const termOptions = {
    fontSize: 13,
    fontFamily: 'Menlo For Powerline,Consolas,Liberation Mono,Menlo,Courier,monospace',
    theme: {
        foreground: '#d2d2d2',
        background: '#2b2b2b',
        cursor: '#adadad',
        black: '#000000',
        red: '#d81e00',
        green: '#5ea702',
        yellow: '#cfae00',
        blue: '#427ab3',
        magenta: '#89658e',
        cyan: '#00a7aa',
        white: '#dbded8',
        brightBlack: '#686a66',
        brightRed: '#f54235',
        brightGreen: '#99e343',
        brightYellow: '#fdeb61',
        brightBlue: '#84b0d8',
        brightMagenta: '#bc94b7',
        brightCyan: '#37e6e8',
        brightWhite: '#f1f1f0',
    } as ITheme,
} as ITerminalOptions;

export class App extends Component {
    state = {
        gpioStates: [] as boolean[],
        rx: false,
        tx: false,
        wsConnected: false,
    };

    setTx = (tx: boolean) => this.setState({ tx });
    setRx = (rx: boolean) => this.setState({ rx });
    setGpioStates = (gpioStates: boolean[]) => {
        this.setState({ gpioStates });
    };
    setWsConnected = (connected: boolean) => this.setState({ wsConnected: connected });

    render() {
        return (
            <div id="wrapper">
                <HeaderBar
                    gpioStates={this.state.gpioStates}
                    restUrl={restUrl}
                    rx={this.state.rx}
                    tx={this.state.tx}
                    wsConnected={this.state.wsConnected}
                />
                <Xterm
                    id="terminal-container"
                    wsUrl={wsUrl}
                    restUrl={restUrl}
                    clientOptions={clientOptions}
                    termOptions={termOptions}
                    onTx={this.setTx}
                    onRx={this.setRx}
                    onWsConnected={this.setWsConnected}
                    onGpioStateUpdate={this.setGpioStates}
                />
            </div>
        );
    }
}
