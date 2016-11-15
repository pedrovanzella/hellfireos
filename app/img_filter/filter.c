#include <hellfire.h>
#include <noc.h>
#include "image.h"

uint8_t numtasks = 1;

uint8_t gausian(uint8_t buffer[5][5]){
	int32_t sum = 0, mpixel;
	uint8_t i, j;

	int16_t kernel[5][5] =	{	{2, 4, 5, 4, 2},
					{4, 9, 12, 9, 4},
					{5, 12, 15, 12, 5},
					{4, 9, 12, 9, 4},
					{2, 4, 5, 4, 2}
				};
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			sum += ((int32_t)buffer[i][j] * (int32_t)kernel[i][j]);
	mpixel = (int32_t)(sum / 159);

	return (uint8_t)mpixel;
}

uint32_t isqrt(uint32_t a){
	uint32_t i, rem = 0, root = 0, divisor = 0;

	for (i = 0; i < 16; i++){
		root <<= 1;
		rem = ((rem << 2) + (a >> 30));
		a <<= 2;
		divisor = (root << 1) + 1;
		if (divisor <= rem){
			rem -= divisor;
			root++;
		}
	}
	return root;
}

uint8_t sobel(uint8_t buffer[3][3]){
	int32_t sum = 0, gx = 0, gy = 0;
	uint8_t i, j;

	int16_t kernelx[3][3] =	{	{-1, 0, 1},
					{-2, 0, 2},
					{-1, 0, 1},
				};
	int16_t kernely[3][3] =	{	{-1, -2, -1},
					{0, 0, 0},
					{1, 2, 1},
				};
	for (i = 0; i < 3; i++){
		for (j = 0; j < 3; j++){
			gx += ((int32_t)buffer[i][j] * (int32_t)kernelx[i][j]);
			gy += ((int32_t)buffer[i][j] * (int32_t)kernely[i][j]);
		}
	}
	
	sum = isqrt(gy * gy + gx * gx);

	if (sum > 255) sum = 255;
	if (sum < 0) sum = 0;

	return (uint8_t)sum;
}

void do_gausian(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[5][5];
	
	for(i = 0; i < height; i++){
		if (i > 1 || i < height-1){
			for(j = 0; j < width; j++){
				if (j > 1 || j < width-1){
					for (k = 0; k < 5;k++)
						for(l = 0; l < 5; l++)
							image_buf[k][l] = image[(((i + l-2) * width) + (j + k-2))];

					img[((i * width) + j)] = gausian(image_buf);
				}else{
					img[((i * width) + j)] = image[((i * width) + j)];
				}
			}
		}
	}
}

void do_sobel(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[3][3];
	
	for(i = 0; i < height; i++){
		if (i > 0 || i < height){
			for(j = 0; j < width; j++){
				if (j > 0 || j < width){
					for (k = 0; k < 3;k++)
						for(l = 0; l < 3; l++)
							image_buf[k][l] = image[(((i + l-1) * width) + (j + k-1))];

					img[((i * width) + j)] = sobel(image_buf);
				}else{
					img[((i * width) + j)] = image[((i * width) + j)];
				}
			}
		}
	}
}

void slave(void) {
	uint16_t ret;
	uint16_t cpu, port, size;
	uint8_t *img = (uint8_t *) malloc(height / numtasks * width / numtasks);

	if (hf_comm_create(hf_selfid(), 2000, 0)) {
		panic(0xff);
	}

	while(1) {
		// Recebe img de master
		ret = hf_recvack(&cpu, &port, img, &size, 0);
		printf("Slave: received image to be processed\n");
		if (!ret) {
			// Se nao deu erro nenhum, podemos processar
			do_gaussian(img, width / numtasks, height / numtasks);
			do_sobel(img, width / numtasks, height / numtasks);

			// retorna pro builder
			hf_sendack(0, 1000, img, sizeof(img), 0, 500);
			printf("Slave: sent processed image chunk\n");
		} else {
			// erro!!!
			printf("slave: deu ruim!\n");
		}
	}

}

void master(void){
	uint32_t i, j, k = 0;
	uint8_t *img;
	uint32_t time;

	if (hf_comm_create(hf_selfid(), 1000, 0)) {
		panic(0xff);
	}
	
	while(1) {
		printf("\n\nstart of processing!\n\n");

		time = _readcounter();

		// for i in (1, numtasks)
		// spawn slave com img cortada

		// envia img cortada
		for (i = 1; i <= numtasks; i++) {
			hf_sendack(i, 2000, image + numtasks * i, (width * height) / numtasks, 0, 500);
			printf("Master: sent image\n");
		}


		// this won't work
		// fica ouvindo por (numtasks) slaves
		uint16_t cpu, port, size;
		// Mudar isso para receber em um novo buffer
		uint8_t* buff = (uint8_t*) malloc(width * height / numtasks);
		hf_recvack(&cpu, &port, buff, size, 0);
		printf("Master: received processes image\n");
		// monta img com isso
		// pra montar: multiplica pelo numero da task
		memcopy(img + cpu * size, buff, size);

		time = _readcounter() - time;

		printf("done in %d clock cycles.\n\n", time);

		printf("\n\nint32_t width = %d, height = %d;\n", width, height);
		printf("uint8_t image[] = {\n");
		for (i = 0; i < height; i++){
			for (j = 0; j < width; j++){
				printf("0x%x", img[i * width + j]);
				if ((i < height-1) || (j < width-1)) printf(", ");
				if ((++k % 16) == 0) printf("\n");
			}
		}
		printf("};\n");

		free(img);

		printf("\n\nend of processing!\n");
		panic(0);
	}
		
}

void app_main(void) {
	if (hf_cpuid() == 0){
		hf_spawn(master, 0, 0, 0, "master", width * height);
	}
	int i;
	for (i = 1; i <= numtasks; i++) {
		if (hf_cpuid() == i) {
			char[8] name = "slave-";
			name[6] = (char)i; // should be a char, doesnt matter
			name[7] = '\0';
			hf_spawn(slave, 0, 0, 0, name, width * height);
		}
	}
}
