#ifndef SWR_SWR_H
#error "Must be included from swr.h"
#endif

static inline void swrMatrix2DIdentity(swr_matrix2d* mtx)
{
	mtx->m_Elem[0] = 1.0f;
	mtx->m_Elem[1] = 0.0f;
	mtx->m_Elem[2] = 0.0f;
	mtx->m_Elem[3] = 1.0f;
	mtx->m_Elem[4] = 0.0f;
	mtx->m_Elem[5] = 0.0f;
}

static inline void swrMatrix2DTranslate(swr_matrix2d* mtx, float x, float y)
{
	mtx->m_Elem[4] += mtx->m_Elem[0] * x + mtx->m_Elem[2] * y;
	mtx->m_Elem[5] += mtx->m_Elem[1] * x + mtx->m_Elem[3] * y;
}

static inline void swrMatrix2DScale(swr_matrix2d* mtx, float x, float y)
{
	mtx->m_Elem[0] *= x;
	mtx->m_Elem[1] *= x;
	mtx->m_Elem[2] *= y;
	mtx->m_Elem[3] *= y;
}

static inline void swrMatrix2DRotate(swr_matrix2d* mtx, float angle_deg)
{
	const float ang_rad = core_toRad(angle_deg);
	const float c = core_cosf(ang_rad);
	const float s = core_sinf(ang_rad);

	const float m0 = mtx->m_Elem[0] * c + mtx->m_Elem[2] * s;
	const float m1 = mtx->m_Elem[1] * c + mtx->m_Elem[3] * s;
	const float m2 = mtx->m_Elem[2] * c - mtx->m_Elem[0] * s;
	const float m3 = mtx->m_Elem[3] * c - mtx->m_Elem[1] * s;

	mtx->m_Elem[0] = m0;
	mtx->m_Elem[1] = m1;
	mtx->m_Elem[2] = m2;
	mtx->m_Elem[3] = m3;
}
