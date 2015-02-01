# Euroduino

Sketches for the Circuit Abbey Euroduino module. 

Most of these sketches are documented in the code itself; but I may have
updated the code more recently than the comments.

These will generally be adaptations of my Ardcore sketches.


## ed3_rls
A random looping sequencer in the style of the Turing Machine and its
predecessors. The output can be quantised with user selectable scales 
and a sequence length.

Sequence length is switch selectable between 8/16/32 bits.
The sequence is implemented internally as an array rather than as a shift
register. The sequence length always counts from the start of the array, the
shifting is done by moving a window through the array. The upshot is that (for
now at least) for a locked sequence, the minimum sequence length always refers to the same
8 positions, not those coming up in the sequence.

* pot1 is chance of bit getting flipped (changing the sequence), CCW no chance -- so
sequence is locked, CW is always (so sequence is locked at double length,
2nd part is the inverse of the first).
* pot2 User selectable scale for quantisation (details in the sketch). CCW is
unquantised.
* sw1 length of shift register, up is 32 bits, middle is 16 bits, down is
8 bits
* sw2 Range for the output, up is full range, middle is half range, down is 1/4
range 
* dig1in - clock in
* D0 trigger following bit flip on the sequence, D1 does the opposite, so either D0 or D1 fires each clock pulse.
* cv1out - quantised output
* cv2out - same output but unquantised





