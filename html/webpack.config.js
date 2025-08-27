import path from 'path';
import { merge } from 'webpack-merge';
import CopyWebpackPlugin from 'copy-webpack-plugin';
import HtmlWebpackPlugin from 'html-webpack-plugin';
import MiniCssExtractPlugin from 'mini-css-extract-plugin';
import CssMinimizerPlugin from 'css-minimizer-webpack-plugin';
import TerserPlugin from 'terser-webpack-plugin';
import webpack from 'webpack';
import ESLintPlugin from 'eslint-webpack-plugin';

const devMode = process.env.NODE_ENV !== 'production';
const __dirname = path.resolve();

const baseConfig = {
    context: path.resolve(__dirname, 'src'),
    entry: {
        app: './index.tsx'
    },
    output: {
        path: path.resolve(__dirname, 'dist'),
        filename: devMode ? '[name].js' : '[name].[fullhash].js',
        clean: true
    },
    module: {
        rules: [
            {
                test: /\.m?js$/,
                include: /node_modules\/@xterm/,
                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: ['@babel/preset-env'],
                    },
                },
            },
            /*{
                test: /\.ts$/,
                enforce: 'pre',
                use: 'tslint-loader',
            },*/
            {
                test: /\.tsx?$/,
                use: 'ts-loader',
                exclude: /node_modules/
            },
            // Handling CSS without Sass.
            {
                test: /\.css$/,
                use: [
                    devMode ? 'style-loader' : MiniCssExtractPlugin.loader,
                    {
                      loader: 'css-loader',
                      options: {
                        sourceMap: devMode,
                      },
                    }
                ],
            },
            // Handling SCSS/SASS with API Dart Sass.
            {
                test: /\.s[ac]ss$/,
                use: [
                    devMode ? 'style-loader' : MiniCssExtractPlugin.loader,
                    {
                      loader: 'css-loader',
                      options: {
                        sourceMap: devMode,
                      },
                    },
                    {
                        loader: 'sass-loader',
                        options: {
                            implementation: 'sass',
                            sassOptions: {
                              quietDeps: true,
                            },
                            sourceMap: devMode,
                        }
                    }
                ],
            },
        ]
    },
    resolve: {
        extensions: [ '.tsx', '.ts', '.js' ],
        fallback: {
            util: 'util/',
            process: 'process/browser',
        }
    },
    plugins: [
        new ESLintPlugin({
            extensions: ["ts", "tsx", "js", "jsx"],
            emitWarning: true,
            failOnError: false
        }),
        new webpack.ProvidePlugin({
            process: 'process/browser',
        }),
        new CopyWebpackPlugin({
            patterns: [
                { from: './favicon.png', to: '.' }
            ],
        }),
        new MiniCssExtractPlugin({
            filename: devMode ? '[name].css' : '[name].[fullhash].css',
            chunkFilename: devMode ? '[id].css' : '[id].[fullhash].css',
        }),
        new HtmlWebpackPlugin({
            inject: false,
            minify: {
                removeComments: true,
                collapseWhitespace: true,
            },
            title: 'Wi-Se - Terminal',
            template: './template.html'
        })
    ],
    performance: {
        hints: false
    },
};

const devConfig = {
    mode: 'development',
    devServer: {
        static: {
            directory: path.join(__dirname, 'dist'),
        },
        compress: true,
        port: 9000,
        proxy: [{
            context: [ '/token', '/stty', '/gpio', '/stats', '/heap', '/reset', '/whoami', '/ws' ],
            target: 'http://localhost:7681',
            ws: true
        }]
    },
    devtool: 'inline-source-map',
};

const prodConfig = {
    mode: 'production',
    optimization: {
        minimize: true,
        minimizer: [
            new TerserPlugin({
                terserOptions: {
                    compress: {
                        drop_console: true,
                    },
                },
            }),
            new CssMinimizerPlugin(),
        ],
    },
    devtool: 'source-map',
};

export default merge(baseConfig, devMode ? devConfig : prodConfig);
