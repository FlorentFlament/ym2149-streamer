Title: ym2149-streamer

The ym2149-streamer allows sending Atari ST YM files to a YM2149 chip
driven by an Arduino.

Requirements
------------

The following libraries are required:

* avr-gcc
* avr-libc
* avrdude

How to do
---------

    $ make
    $ make flash
    $ python streamer.py <output_device> <ym_filepath>

Where <output_device> would typically be `/dev/ttyUSB0`.

More information
----------------

More information can be found on my blog:

* [Streaming music to YM2149F][1]
* [Driving YM2149F sound chip with an Arduino][2]
* [Arduino Hello World without IDE][3]

Besides, a video showing the [YM2149 & Arduino circuit playing a tune][4] is
available.


[1]: http://www.florentflament.com/blog/streaming-music-to-ym2149f.html
[2]: http://www.florentflament.com/blog/driving-ym2149f-sound-chip-with-an-arduino.html
[3]: http://www.florentflament.com/blog/arduino-hello-world-without-ide.html
[4]: https://www.youtube.com/watch?v=MTRJdDbY048
