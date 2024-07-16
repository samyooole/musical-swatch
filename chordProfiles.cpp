// chordProfiles.cpp
#include "chordProfiles.h"
#include <iostream>
#include <cstring> // for memset

// Define chordProfiles as a pointer to a 2D array
float (*chordProfiles)[12] = nullptr;

// Define the initialization flag
bool chordProfilesInitialized = false;


void createChordProfiles() {
    if (chordProfilesInitialized) return;

    // Allocate memory for chordProfiles
    chordProfiles = new float[108][12];

    // Initialize chordProfiles to zero
    memset(chordProfiles, 0, sizeof(float) * 108 * 12);

    //thanks to adam stark!
    int i;
	int t;
	int j = 0;
	int root;
	int third;
	int fifth;
	int seventh;
	
	float v1 = 1;
	float v2 = 1;
	float v3 = 1;
	
	// set profiles matrix to all zeros
	for (j = 0; j < 108; j++)
	{
		for (t = 0;t < 12;t++)
		{
			chordProfiles[j][t] = 0;
		}
	}
	
	// reset j to zero to begin creating profiles
	j = 0;
	
	// major chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 4) % 12;
		fifth = (i + 7) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}

	// minor chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 3) % 12;
		fifth = (i + 7) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}

	// diminished chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 3) % 12;
		fifth = (i + 6) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}	
	
	// augmented chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 4) % 12;
		fifth = (i + 8) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}	
	
	// sus2 chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 2) % 12;
		fifth = (i + 7) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}
	
	// sus4 chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 5) % 12;
		fifth = (i + 7) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		
		j++;				
	}		
	
	// major 7th chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 4) % 12;
		fifth = (i + 7) % 12;
		seventh = (i + 11) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		chordProfiles[j][seventh] = v3;
		
		j++;				
	}	
	
	// minor 7th chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 3) % 12;
		fifth = (i + 7) % 12;
		seventh = (i + 10) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		chordProfiles[j][seventh] = v3;
		
		j++;				
	}
	
	// dominant 7th chords
	for (i = 0; i < 12; i++)
	{
		root = i % 12;
		third = (i + 4) % 12;
		fifth = (i + 7) % 12;
		seventh = (i + 10) % 12;
		
		chordProfiles[j][root] = v1;
		chordProfiles[j][third] = v2;
		chordProfiles[j][fifth] = v3;
		chordProfiles[j][seventh] = v3;
		
		j++;				
	}


    chordProfilesInitialized = true;
}

void cleanupChordProfiles() {
    if (chordProfiles) {
        delete[] chordProfiles;
        chordProfiles = nullptr;
        chordProfilesInitialized = false;
    }
}

double calculateChordScore(float* chroma, float* chordProfile, double biasToUse, double N) {
    double sum = 0;
    double delta;

    for (int i = 0; i < 12; i++) {
        //sum = sum + ((1 - chordProfile[i]) * (chroma[i] * chroma[i]));
        sum = sum + ((1 - chordProfile[i]) * (chroma[i]));
    }

    delta = sqrt(sum) / ((12 - N) * biasToUse);
    return delta;
}


int minimumIndex (double* array, int arrayLength)
{
	double minValue = 100000;
	int minIndex = 0;
	
	for (int i = 0;i < arrayLength;i++)
	{
		if (array[i] < minValue)
		{
			minValue = array[i];
			minIndex = i;
		}
	}
	
	return minIndex;
}



int classifyChromagram(float* chromagram, float (*chordProfiles)[12]) {
    int i;
    int j;
    int fifth;
    int chordindex;
    double chord[108]; // Allocate memory for 108 doubles
    double bias = 1.06;

    // Remove some of the 5th note energy from chromagram
    /*
    for (i = 0; i < 12; i++) {
        fifth = (i + 7) % 12;
        chromagram[fifth] = chromagram[fifth] - (0.1 * chromagram[i]);
        if (chromagram[fifth] < 0) {
            chromagram[fifth] = 0;
        }
    }
    */
    
    
    

    // Major chords
    for (j = 0; j < 12; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 1, 3);
    }

    // major vs minor is tricky

    // Minor chords
    for (j = 12; j < 24; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 1, 3);
    }

    // Diminished 5th chords
    for (j = 24; j < 36; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 0.99, 3); //dim5 appears way too often
    }

    // Augmented 5th chords
    for (j = 36; j < 48; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 0.99, 3);
    }

    // Sus2 chords
    for (j = 48; j < 60; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 0.98, 3);
    }

    // Sus4 chords
    for (j = 60; j < 72; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 0.98, 3);
    }

    // Major 7th chords
    for (j = 72; j < 84; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 1, 4);
    }

    // Minor 7th chords
    for (j = 84; j < 96; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 1, 4);
    }

    // Dominant 7th chords
    for (j = 96; j < 108; j++) {
        chord[j] = calculateChordScore(chromagram, chordProfiles[j], 1, 4);
    }

    chordindex = minimumIndex(chord, 108);
    //chordValue = chord[chordindex];

    return chordindex;

    
}

