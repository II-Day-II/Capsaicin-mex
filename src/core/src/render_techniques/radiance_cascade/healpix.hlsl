#ifndef HEALPIX_HLSL
#define HEALPIX_HLSL

#include "../../math/math_constants.hlsl"

// All this is from https://github.com/ntessore/healpix/blob/main/src/healpix.c

const int JPLL[] = { 1, 3, 5, 7, 0, 2, 4, 6, 1, 3, 5, 7 };

// a structure describing a location in cylindrical coordinates
// z = cos(theta), s = sin(theta), phi
struct loc_
{
    float z, s, phi;
};

// a structure describing the healpix coordinate space
// x, y are in [0, nside)
// face is in [0, 11]
struct hpd_
{
    uint x, y;
    uint face;
};

uint compress_bits(uint v) {
  uint res = v & 0x55555555;
  res = (res^(res>> 1)) & 0x33333333;
  res = (res^(res>> 2)) & 0x0f0f0f0f;
  res = (res^(res>> 4)) & 0x00ff00ff;
  res = (res^(res>> 8)) & 0x0000ffff;
  return res;
}

hpd_ nest2hpd(uint nside, uint pix) 
{
	uint npface = nside*nside;
	uint p2 = pix & (npface - 1);
	hpd_ hpd = {
		compress_bits(p2), 
		compress_bits(p2 >> 1), 
		pix / npface 
	};
    return hpd;
}

loc_ hpd2loc(uint nside, hpd_ hpd, float2 uv)
{
	float z, s, phi;

	const float x = (hpd.x+uv.x) / nside;
	const float y = (hpd.y+uv.y) / nside;
	const int r = 1 - hpd.face / 4;
	const float h = r - 1 + x + y;
	float m = 2 - r * h;
	if (m < 1.) 
	{
		// polar cap
		float tmp = m*m* 1./3.;
		z = r * (1. - tmp);
		s = sqrt(tmp * (2. - tmp));
		phi = (PI/4)*JPLL[hpd.face] + (x - y) / m;
	}
	else 
	{
		// equatorial region
		z = h * (2. / 3.);
		s = sqrt((1. + z) * (1. - z));
		phi = (PI / 4)*JPLL[hpd.face] + x - y;
	}
    loc_ loc = { z, s, phi };
    return loc;
}

float3 loc2dir(loc_ loc)
{
	return float3(loc.s * cos(loc.phi), loc.s * sin(loc.phi), loc.z);
}

float3 healpix_index_to_direction(uint nside, uint ipix)
{
    return loc2dir(hpd2loc(nside, nest2hpd(nside, ipix), float2(0.5, 0.5)));
}

#endif //HEALPIX_HLSL
