// Using a sample buffer will allow audio() to get called less often, with more samples.
// This offloads the processor from having to do alot of context switching.
SampleBuffer buffer[1024];

// This is where recorded samples end up, and where you generate new samples for playback.
void audio(samp32bit recv[][2], samp32bit send[][2], int nsamples) {
  // Copy input to output (thru)
  memcpy(send, recv, nsamples * sizeof(samp32bit[2]));
}

void setup() {
  // Initialize audio system
  Audio.begin(AUDIO_48KHZ, AUDIO_32BIT, true, buffer, sizeof(buffer));
  // Left+right headphone volume = 100, load on zero cross = true
  Audio.hpVolume(110, true);
  // Line in volume = 31, mute = false
  Audio.lineInVolume(31, false);  
}

void loop() {
}
