add-markers: add-markers.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o add-markers add-markers.c -lpng

reader: reader.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o reader reader.c

sender: sender.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o sender sender.c

tv-sender: tv-sender.c
	gcc -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -l hackrf -o tv-sender tv-sender.c

vis: vis.c
	gcc -I /usr/local/include -L /usr/local/lib -o vis vis.c -l glfw3 -l hackrf -lpng -framework OpenGL

gradual-noise: gradual-noise.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o gradual-noise gradual-noise.c -lpng

gridvis: gridvis.c
	gcc -I /usr/local/include -L /usr/local/lib -o gridvis gridvis.c -l glfw3 -l hackrf -lpng -framework OpenGL

rfcap: rfcap.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o rfcap rfcap.c

batch: batch.c
	gcc -I /usr/local/include -L /usr/local/lib -o batch batch.c -l hackrf -lpng

fft: fft.c
	gcc -I /usr/local/include -L /usr/local/lib -o fft fft.c -l hackrf -lpng -lfftw3 -lm -l glfw3 -framework OpenGL

fft-batch: fft-batch.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o fft-batch fft-batch.c -lhackrf -lpng -lfftw3

fft-batch-broad: fft-batch-broad.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o fft-batch-broad fft-batch-broad.c -lhackrf -lpng -lfftw3

fft-stitch: fft-stitch.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o fft-stitch fft-stitch.c -lpng

fft-stitch-broad: fft-stitch-broad.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -o fft-stitch-broad fft-stitch-broad.c -lpng

iq-lines: iq-lines.c
	gcc --std=c99 -g -Wall -Werror -pedantic `pkg-config --cflags --libs --static libpng libhackrf glfw3` -o iq-lines iq-lines.c

iqvis: iqvis.c
	gcc -I /usr/local/include -L /usr/local/lib -o iqvis iqvis.c -l glfw3 -l hackrf -lpng -framework OpenGL

iqvis-rtl: iqvis-rtl.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I/usr/local/include `pkg-config --cflags libpng librtlsdr glfw3 glew` -o iqvis-rtl iqvis-rtl.c -L/usr/local/lib `pkg-config --libs --static libpng librtlsdr glfw3 glew`

demod-fm: demod-fm.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -l hackrf -framework OpenAL -o demod-fm demod-fm.c

demod-fm-streaming: demod-fm-streaming.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -l hackrf -framework OpenAL -o demod-fm-streaming demod-fm-streaming.c

osc-server: osc-server.c
	gcc --std=c99 -g -Wall -Werror -pedantic -o osc-server osc-server.c

play: play.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I /usr/local/include -L /usr/local/lib -l sndfile -framework OpenAL -o play play.c

piqvis: piqvis.c
	gcc --std=c99 -g -Wall -Werror -pedantic -I/usr/local/include `pkg-config --cflags librtlsdr glfw3 glew` -o piqvis piqvis.c -L/usr/local/lib `pkg-config --libs --static librtlsdr glfw3 glew`

single-sample: single-sample.c
	gcc --std=c99 -g -Wall -Werror -pedantic `pkg-config --cflags --libs libpng` -o single-sample single-sample.c

render-text: render-text.c
	gcc --std=c99 -g -Wall -Werror -pedantic `pkg-config --cflags --libs libpng` -o render-text render-text.c
