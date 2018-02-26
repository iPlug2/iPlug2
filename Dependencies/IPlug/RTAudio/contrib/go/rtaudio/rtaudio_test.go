package rtaudio

import (
	"log"
	"math"
	"time"
)

func ExampleCompiledAPI() {
	log.Println("RtAudio version: ", Version())
	for _, api := range CompiledAPI() {
		log.Println("Compiled API: ", api)
	}
}

func ExampleRtAudio_Devices() {
	audio, err := Create(APIUnspecified)
	if err != nil {
		log.Fatal(err)
	}
	defer audio.Destroy()
	devices, err := audio.Devices()
	if err != nil {
		log.Fatal(err)
	}
	for _, d := range devices {
		log.Printf("Audio device: %#v\n", d)
	}
}

func ExampleRtAudio_Open() {
	const (
		sampleRate = 44100
		bufSz      = 512
		freq       = 440.0
	)
	phase := 0.0
	audio, err := Create(APIUnspecified)
	if err != nil {
		log.Fatal(err)
	}
	defer audio.Destroy()

	params := StreamParams{
		DeviceID:     uint(audio.DefaultOutputDevice()),
		NumChannels:  2,
		FirstChannel: 0,
	}
	options := StreamOptions{
		Flags: FlagsAlsaUseDefault,
	}
	cb := func(out, in Buffer, dur time.Duration, status StreamStatus) int {
		samples := out.Float32()
		for i := 0; i < len(samples)/2; i++ {
			sample := float32(math.Sin(2 * math.Pi * phase))
			phase += freq / sampleRate

			samples[i*2] = sample
			samples[i*2+1] = sample
		}
		return 0
	}
	err = audio.Open(&params, nil, FormatFloat32, sampleRate, bufSz, cb, &options)
	if err != nil {
		log.Fatal(err)
	}
	defer audio.Close()
	audio.Start()
	defer audio.Stop()
	time.Sleep(3 * time.Second)
}
