/**
 * Copyright (C) 2015 Akop Karapetyan
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PI_MATRIX_H
#define PI_MATRIX_H

struct matrix {
	float xx, xy, xz, xw;
	float yx, yy, yz, yw;
	float zx, zy, zz, zw;
	float wx, wy, wz, ww;
};

void matrixIdentity(struct matrix *m);
void matrixCopy(struct matrix *dst, const struct matrix *src);
void matrixMultiply(struct matrix *r,
	const struct matrix *a, const struct matrix *b);
void matrixScale(struct matrix *m, float x, float y, float z);
void matrixRotateZ(struct matrix *m, float angle);
void matrixOrtho(struct matrix *m,
	float left, float right, float bottom, float top, float near, float far);
void matrixDump(const struct matrix *m);

#endif // PI_MATRIX_H
