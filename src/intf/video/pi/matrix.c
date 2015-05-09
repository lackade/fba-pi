/**
** Copyright (C) 2015 Akop Karapetyan
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
** http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "matrix.h"

void matrixIdentity(struct matrix *m)
{
	memset(m, 0, sizeof(struct matrix));
	m->xx = m->yy = m->zz = m->ww = 1.0f;
}

void matrixCopy(struct matrix *dst, const struct matrix *src)
{
	dst->xx = src->xx; dst->xy = src->xy; dst->xz = src->xz; dst->xw = src->xw;
	dst->yx = src->yx; dst->yy = src->yy; dst->yz = src->yz; dst->yw = src->yw;
	dst->zx = src->zx; dst->zy = src->zy; dst->zz = src->zz; dst->zw = src->zw;
	dst->wx = src->wx; dst->wy = src->wy; dst->wz = src->wz; dst->ww = src->ww;
}

void matrixMultiply(struct matrix *r,
	const struct matrix *a, const struct matrix *b)
{
	r->xx = a->xx * b->xx + a->xy * b->yx + a->xz * b->zx + a->xw * b->wx;
	r->xy = a->xx * b->xy + a->xy * b->yy + a->xz * b->zy + a->xw * b->wy;
	r->xz = a->xx * b->xz + a->xy * b->yz + a->xz * b->zz + a->xw * b->wz;
	r->xw = a->xx * b->xw + a->xy * b->yw + a->xz * b->zw + a->xw * b->ww;

	r->yx = a->yx * b->xx + a->yy * b->yx + a->yz * b->zx + a->yw * b->wx;
	r->yy = a->yx * b->xy + a->yy * b->yy + a->yz * b->zy + a->yw * b->wy;
	r->yz = a->yx * b->xz + a->yy * b->yz + a->yz * b->zz + a->yw * b->wz;
	r->yw = a->yx * b->xw + a->yy * b->yw + a->yz * b->zw + a->yw * b->ww;

	r->zx = a->zx * b->xx + a->zy * b->yx + a->zz * b->zx + a->zw * b->wx;
	r->zy = a->zx * b->xy + a->zy * b->yy + a->zz * b->zy + a->zw * b->wy;
	r->zz = a->zx * b->xz + a->zy * b->yz + a->zz * b->zz + a->zw * b->wz;
	r->zw = a->zx * b->xw + a->zy * b->yw + a->zz * b->zw + a->zw * b->ww;

	r->wx = a->wx * b->xx + a->wy * b->yx + a->wz * b->zx + a->ww * b->wx;
	r->wy = a->wx * b->xy + a->wy * b->yy + a->wz * b->zy + a->ww * b->wy;
	r->wz = a->wx * b->xz + a->wy * b->yz + a->wz * b->zz + a->ww * b->wz;
	r->ww = a->wx * b->xw + a->wy * b->yw + a->wz * b->zw + a->ww * b->ww;
}

void matrixScale(struct matrix *m, float x, float y, float z)
{
	struct matrix mcopy;
	matrixCopy(&mcopy, m);

	struct matrix smat;
	matrixIdentity(&smat);

	smat.xx = x;
	smat.yy = y;
	smat.zz = z;

	matrixMultiply(m, &mcopy, &smat);
}

void matrixRotateZ(struct matrix *m, float angle)
{
	struct matrix mcopy;
	matrixCopy(&mcopy, m);

	struct matrix rmat;
	matrixIdentity(&rmat);

	float c = cos(angle * M_PI / 180.0);
	float s = sin(angle * M_PI / 180.0);

	rmat.xx = c;
	rmat.xy = -s;
	rmat.yx = s;
	rmat.yy = c;

	matrixMultiply(m, &mcopy, &rmat);
}

void matrixOrtho(struct matrix *m,
	float left, float right, float bottom, float top, float near, float far)
{
	struct matrix mcopy;
	matrixCopy(&mcopy, m);

	struct matrix omat;
	matrixIdentity(&omat);

	omat.xx = 2.0f / (right - left);
	omat.yy = 2.0f / (top - bottom);
	omat.zz = 2.0f / (near - far);
	omat.wx = -(right + left) / (right - left);
	omat.wy = -(top + bottom) / (top - bottom);
	omat.wz = -(far + near) / (far - near);

	matrixMultiply(m, &mcopy, &omat);
}

void matrixDump(const struct matrix *m)
{
	printf("[%.02f,%.02f,%.02f,%.02f]\n", m->xx, m->xy, m->xz, m->xw);
	printf("[%.02f,%.02f,%.02f,%.02f]\n", m->yx, m->yy, m->yz, m->yw);
	printf("[%.02f,%.02f,%.02f,%.02f]\n", m->zx, m->zy, m->zz, m->zw);
	printf("[%.02f,%.02f,%.02f,%.02f]\n", m->wx, m->wy, m->wz, m->ww);
}
