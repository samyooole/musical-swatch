#ifndef CHORDPROFILES_H
#define CHORDPROFILES_H

// Declare chordProfiles as a pointer to a 2D array
extern float (*chordProfiles)[12];

// Declare a flag to check if profiles are initialized
extern bool chordProfilesInitialized;

// Declare the function to create chord profiles
void createChordProfiles();

// Declare the function to free chord profiles
void cleanupChordProfiles();

int classifyChromagram(float* chromagram, float (*chordProfiles)[12]);

#endif // CHORDPROFILES_H