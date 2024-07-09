// content.js
'use strict';

let wasmModule;

// Load the Emscripten generated JavaScript file
const script = document.createElement('script');
script.src = 'chroma.js'; // Adjust the path to your generated Wasm JavaScript
script.onload = () => {
    // This function is typically defined by Emscripten
    Module().then(module => {
        wasmModule = module;
        console.log('WASM module loaded:', wasmModule);
        captureTabAudio(); // Only start capturing after WASM is loaded
    });
};
document.body.appendChild(script);

function captureTabAudio() {
    try {
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();

        chrome.tabCapture.capture({ audio: true, video: false }, tabStream => {
            if (!tabStream) {
                console.error('Error capturing tab audio: Tab stream is null or undefined.');
                return;
            }

            const sourceNode = audioContext.createMediaStreamSource(tabStream);
            const scriptNode = audioContext.createScriptProcessor(4096, 1, 1);

            sourceNode.connect(scriptNode);
            scriptNode.connect(audioContext.destination);

            scriptNode.onaudioprocess = event => {
                console.log('Processing audio...');
                const inputBuffer = event.inputBuffer;
                const inputData = inputBuffer.getChannelData(0); // Mono audio data

                // Ensure input data is valid
                if (!inputData) {
                    console.error('No input data received.');
                    return;
                }

                // Allocate memory in WebAssembly for the input data
                const inputArrayPtr = wasmModule._malloc(inputData.length * inputData.BYTES_PER_ELEMENT);
                wasmModule.HEAPF32.set(inputData, inputArrayPtr / inputData.BYTES_PER_ELEMENT);

                // Call the FFT function in WebAssembly
                const resultPtr = wasmModule._chroma(inputArrayPtr, inputData.length, audioContext.sampleRate);

                // Create a Float32Array view to access the result from WebAssembly memory
                const resultArray = new Float32Array(wasmModule.HEAPF32.buffer, resultPtr, 12);

                console.log(resultArray); // should return a chroma vector

                // Free the allocated memory in WebAssembly
                wasmModule._free(inputArrayPtr);
                // Don't free resultPtr here, it was allocated by new[] in C++ and returned to JS
                // It's already managed by Emscripten, so you should not free it manually
            };
        });

    } catch (error) {
        console.error('Error capturing tab audio:', error);
    }
}