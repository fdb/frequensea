reader: reader.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o reader reader.c

sender: sender.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o sender sender.c

vis: vis.c
	gcc -I /usr/local/include -L /usr/local/lib -o vis vis.c -l glfw3 -l hackrf -lpng -framework OpenGL

gridvis: gridvis.c
	gcc -I /usr/local/include -L /usr/local/lib -o gridvis gridvis.c -l glfw3 -l hackrf -lpng -framework OpenGL

rfcap: rfcap.c
	gcc -I /usr/local/include -L /usr/local/lib -l hackrf -o rfcap rfcap.c

batch: batch.c
	gcc -I /usr/local/include -L /usr/local/lib -o batch batch.c -l hackrf -lpng

fft: fft.c
	gcc -I /usr/local/include -L /usr/local/lib -o fft fft.c -l hackrf -lpng -lfftw3 -lm -l glfw3 -framework OpenGL

iqvis: iqvis.c
	gcc -I /usr/local/include -L /usr/local/lib -o iqvis iqvis.c -l glfw3 -l hackrf -lpng -framework OpenGL

audio: audio.c
	gcc -framework OpenAL -o audio audio.c
