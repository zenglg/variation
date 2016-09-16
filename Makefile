variation: variation.c
	gcc -o variation variation.c
clean:
	rm -rf *.o variation tmp tmp.c
