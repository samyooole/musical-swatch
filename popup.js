// popup.js
document.addEventListener('DOMContentLoaded', function() {
    const playPauseButton = document.getElementById('playPauseButton');
    playPauseButton.addEventListener('click', togglePlayPause);
});

function togglePlayPause() {
    chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
        chrome.tabs.sendMessage(tabs[0].id, { action: "togglePlayPause" }, function(response) {
            if (response) {
                document.getElementById('playPauseButton').textContent = response === 'playing' ? 'Pause' : 'Play';
            }
        });
    });
}