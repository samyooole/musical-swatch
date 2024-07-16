//content.js

'use strict';

let wasmModule;

const script = document.createElement('script');
script.src = 'chroma.js';
script.onload = () => {
    Module().then(module => {
        wasmModule = module;
        console.log('WASM module loaded:', wasmModule);
        captureTabAudio();
    });
};
document.body.appendChild(script);

let lastChroma = new Float32Array(12).fill(-1);
const cosineThreshold = 0.95

function cosineSimilarity(vecA, vecB) {
    if (vecA.length !== vecB.length) {
        throw new Error("Vectors must be of the same length");
    }

    let dotProduct = 0;
    let magnitudeA = 0;
    let magnitudeB = 0;

    for (let i = 0; i < vecA.length; i++) {
        dotProduct += vecA[i] * vecB[i];
        magnitudeA += vecA[i] * vecA[i];
        magnitudeB += vecB[i] * vecB[i];
    }

    magnitudeA = Math.sqrt(magnitudeA);
    magnitudeB = Math.sqrt(magnitudeB);

    if (magnitudeA === 0 || magnitudeB === 0) {
        return 0; // Prevent division by zero
    }

    return dotProduct / (magnitudeA * magnitudeB);
}


function normalizeChroma(chromaVector) {
    // Find the maximum value in the chroma vector
    const maxValue = Math.max(...chromaVector);

    // Avoid division by zero
    if (maxValue > 0) {
        // Normalize each value by dividing it by the maximum value
        return chromaVector.map(value => value / maxValue);
    }

    // If maxValue is 0, return the original vector
    return chromaVector;
}

function captureTabAudio() {
    try {
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();

        chrome.tabCapture.capture({ audio: true, video: false }, tabStream => {
            if (!tabStream) {
                console.error('Error capturing tab audio: Tab stream is null or undefined.');
                return;
            }

            audioContext.createMediaStreamSource(tabStream).connect(audioContext.destination); // secondary stream for audio to pass through?

            const sourceNode = audioContext.createMediaStreamSource(tabStream);
            const scriptNode = audioContext.createScriptProcessor(4096, 1, 1);

            sourceNode.connect(scriptNode);
            scriptNode.connect(audioContext.destination);

            // Define a buffer to store recent chroma vectors
            const chromaBuffer = [];
            const bufferSize = 4; // Number of frames to average over

            // Handle audio processing
            scriptNode.onaudioprocess = event => {
                const inputBuffer = event.inputBuffer;
                const inputData = inputBuffer.getChannelData(0);
            
                if (!inputData) {
                    console.error('No input data received.');
                    return;
                }
            
                // Allocate memory for input data in the WebAssembly module
                const inputArrayPtr = wasmModule._malloc(inputData.length * Float32Array.BYTES_PER_ELEMENT);
                wasmModule.HEAPF32.set(inputData, inputArrayPtr / Float32Array.BYTES_PER_ELEMENT);
            
                // Call the C function to compute chroma
                const thischromaPtr = wasmModule._chroma(inputArrayPtr, inputData.length, audioContext.sampleRate);
            
                // Create a JavaScript Float32Array view of the returned thischroma data (assuming it's a Float32Array)
                const thischroma = new Float32Array(wasmModule.HEAPF32.buffer, thischromaPtr, 12);

                //const normalizedChroma = normalizeChroma(thischroma);
                
                const normalizedChroma = thischroma;
                
                // Free allocated memory for input data
                wasmModule._free(inputArrayPtr);
            
                // Push thischroma into chromaBuffer
                //chromaBuffer.push(thischroma);
                chromaBuffer.push(normalizedChroma);
            
                // Ensure buffer size doesn't exceed maximum
                if (chromaBuffer.length > bufferSize) {
                    chromaBuffer.shift(); // Remove oldest frame
                }


                //  average chroma vector across the buffer
                if (chromaBuffer.length > 0) {
                    const averagedChroma = new Float32Array(12).fill(0);
                    chromaBuffer.forEach(frame => {
                        for (let i = 0; i < 12; i++) {
                            averagedChroma[i] += frame[i];
                        }
                    });
                    for (let i = 0; i < 12; i++) {
                        averagedChroma[i] /= chromaBuffer.length;
                    }


                    //if the averaged chroma is significantly different than before
                    if (cosineSimilarity(lastChroma, averagedChroma) < cosineThreshold){
                        // Perform chord classification on averagedChroma instead of raw chroma
                        const averagedChromaPtr = wasmModule._malloc(averagedChroma.length * Float32Array.BYTES_PER_ELEMENT);
                        wasmModule.HEAPF32.set(averagedChroma, averagedChromaPtr / Float32Array.BYTES_PER_ELEMENT);

                        const chordResult = wasmModule._classifyChord(averagedChromaPtr);

                        const chordString = classifyChord(chordResult);
                        console.log(`Chord: ${chordString}`);

                        updateChord(chordString);

                        // Free allocated memory for averagedChroma
                        wasmModule._free(averagedChromaPtr);
                        // Update lastChroma
                        lastChroma = new Float32Array(averagedChroma);
                    }
            
                    
                }
                
            };
        });
    } catch (error) {
        console.error('Error capturing tab audio:', error);
    }
}

