#include "burnint.h"
#include "burn_gun.h"

// Generic Light Gun support for FBA
// written by Barry Harris (Treble Winner) based on the code in Kev's opwolf driver

INT32 nBurnGunNumPlayers = 0;
bool bBurnGunAutoHide = 1;
static bool bBurnGunDrawTargets = true;

static INT32 nBurnGunMaxX = 0;
static INT32 nBurnGunMaxY = 0;

INT32 BurnGunX[MAX_GUNS];
INT32 BurnGunY[MAX_GUNS];

struct GunWrap { INT32 xmin; INT32 xmax; INT32 ymin; INT32 ymax; };
static GunWrap BurnGunWrapInf[MAX_GUNS]; // Paddle/Dial use

#define a 0,
#define b 1,

UINT8 BurnGunTargetData[18][18] = {
	{ a a a a  a a a a  b a a a  a a a a  a a },
	{ a a a a  a a b b  b b b a  a a a a  a a },
	{ a a a a  b b a a  b a a b  b a a a  a a },
	{ a a a b  a a a a  b a a a  a b a a  a a },
	{ a a b a  a a a a  b a a a  a a b a  a a },
	{ a a b a  a a a b  b b a a  a a b a  a a },
	{ a b a a  a a b b  b b b a  a a a b  a a },
	{ a b a a  a b b a  a a a b  a a a b  a a },
	{ b b b b  b b b a  a a b b  b b b b  b a },
	{ a b a a  a b b a  a a a b  b a a b  a a },
	{ a b a a  a a b a  b a b b  a a a b  a a },
	{ a a b a  a a a b  b b b a  a a b a  a a },
	{ a a b a  a a a a  b b a a  a a b a  a a },
	{ a a a b  a a a a  b a a a  a b a a  a a },
	{ a a a a  b b a a  b a a b  b a a a  a a },
	{ a a a a  a a b b  b b b a  a a a a  a a },
	{ a a a a  a a a a  b a a a  a a a a  a a },
	{ a a a a  a a a a  a a a a  a a a a  a a },
};
#undef b
#undef a

#define GunTargetHideTime 60 * 4 /* 4 seconds @ 60 fps */
static INT32 GunTargetTimer[MAX_GUNS]  = {0, 0, 0, 0};
static INT32 GunTargetLastX[MAX_GUNS]  = {0, 0, 0, 0};
static INT32 GunTargetLastY[MAX_GUNS]  = {0, 0, 0, 0};

static void GunTargetUpdate(INT32 player)
{
	if (GunTargetLastX[player] != BurnGunReturnX(player) || GunTargetLastY[player] != BurnGunReturnY(player)) {
		GunTargetLastX[player] = BurnGunReturnX(player);
		GunTargetLastY[player] = BurnGunReturnY(player);
		GunTargetTimer[player] = nCurrentFrame;
	}
}

static UINT8 GunTargetShouldDraw(INT32 player)
{
	return ((INT32)nCurrentFrame < GunTargetTimer[player] + GunTargetHideTime);
}
#undef GunTargetHideTime

UINT8 BurnGunReturnX(INT32 num)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunReturnX called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnGunReturnX called with invalid player %x\n"), num);
#endif

	if (num > MAX_GUNS - 1) return 0xff;

	float temp = (float)((BurnGunX[num] >> 8) + 8) / nBurnGunMaxX * 0xff;
	return (UINT8)temp;
}

UINT8 BurnGunReturnY(INT32 num)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunReturnY called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnGunReturnY called with invalid player %x\n"), num);
#endif

	if (num > MAX_GUNS - 1) return 0xff;
	
	float temp = (float)((BurnGunY[num] >> 8) + 8) / nBurnGunMaxY * 0xff;
	return (UINT8)temp;
}

// Paddle/Dial stuff
static INT32 PaddleLastA[MAX_GUNS];
static INT32 PaddleLastB[MAX_GUNS];

