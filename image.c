/****************************************************************************
 *            image.c
 *
 * Author: 2011  Daniel Jungmann <dsj@gmx.net>
 * Copyright: See COPYING file that comes with this distribution
 ****************************************************************************/

#include "image.h"

void unpack(const Uint8* source, const Uint32 count, const Uint32 red,
	const Uint32 green, const Uint32 blue, const Uint32 alpha, Uint8* dest)
{
	Uint32 i, pixel, temp, bpp;
	Uint32 r, g, b, a;
	Uint32 red_shift, red_mask;
	Uint32 green_shift, green_mask;
	Uint32 blue_shift, blue_mask;
	Uint32 alpha_shift, alpha_mask;

	bpp = popcount(red | green | blue | alpha) / 8;

	red_shift = 0;
	green_shift = 0;
	blue_shift = 0;
	alpha_shift = 0;

   while (red && (red & (1 << red_shift)) == 0)
   {
      red_shift++;
   }
   while (green && (green & (1 << green_shift)) == 0)
   {
      green_shift++;
   }
   while (blue && (blue & (1 << blue_shift)) == 0)
   {
      blue_shift++;
   }
   while (alpha && (alpha & (1 << alpha_shift)) == 0)
   {
      alpha_shift++;
   }

	red_mask = red >> red_shift;
	green_mask = green >> green_shift;
	blue_mask = blue >> blue_shift;
	alpha_mask = alpha >> alpha_shift;

	for (i = 0; i < count; i++)
	{
		memcpy(&pixel, &source[i * bpp], bpp);

		/* Get Red component */
		temp = pixel >> red_shift;
		temp = temp & red_mask;
		temp = (temp * 255) / red_mask;
		r = (Uint8)temp;

		/* Get Green component */
		temp = pixel >> green_shift;
		temp = temp & green_mask;
		temp = (temp * 255) / green_mask;
		g = (Uint8)temp;

		/* Get Blue component */
		temp = pixel >> blue_shift;
		temp = temp & blue_mask;
		temp = (temp * 255) / blue_mask;
		b = (Uint8)temp;

		/* Get Alpha component */
		if (alpha_mask != 0)
		{
			temp = pixel >> alpha_shift;
			temp = temp & alpha_mask;
			temp = (temp * 255) / alpha_mask;
			a = (Uint8)temp;
		}
		else
		{
			a = 255;
		}

		dest[i * 4 + 0] = r;
		dest[i * 4 + 1] = g;
		dest[i * 4 + 2] = b;
		dest[i * 4 + 3] = a;
	}
}

void fast_unpack(const Uint8* source, const Uint32 size, const Uint32 red,
	const Uint32 green, const Uint32 blue, const Uint32 alpha, Uint8* dest)
{
	unpack(source, size, red, green, blue, alpha, dest);
}

void fast_replace_a8_rgba8(const Uint8* alpha, const Uint32 size, Uint8* source)
{
	Uint32 i;


	for (i = 0; i < size; i++)
	{
		source[i * 4 + 3] = alpha[i];
	}
}

void fast_replace_alpha_rgba8(const Uint8 alpha, const Uint32 size, Uint8* source)
{
	Uint32 i;


	for (i = 0; i < size; i++)
	{
		source[i * 4 + 3] = alpha;
	}
}

void fast_blend(const Uint8* alpha, const Uint32 size, const Uint8* source0,
	const Uint8* source1, Uint8* dest)
{
	Uint32 i, j, tmp;


	for (i = 0; i < size; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp = source1[i * 4 + j] * alpha[i];
			tmp += source0[i * 4 + j] * (255 - alpha[i]);
			dest[i * 4 + j] = tmp / 255;
		}
	}
}

