reader: reader.c
	gcc -I /opt/local/include -L /opt/local/lib -l hackrf -o reader reader.c

sender: sender.c
	gcc -I /opt/local/include -L /opt/local/lib -l hackrf -o sender sender.c

vis: vis.c
	gcc -I /opt/local/include -L /opt/local/lib -o vis vis.c -l glfw -l hackrf -lpng -framework OpenGL

gridvis: gridvis.c
	gcc -I /opt/local/include -L /opt/local/lib -o gridvis gridvis.c -l glfw -l hackrf -lpng -framework OpenGL

batch: batch.c
	gcc -I /opt/local/include -L /opt/local/lib -o batch batch.c -l hackrf -lpng

fft: fft.c
	gcc -I /opt/local/include -L /opt/local/lib -o fft fft.c -l hackrf -lpng -lfftw3 -lm -l glfw -framework OpenGL