function classifyChord(chordIndex) {
    const chordTypeIndex = Math.floor(chordIndex / 12);
    const chordKeyIndex = chordIndex % 12;

    // Define arrays to hold the chord type and chord key strings
    const chordTypes = [
        "Major", "Minor", "Diminished 5th", "Augmented 5th",
        "Sus2", "Sus4", "Major 7th", "Minor 7th", "Dominant 7th", "Unknown"
    ];

    const chordKeys = [
        "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B", "Unknown"
    ];

    const chordTypeString = chordTypes[chordTypeIndex] || "Unknown";
    const chordKeyString = chordKeys[chordKeyIndex] || "Unknown";

    return `${chordKeyString} ${chordTypeString}`;
}

function updateChord(chordString) {
    const chordDiv = document.getElementById('chord-text');
    chordDiv.textContent = `${chordString}`;
}



/*

document.addEventListener('DOMContentLoaded', () => {
    // Define the note labels
    const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    
    // Get the chromagram container
    const chromagram = document.getElementById('chromagram');
    
    // Create a div for each note
    notes.forEach((note, index) => {
      const noteDiv = document.createElement('div');
      noteDiv.id = `note-${index}`;
      noteDiv.className = 'note';
      noteDiv.innerHTML = `<strong>${note}</strong>: <span id="value-${index}">0</span>`;
      chromagram.appendChild(noteDiv);
    });
  
    // Assuming `resultArray` is populated with the chroma values from WASM
    const resultArray = new Float32Array(wasmModule.HEAPF32.buffer, resultPtr, 12);
  
    // Update the divs with the chroma values
    resultArray.forEach((value, index) => {
      document.getElementById(`value-${index}`).textContent = value.toFixed(2);
    });
  
    // Optionally, apply some visual styles based on the chroma values
    const maxValue = Math.max(...resultArray);
    resultArray.forEach((value, index) => {
      const noteDiv = document.getElementById(`note-${index}`);
      const intensity = value / maxValue;
      noteDiv.style.backgroundColor = `rgba(0, 0, 255, ${intensity})`; // Blue color with varying intensity
    });
  });


  function updateChromagram(resultArray) {
    const maxValue = Math.max(...resultArray);
    resultArray.forEach((value, index) => {
        const noteDiv = document.getElementById(`note-${index}`);
        const intensity = value / maxValue;
        noteDiv.style.backgroundColor = `rgba(0, 0, 255, ${intensity})`; // Blue color with varying intensity
        document.getElementById(`value-${index}`).textContent = value.toFixed(2);
    });
}
*/




