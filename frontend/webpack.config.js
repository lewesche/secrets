const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const webpack = require('webpack');
const WasmPackPlugin = require("@wasm-tool/wasm-pack-plugin");

module.exports = {
    entry: './src/index.js',
    output: {
	publicPath: 'secrets.html',
    	path: path.resolve(__dirname, 'dist'),
    	filename: 'index.js',
		libraryTarget: 'var',
    	library: 'Lib'
    },
    plugins: [
        new HtmlWebpackPlugin({
		template: 'src/template.html'
	}),
        new WasmPackPlugin({
            crateDirectory: path.resolve(__dirname, ".")
        }),
        // Have this example work in Edge which doesn't ship `TextEncoder` or
        // `TextDecoder` at this time.
        new webpack.ProvidePlugin({
          TextDecoder: ['text-encoding', 'TextDecoder'],
          TextEncoder: ['text-encoding', 'TextEncoder']
        })
    ],
    mode: 'development'
};