#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <stdlib.h>
#include "st7735.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "spi.h"
#include "gpio.h"

int main()
{
	int fbfd = 0;

	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize = 0;
	char *fbp = 0;
	int x = 0, y = 0;
	long int location = 0;

	// Open the file for reading and writing
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		exit(1);
	}
	printf("The framebuffer device was opened successfully.\n");

	struct stat stat;
	fstat(fbfd, &stat);
	printf("/dev/mem -> size: %u blksize: %u blkcnt: %u\n",
	       stat.st_size, stat.st_blksize, stat.st_blocks);

	// Get fixed screen information
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		exit(2);
	}
	// Get variable screen information
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		exit(3);
	}

	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

	// Figure out the size of the screen in bytes
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
	const int PADDING = 4096;
	int mmapsize = (screensize + PADDING - 1) & ~(PADDING - 1);
	printf("screen size = %d KB\n", screensize / 1024);
	// Map the device to memory
	fbp =
	    (char *)mmap(0, mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd,
			 0);
	if ((int)fbp == -1) {
		perror("Error: failed to map framebuffer device to memory");
		exit(4);
	}
	printf("The framebuffer device was mapped to memory successfully.\n");

	int i;

	ST7735_Init();

	ST7735_Orientation(1);

	CS_L();

	ST7735_AddrSet(0, 0, 160, 120);

	A0_H();

	uint8_t fb2[128 * 160 * 2];

	int xx = 0;
	while (1) {
		CS_L();

		ST7735_AddrSet(0, 0, 160, 128);

		A0_H();

		char *p = fb2;
		char *x = fbp;
		for (xx = 0; xx < 128; xx++) {
			for (y = 0; y < 160; y++) {
				*p++ = *(x + 1);
				*p++ = *x;
				x += 2;
			}
		}

		p = fb2;

		//memcpy(fb2, fbp, 128*160*2);   

		for (i = 0; i < 10; i++) {
			spi_write(0, p, 4096);
			p += 4096;
		}
		//      spi_write(0, p, 120*160*2 - 4096*9);

	}

	CS_H();
	printf("done\n");

	volatile uint8_t bb[mmapsize];

	for (i = 0; i < mmapsize; i++)
		bb[i] = *fbp++;

	printf("copy done!");
	for (i = 0; i < mmapsize; i++)
		putchar(bb[i]);

	munmap(fbp, screensize);
	close(fbfd);

	return 0;
}
