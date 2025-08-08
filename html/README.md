The HTML has been adapted from ttyd:

https://github.com/tsl0922/ttyd/

## Prerequisites

Install [Node.js](https://nodejs.org/en) v15.14.0, eg: `nvm install v15.14.0`.

Install [Yarn](https://yarnpkg.com), and run: `yarn install`.

## Development

1. Start ttyd: `ttyd bash`
2. Start the dev server: `yarn run start`

## Publish

Run `yarn run build`, this will compile the inlined html to `../include/html.h`.