BurnDialINF BurnPaddleReturnA(INT32 num)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnPaddleReturnA called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnPaddleReturnA called with invalid player %x\n"), num);
#endif

	BurnDialINF dial = { 0, 0, 0 };

	if (num > MAX_GUNS - 1) return dial;

	INT32 PaddleA = ((BurnGunX[num] >> 8) / 0x4) & 0xff;

	if (PaddleA < PaddleLastA[num]) {
		dial.Velocity = (PaddleLastA[num] - PaddleA);
		dial.Backward = 1;
	}
	else if (PaddleA > PaddleLastA[num]) {
		dial.Velocity = (PaddleA - PaddleLastA[num]);
		dial.Forward = 1;
	}

	PaddleLastA[num] = PaddleA;

	return dial;
}

BurnDialINF BurnPaddleReturnB(INT32 num)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnPaddleReturnB called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnPaddleReturnB called with invalid player %x\n"), num);
#endif

	BurnDialINF dial = { 0, 0, 0 };

	if (num > MAX_GUNS - 1) return dial;

	INT32 PaddleB = ((BurnGunY[num] >> 8) / 0x4) & 0xff;

	if (PaddleB < PaddleLastB[num]) {
		dial.Velocity = (PaddleLastB[num] - PaddleB);
		dial.Backward = 1;
	}
	else if (PaddleB > PaddleLastB[num]) {
		dial.Velocity = (PaddleB - PaddleLastB[num]);
		dial.Forward = 1;
	}

	PaddleLastB[num] = PaddleB;

	return dial;
}

void BurnPaddleSetWrap(INT32 num, INT32 xmin, INT32 xmax, INT32 ymin, INT32 ymax)
{
	BurnGunWrapInf[num].xmin = xmin * 0x10; BurnGunWrapInf[num].xmax = xmax * 0x10;
	BurnGunWrapInf[num].ymin = ymin * 0x10; BurnGunWrapInf[num].ymax = ymax * 0x10;
}

void BurnPaddleMakeInputs(INT32 num, INT16 x, INT16 y)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunMakeInputs called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnGunMakeInputs called with invalid player %x\n"), num);
#endif

	if (num > MAX_GUNS - 1) return;
	
	if (y == 1 || y == -1) y = 0;
	if (x == 1 || x == -1) x = 0; // prevent walking crosshair

	BurnGunX[num] += x;
	BurnGunY[num] += y;

	// Wrapping (for dial/paddle use)
	if (BurnGunWrapInf[num].xmin != -1)
		if (BurnGunX[num] < BurnGunWrapInf[num].xmin * 0x100) {
			BurnGunX[num] = BurnGunWrapInf[num].xmax * 0x100;
			BurnPaddleReturnA(num); // rebase PaddleLast* on wrap
		}
	if (BurnGunWrapInf[num].xmax != -1)
		if (BurnGunX[num] > BurnGunWrapInf[num].xmax * 0x100) {
			BurnGunX[num] = BurnGunWrapInf[num].xmin * 0x100;
			BurnPaddleReturnA(num); // rebase PaddleLast* on wrap
		}

	if (BurnGunWrapInf[num].ymin != -1)
		if (BurnGunY[num] < BurnGunWrapInf[num].ymin * 0x100) {
			BurnGunY[num] = BurnGunWrapInf[num].ymax * 0x100;
			BurnPaddleReturnB(num); // rebase PaddleLast* on wrap
		}
	if (BurnGunWrapInf[num].ymax != -1)
		if (BurnGunY[num] > BurnGunWrapInf[num].ymax * 0x100) {
			BurnGunY[num] = BurnGunWrapInf[num].ymin * 0x100;
			BurnPaddleReturnB(num); // rebase PaddleLast* on wrap
		}
}

void BurnGunMakeInputs(INT32 num, INT16 x, INT16 y)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunMakeInputs called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnGunMakeInputs called with invalid player %x\n"), num);
#endif

	if (num > MAX_GUNS - 1) return;
	
	const INT32 MinX = -8 * 0x100;
	const INT32 MinY = -8 * 0x100;

	if (y == 1 || y == -1) y = 0;
	if (x == 1 || x == -1) x = 0; // prevent walking crosshair

	BurnGunX[num] += x;
	BurnGunY[num] += y;

	if (BurnGunX[num] < MinX) BurnGunX[num] = MinX;
	if (BurnGunX[num] > MinX + nBurnGunMaxX * 0x100) BurnGunX[num] = MinX + nBurnGunMaxX * 0x100;
	if (BurnGunY[num] < MinY) BurnGunY[num] = MinY;
	if (BurnGunY[num] > MinY + nBurnGunMaxY * 0x100) BurnGunY[num] = MinY + nBurnGunMaxY * 0x100;

	for (INT32 i = 0; i < nBurnGunNumPlayers; i++)
		GunTargetUpdate(i);
}

