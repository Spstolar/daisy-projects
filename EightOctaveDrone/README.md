# Eight-Octave Drone

This is a simple patch for the Field. The keys will determine the root note, and a sine wave for that root as well its 7 octaves above will play. The knobs will determine the volume for each octave's sine wave allowing some simply play with harmonics. 

## TODO

* Use the CV inputs for filtering. 
* Use the switches to change the knob purposes from component volumes to various effects.

## Added

1. Show the knob values on the screen.
2. Normalize the final output. This was done by summing the values of the knobs while collecting them. Then scale the final output by (2 / sum of knobs) so that the sound where all knobs were at a quarter was the baseline.
3. Smoothly update the screen. This was done by following the example of the Vegas Mode for the field. Do not do the OLED updates in the audio processing function. Instead use the while loop in the main function to update the screen periodically given the current base note and the value of the knobs. This removes the glitchness that was happening while changing notes and allows the screen to update as if caused by changes in the knobs.
