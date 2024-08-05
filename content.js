// content.js

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
const cosineThreshold = 0.97;

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
    const maxValue = Math.max(...chromaVector);

    if (maxValue > 0) {
        return chromaVector.map(value => value / maxValue);
    }

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

            audioContext.createMediaStreamSource(tabStream).connect(audioContext.destination);

            const sourceNode = audioContext.createMediaStreamSource(tabStream);
            const scriptNode = audioContext.createScriptProcessor(4096, 1, 1);

            sourceNode.connect(scriptNode);
            scriptNode.connect(audioContext.destination);

            const chromaBuffer = [];
            const bufferSize = 5;

            let lastChromaString = "❓No music detected";
            let chordString = "";

            scriptNode.onaudioprocess = event => {
                const inputBuffer = event.inputBuffer;
                const inputData = inputBuffer.getChannelData(0);

                if (!inputData) {
                    console.error('No input data received.');
                    return;
                }

                const isEmptyArray = inputData.every(sample => sample === 0);

                if (isEmptyArray) {
                    chordString = lastChromaString;
                    console.log('isempty');
                } else {
                    const inputArrayPtr = wasmModule._malloc(inputData.length * Float32Array.BYTES_PER_ELEMENT);
                    wasmModule.HEAPF32.set(inputData, inputArrayPtr / Float32Array.BYTES_PER_ELEMENT);

                    const thischromaPtr = wasmModule._chroma(inputArrayPtr, inputData.length, audioContext.sampleRate);
                    const thischroma = new Float32Array(wasmModule.HEAPF32.buffer, thischromaPtr, 12);

                    const normalizedChroma = thischroma;
                    chromaBuffer.push(normalizedChroma);

                    if (chromaBuffer.length > bufferSize) {
                        chromaBuffer.shift();
                    }

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

                        if (cosineSimilarity(lastChroma, averagedChroma) < cosineThreshold) {
                            const averagedChromaPtr = wasmModule._malloc(averagedChroma.length * Float32Array.BYTES_PER_ELEMENT);
                            wasmModule.HEAPF32.set(averagedChroma, averagedChromaPtr / Float32Array.BYTES_PER_ELEMENT);

                            const chordResult = wasmModule._classifyChord(averagedChromaPtr);

                            chordString = classifyChord(chordResult);

                            wasmModule._free(averagedChromaPtr);
                            lastChroma = new Float32Array(averagedChroma);
                            lastChromaString = chordString;

                            let notes_tobeHighlighted = determineNotesfromChordInteger(chordResult);
                            highlightChord(notes_tobeHighlighted);
                        }
                    }

                    wasmModule._free(inputArrayPtr);
                }

                updateChord(chordString);
            };
        });
    } catch (error) {
        console.error('Error capturing tab audio:', error);
    }
}

function classifyChord(chordIndex) {
    const chordTypeIndex = Math.floor(chordIndex / 12);
    const chordKeyIndex = chordIndex % 12;

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

function highlightChord(chordNotes) {
    document.querySelectorAll('.key').forEach(key => {
        key.classList.remove('highlighted');
    });

    chordNotes.forEach(note => {
        const key = document.querySelector(`.key[data-note="${note}"]`);
        if (key) {
            key.classList.add('highlighted');
        }
    });
}

function determineNotesfromChordInteger(chordInteger) {
    const key = chordInteger % 12;
    const phase = Math.floor(chordInteger / 12);

    let constituent_notes = [];

    switch (phase) {
        case 0: constituent_notes = [0, 4, 7]; break;
        case 1: constituent_notes = [0, 3, 7]; break;
        case 2: constituent_notes = [0, 3, 6]; break;
        case 3: constituent_notes = [0, 4, 8]; break;
        case 4: constituent_notes = [0, 2, 7]; break;
        case 5: constituent_notes = [0, 5, 7]; break;
        case 6: constituent_notes = [0, 4, 7, 11]; break;
        case 7: constituent_notes = [0, 3, 7, 10]; break;
        case 8: constituent_notes = [0, 4, 7, 10]; break;
        default: console.log("Chord phase not recognized"); break;
    }

    let notes = [];
    for (let note_pos of constituent_notes) {
        notes.push(getNotefromRoot(key, note_pos));
    }

    return notes;
}

function getNotefromRoot(root, note_position) {
    const twelvekeys = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
    const rotatedKeys = twelvekeys.slice(root).concat(twelvekeys.slice(0, root));
    return rotatedKeys[note_position];
}

// Play/pause control

function controlPageMedia() {
    const media = document.querySelector('video, audio');
    if (media) {
        if (media.paused) {
            media.play();
        } else {
            media.pause();
        }
        return media.paused ? 'paused' : 'playing';
    }
    return 'no media found';
}


chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
    if (request.action === "togglePlayPause") {
        sendResponse(controlPageMedia());
    }
});



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