void BurnGunInit(INT32 nNumPlayers, bool bDrawTargets)
{
	Debug_BurnGunInitted = 1;
	
	if (nNumPlayers > MAX_GUNS) nNumPlayers = MAX_GUNS;
	nBurnGunNumPlayers = nNumPlayers;
	bBurnGunDrawTargets = bDrawTargets;
	
	if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
		BurnDrvGetVisibleSize(&nBurnGunMaxY, &nBurnGunMaxX);
	} else {
		BurnDrvGetVisibleSize(&nBurnGunMaxX, &nBurnGunMaxY);
	}
	
	for (INT32 i = 0; i < MAX_GUNS; i++) {
		BurnGunX[i] = ((nBurnGunMaxX >> 1) - 7) << 8;
		BurnGunY[i] = ((nBurnGunMaxY >> 1) - 8) << 8;

		BurnPaddleSetWrap(i, 0, 0xf0, 0, 0xf0); // Paddle/dial stuff
	}
}

void BurnGunExit()
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunExit called without init\n"));
#endif

	nBurnGunNumPlayers = 0;
	bBurnGunDrawTargets = true;
	
	nBurnGunMaxX = 0;
	nBurnGunMaxY = 0;
	
	for (INT32 i = 0; i < MAX_GUNS; i++) {
		BurnGunX[i] = 0;
		BurnGunY[i] = 0;
	}
	
	Debug_BurnGunInitted = 0;
}

void BurnGunScan()
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunScan called without init\n"));
#endif

	SCAN_VAR(BurnGunX);
	SCAN_VAR(BurnGunY);
}

void BurnGunDrawTarget(INT32 num, INT32 x, INT32 y)
{
#if defined FBA_DEBUG
	if (!Debug_BurnGunInitted) bprintf(PRINT_ERROR, _T("BurnGunDrawTarget called without init\n"));
	if (num >= nBurnGunNumPlayers) bprintf(PRINT_ERROR, _T("BurnGunDrawTarget called with invalid player %x\n"), num);
#endif

	if (bBurnGunDrawTargets == false) return;
	
	if (num > MAX_GUNS - 1) return;

	if (bBurnGunAutoHide && !GunTargetShouldDraw(num)) return;

	UINT8* pTile = pBurnDraw + nBurnGunMaxX * nBurnBpp * (y - 1) + nBurnBpp * x;
	
	UINT32 nTargetCol = 0;
	if (num == 0) nTargetCol = BurnHighCol(0xfc, 0x12, 0xee, 0);
	if (num == 1) nTargetCol = BurnHighCol(0x1c, 0xfc, 0x1c, 0);
	if (num == 2) nTargetCol = BurnHighCol(0x15, 0x93, 0xfd, 0);
	if (num == 3) nTargetCol = BurnHighCol(0xf7, 0xfa, 0x0e, 0);

	for (INT32 y2 = 0; y2 < 17; y2++) {

		pTile += nBurnGunMaxX * nBurnBpp;

		if ((y + y2) < 0 || (y + y2) > nBurnGunMaxY - 1) {
			continue;
		}

		for (INT32 x2 = 0; x2 < 17; x2++) {

			if ((x + x2) < 0 || (x + x2) > nBurnGunMaxX - 1) {
				continue;
			}

			if (BurnGunTargetData[y2][x2]) {
				if (nBurnBpp == 2) {
					((UINT16*)pTile)[x2] = (UINT16)nTargetCol;
				} else {
					((UINT32*)pTile)[x2] = nTargetCol;
				}
			}
		}
	}
}

void BurnGunDrawTargets()
{
	for (INT32 i = 0; i < nBurnGunNumPlayers; i++) {
		BurnGunDrawTarget(i, BurnGunX[i] >> 8, BurnGunY[i] >> 8);
	}
}
