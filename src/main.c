#include <stdint.h>
#include <memory.h>
#include <stdlib.h> // rand
#include <MiniFB.h>
#include "core/core.h"
#include "core/cpu.h"
#include "core/allocator.h"
#include "core/os.h"
#include "core/string.h"
#include "core/memory.h"
#include "swr/swr.h"
#include "fonts/font8x8_basic.h"
#include "m6502_mesh.h"

static const uint32_t kWinWidth = 1280;
static const uint32_t kWinHeight = 720;

typedef struct mesh_t
{
	uint16_t* m_IndexBuffer;
	uint32_t m_NumIndices;

	float* m_PosBuffer;
	uint32_t* m_ColorBuffer;
	uint32_t m_NumVertices;
} mesh_t;

typedef struct drawcall_t
{
	uint32_t m_BaseVertex;
	uint32_t m_BaseIndex;
	uint32_t m_NumIndices;
} drawcall_t;

#define MOVING_AVG_NUM_SAMPLES 512

typedef struct stats_t
{
	double m_Min;
	double m_Percent25;
	double m_Median;
	double m_Percent75;
	double m_Max;
	double m_Average;
	double m_StdDev;
} stats_t;

typedef struct movavgd_t
{
	double m_Value[MOVING_AVG_NUM_SAMPLES];
	double m_StatsBuffer[MOVING_AVG_NUM_SAMPLES]; // Temporary buffer for stats. Used for sorting values.
	uint32_t m_NumItems;
	uint32_t m_NextItemID;
} movavgd_t;

static void movAvgPush(movavgd_t* avg, double val);
static double movAvgGetAverage(const movavgd_t* avg);
static void movAvgGetMinMax(const movavgd_t* avg, double* tmin, double* tmax);
static void movAvgGetStats(movavgd_t* avg, stats_t* s);

#if 0
static bool meshLoadRenderDocBuffers(mesh_t* m, core_file_base_dir baseDir, const char* ibPath, const char* vbPosPath, const char* vbColorPath, core_allocator_i* allocator);
#endif
static bool meshBuild6502(mesh_t* m, drawcall_t** drawCalls, uint32_t* numDrawCalls, core_allocator_i* allocator);

int32_t main(void)
{
	coreInit(CORE_CPU_FEATURE_MASK_ALL);

	core_allocator_i* allocator = core_allocatorCreateAllocator("app");

	struct mfb_window* window = mfb_open_ex("swr", kWinWidth, kWinHeight, 0);
	if (!window) {
		return -1;
	}
	mfb_set_target_fps(0);

	swr_context* swrCtx = swr->createContext(allocator, kWinWidth, kWinHeight);
	if (!swrCtx) {
		mfb_close(window);
		return -1;
	}

	swr_font font = {
		.m_CharData = kFont8x8_basic,
		.m_CharWidth = 8,
		.m_CharHeight = 8,
		.m_CharMin = 0,
		.m_CharMax = 0x7F,
		.m_MissingCharFallbackID = 0
	};

	mesh_t mesh;
	drawcall_t* drawCalls = NULL;
	uint32_t numDrawCalls = 0;
	if (!meshBuild6502(&mesh, &drawCalls, &numDrawCalls, allocator)) {
		return -1;
	}

	int32_t* transformedVertices = (int32_t*)CORE_ALLOC(allocator, sizeof(int32_t) * 2 * mesh.m_NumVertices);
	if (!transformedVertices) {
		return -1;
	}

	movavgd_t frameTimeAvg;
	core_memSet(&frameTimeAvg, 0, sizeof(movavgd_t));

	do {
		swr->clear(swrCtx, SWR_COLOR_BLACK);

		const uint64_t tStart = core_osTimeNow();
		{
			const float size = 680.0f;
			const float padX = ((float)kWinWidth - size) * 0.5f;
			const float padY = ((float)kWinHeight - size) * 0.5f;

			// Transform vertices to screen space
			{
				const uint32_t numVertices = mesh.m_NumVertices;
				const float* vertexPosf = mesh.m_PosBuffer;
				int32_t* vertexPosi = transformedVertices;
				for (uint32_t i = 0; i < numVertices; ++i) {
					const float fx = vertexPosf[i * 2 + 0];
					const float fy = vertexPosf[i * 2 + 1];
					const int32_t ix = (int32_t)(((fx - 215.0f) / (8984.0f - 215.0f)) * size + padX);
					const int32_t iy = (int32_t)(((fy - 180.0f) / (9808.0f - 180.0f)) * size + padY);
					vertexPosi[i * 2 + 0] = ix;
					vertexPosi[i * 2 + 1] = iy;
				}
			}

			for (uint32_t idc = 0; idc < numDrawCalls; ++idc) {
				const drawcall_t* curDC = &drawCalls[idc];
				const uint16_t* ib = &mesh.m_IndexBuffer[curDC->m_BaseIndex];
				const int32_t* posVB = &transformedVertices[curDC->m_BaseVertex * 2];
				const uint32_t* colorVB = &mesh.m_ColorBuffer[curDC->m_BaseVertex];

				const uint32_t numTriangles = curDC->m_NumIndices / 3;
				for (uint32_t i = 0; i < numTriangles; ++i) {
					const uint16_t id0 = *ib++;
					const uint16_t id1 = *ib++;
					const uint16_t id2 = *ib++;

					const int32_t x0 = posVB[id0 * 2 + 0];
					const int32_t y0 = posVB[id0 * 2 + 1];
					const int32_t x1 = posVB[id1 * 2 + 0];
					const int32_t y1 = posVB[id1 * 2 + 1];
					const int32_t x2 = posVB[id2 * 2 + 0];
					const int32_t y2 = posVB[id2 * 2 + 1];
					const uint32_t c0 = colorVB[id0];
					const uint32_t c1 = colorVB[id1];
					const uint32_t c2 = colorVB[id2];

					swr->drawTriangle(swrCtx, x0, y0, x1, y1, x2, y2, c0, c1, c2);
				}
			}
		}
		const uint64_t tDelta = core_osTimeDiff(core_osTimeNow(), tStart);

		const double dt_ms = core_osTimeConvertTo(tDelta, CORE_TIME_UNITS_MS);
		movAvgPush(&frameTimeAvg, dt_ms);

		{
			stats_t s;
			movAvgGetStats(&frameTimeAvg, &s);

			char str[256];
			core_snprintf(str, CORE_COUNTOF(str), "Frame Time: %.2fms (min: %.2f, 25th: %.2f, med: %.2f, 75th: %.2f, max: %.2f, avg: %.2f, stddev: %.4f)", dt_ms
				, s.m_Min
				, s.m_Percent25
				, s.m_Median
				, s.m_Percent75
				, s.m_Max
				, s.m_Average
				, s.m_StdDev
			);

			swr->drawText(swrCtx, &font, 8, 8, str, NULL, SWR_COLOR_WHITE);
		}

		const int32_t winState = mfb_update_ex(window, swrCtx->m_FrameBuffer, swrCtx->m_Width, swrCtx->m_Height);
		if (winState < 0) {
			window = NULL;
			break;
		}
	} while (mfb_wait_sync(window));

	swr->destroyContext(allocator, swrCtx);

	core_allocatorDestroyAllocator(allocator);
	coreShutdown();

	return 0;
}

static bool meshBuild6502(mesh_t* m, drawcall_t** drawCalls, uint32_t* numDrawCalls, core_allocator_i* allocator)
{
	typedef struct seg_vertex_buffer_t
	{
		const seg_vertex_t* m_Vertices;
		uint32_t m_NumVertices;
	} seg_vertex_buffer_t;

	const seg_vertex_buffer_t segVertexBuffers[] = {
		{ .m_Vertices = seg_vertices_0, .m_NumVertices = CORE_COUNTOF(seg_vertices_0) },
		{ .m_Vertices = seg_vertices_1, .m_NumVertices = CORE_COUNTOF(seg_vertices_1) },
		{ .m_Vertices = seg_vertices_2, .m_NumVertices = CORE_COUNTOF(seg_vertices_2) },
		{ .m_Vertices = seg_vertices_3, .m_NumVertices = CORE_COUNTOF(seg_vertices_3) },
		{ .m_Vertices = seg_vertices_4, .m_NumVertices = CORE_COUNTOF(seg_vertices_4) },
		{ .m_Vertices = seg_vertices_5, .m_NumVertices = CORE_COUNTOF(seg_vertices_5) },
	};

	const uint32_t palette[] = {
		SWR_COLOR(0xf5, 0x00, 0x57, 0xFF),
		SWR_COLOR(0xff, 0xeb, 0x3b, 0xFF),
		SWR_COLOR(0xff, 0x52, 0x52, 0xFF),
		SWR_COLOR(0x7e, 0x57, 0xc2, 0xB2),
		SWR_COLOR(0xfb, 0x8c, 0x00, 0xB2),
		SWR_COLOR(0x00, 0xb0, 0xff, 0xFF),
	};

	uint32_t* vertices = NULL; // { (uint16_t)x, (uint16_t)y }
	uint32_t* colors = NULL;
	uint32_t numVertices = 0;
	uint32_t vertexCapacity = 0;
	uint16_t* indexBuffer = NULL;
	uint32_t numIndices = 0;
	uint32_t indexCapacity = 0;

	const uint32_t numMeshes = CORE_COUNTOF(segVertexBuffers);
	drawcall_t* dc = (drawcall_t*)CORE_ALLOC(allocator, sizeof(drawcall_t) * numMeshes);
	if (!numMeshes) {
		return false;
	}

	for (uint32_t iMesh = 0; iMesh < numMeshes; ++iMesh) {
		const seg_vertex_t* segVertices = segVertexBuffers[iMesh].m_Vertices;
		const uint32_t segNumVertices = segVertexBuffers[iMesh].m_NumVertices;

		const uint32_t baseVertexID = numVertices;
		const uint32_t baseIndexID = numIndices;

		const uint32_t meshColor = palette[iMesh];

		for (uint32_t iVertex = 0; iVertex < segNumVertices; ++iVertex) {
			const seg_vertex_t* segVertex = &segVertices[iVertex];
			const uint32_t segVertexCoord = ((uint32_t)segVertex->x) | ((uint32_t)segVertex->y << 16);
			const uint16_t segVertexNodeID = segVertex->nodeID;

			// Check if the vertex is already in the list
			// NOTE: Check only the vertices of the current mesh (starting from the baseVertex).
			uint32_t vertexID = UINT32_MAX;
			for (uint32_t jVertex = baseVertexID; jVertex < numVertices; ++jVertex) {
				if (vertices[jVertex] == segVertexCoord) {
					vertexID = jVertex;
					break;
				}
			}

			if (vertexID == UINT32_MAX) {
				// Vertex not in buffer. Add it.
				if (numVertices == vertexCapacity) {
					vertexCapacity = vertexCapacity != 0
						? (vertexCapacity * 3) / 2
						: 256
						;

					vertices = (uint32_t*)CORE_REALLOC(allocator, vertices, sizeof(uint32_t) * vertexCapacity);
					if (!vertices) {
						return false;
					}

					colors = (uint32_t*)CORE_REALLOC(allocator, colors, sizeof(uint32_t) * vertexCapacity);
					if (!colors) {
						return false;
					}
				}

				vertexID = numVertices++;
				vertices[vertexID] = segVertexCoord;
				colors[vertexID] = meshColor;
			}

			// At this point vertex ID should be valid.
			if ((vertexID - baseVertexID) > 65535) {
				// TODO: Index cannot be larger than uint16_t
				int a = 0;
			}

			// Add index to index buffer
			{
				if (numIndices == indexCapacity) {
					indexCapacity = indexCapacity != 0
						? (indexCapacity * 3) / 2
						: 256
						;

					indexBuffer = (uint16_t*)CORE_REALLOC(allocator, indexBuffer, sizeof(uint16_t) * indexCapacity);
					if (!indexBuffer) {
						return false;
					}
				}

				indexBuffer[numIndices++] = (uint16_t)(vertexID - baseVertexID);
			}
		}

		dc[iMesh] = (drawcall_t){
			.m_BaseVertex = baseVertexID,
			.m_BaseIndex = baseIndexID,
			.m_NumIndices = numIndices - baseIndexID
		};
	}

	// Convert vertices into pos buffer
	{
		m->m_PosBuffer = (float*)CORE_ALLOC(allocator, sizeof(float) * 2 * numVertices);
		if (!m->m_PosBuffer) {
			return false;
		}

		for (uint32_t i = 0; i < numVertices; ++i) {
			const uint32_t segVertexCoord = vertices[i];
			const uint16_t x = (uint16_t)((segVertexCoord & 0x0000FFFF) >> 0);
			const uint16_t y = (uint16_t)((segVertexCoord & 0xFFFF0000) >> 16);
			m->m_PosBuffer[i * 2 + 0] = (float)x;
			m->m_PosBuffer[i * 2 + 1] = (float)y;
		}
	}

	m->m_ColorBuffer = colors;
	m->m_IndexBuffer = indexBuffer;
	m->m_NumIndices = numIndices;
	m->m_NumVertices = numVertices;

	*drawCalls = dc;
	*numDrawCalls = numMeshes;

	// Cleanup
	CORE_FREE(allocator, vertices);

	return true;
}

static void movAvgPush(movavgd_t* avg, double val)
{
	const uint32_t id = avg->m_NextItemID;
	avg->m_NumItems++;
	if (avg->m_NumItems >= MOVING_AVG_NUM_SAMPLES) {
		avg->m_NumItems = MOVING_AVG_NUM_SAMPLES;
	}
	avg->m_NextItemID = (avg->m_NextItemID + 1) % avg->m_NumItems;
	avg->m_Value[id] = val;
}

static double movAvgGetAverage(const movavgd_t* avg)
{
	const uint32_t n = avg->m_NumItems;
	double sum = 0.0;
	for (uint32_t i = 0; i < n; ++i) {
		sum += avg->m_Value[i];
	}
	return sum / (double)n;
}

static void movAvgGetMinMax(const movavgd_t* avg, double* tmin, double* tmax)
{
	const uint32_t n = avg->m_NumItems;
	double minT = avg->m_Value[0];
	double maxT = avg->m_Value[0];
	for (uint32_t i = 1; i < n; ++i) {
		minT = avg->m_Value[i] < minT ? avg->m_Value[i] : minT;
		maxT = avg->m_Value[i] > maxT ? avg->m_Value[i] : maxT;
	}
	*tmin = minT;
	*tmax = maxT;
}

static int32_t compareDoubleAsc(const void* elem1, const void* elem2)
{
	const double v1 = *(const double*)elem1;
	const double v2 = *(const double*)elem2;
	if (v1 > v2) {
		return 1;
	} else if (v1 < v2) {
		return -1;
	}
	return 0;
}

static void movAvgGetStats(movavgd_t* avg, stats_t* s)
{
	const uint32_t n = avg->m_NumItems;

	memcpy(avg->m_StatsBuffer, avg->m_Value, sizeof(double) * n);
	qsort(avg->m_StatsBuffer, avg->m_NumItems, sizeof(double), compareDoubleAsc);
	s->m_Min = avg->m_StatsBuffer[0];
	s->m_Percent25 = avg->m_StatsBuffer[n >> 2];
	s->m_Median = avg->m_StatsBuffer[n >> 1];
	s->m_Percent75 = avg->m_StatsBuffer[(n >> 2) * 3];
	s->m_Max = avg->m_StatsBuffer[n - 1];
	s->m_Average = 0.0;
	for (uint32_t i = 0; i < n; ++i) {
		s->m_Average += avg->m_StatsBuffer[i];
	}
	s->m_Average /= (double)n;

	s->m_StdDev = 0.0;
	for (uint32_t i = 0; i < n; ++i) {
		const double d = avg->m_StatsBuffer[i] - s->m_Average;
		s->m_StdDev += d * d;
	}
	s->m_StdDev = sqrt(s->m_StdDev / (double)n);
}

#if 0
static uint8_t* loadTextFile(core_file_base_dir baseDir, const char* path, uint64_t* fileSize, core_allocator_i* allocator)
{
	core_os_file* file = core_osFileOpenRead(baseDir, path);
	if (!file) {
		return NULL;
	}

	core_osFileSeek(file, 0, CORE_FILE_SEEK_ORIGIN_END);
	const uint64_t sz = core_osFileTell(file);
	core_osFileSeek(file, 0, CORE_FILE_SEEK_ORIGIN_BEGIN);

	uint8_t* buffer = (uint8_t*)CORE_ALLOC(allocator, sz + 1);
	if (!buffer) {
		core_osFileClose(file);
		return NULL;
	}

	// TODO: Assumes sz is less the 4GB
	core_osFileRead(file, buffer, (uint32_t)sz);
	buffer[sz] = '\0';

	core_osFileClose(file);

	if (fileSize) {
		*fileSize = sz;
	}

	return buffer;
}

// TODO: Stop when a degenerate triangle is encountered.
static uint16_t* meshParseRenderDocIndexBuffer(const char* csv, uint64_t csvSize, uint32_t* numIndices, core_allocator_i* allocator)
{
	// Skip first line
	const char* ptr = core_strchr((char*)csv, '\n');
	if (*ptr != '\n') {
		return NULL;
	}
	++ptr;

	// Parse indices
	uint16_t* ib = NULL;
	uint32_t ibSize = 0;
	uint32_t ibCapacity = 0;

	while (*ptr != '\0') {
		// Skip element index
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse index
		const int32_t id = core_strToInt(ptr, &ptr, 0);
		if (id < 0 || id > 65535) {
			// Invalid index...
			CORE_FREE(allocator, ib);
			return NULL;
		}

		ptr = core_strchr((char*)ptr, '\n');
		if (*ptr == '\n') {
			++ptr;
		}

		// Add index to buffer
		if (ibSize == ibCapacity) {
			ibCapacity = (ibCapacity == 0)
				? 1024
				: (ibCapacity * 3) / 2
				;
			ib = CORE_REALLOC(allocator, ib, sizeof(uint16_t) * ibCapacity);
			if (!ib) {
				return NULL;
			}
		}

		ib[ibSize++] = id;
	}

	*numIndices = ibSize;
	return ib;
}

static bool meshParseRenderDocPosBuffer(const char* csv, uint64_t csvSize, float* pos, uint32_t numVertices)
{
	// Skip first line
	const char* ptr = core_strchr((char*)csv, '\n');
	if (*ptr != '\n') {
		return false;
	}
	++ptr;

	uint32_t n = 0;
	while (*ptr != '\0' && n < numVertices) {
		// Skip element index
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse position.x
		const float x = core_strToFloat(ptr, &ptr);

		// Skip comma
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse position.y
		const float y = core_strToFloat(ptr, &ptr);

		// Skip to next line
		ptr = core_strchr((char*)ptr, '\n');
		if (*ptr == '\n') {
			++ptr;
		}

		pos[n * 2 + 0] = x;
		pos[n * 2 + 1] = y;
		++n;
	}

	return n == numVertices;
}

static bool meshParseRenderDocColorBuffer(const char* csv, uint64_t csvSize, uint32_t* color, uint32_t numVertices)
{
	// Skip first line
	const char* ptr = core_strchr((char*)csv, '\n');
	if (*ptr != '\n') {
		return false;
	}
	++ptr;

	uint32_t n = 0;
	while (*ptr != '\0' && n < numVertices) {
		// Skip element index
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse color.r
		const float r = core_strToFloat(ptr, &ptr);

		// Skip comma
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse color.g
		const float g = core_strToFloat(ptr, &ptr);

		// Skip comma
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse color.b
		const float b = core_strToFloat(ptr, &ptr);

		// Skip comma
		ptr = core_strchr((char*)ptr, ',');
		if (*ptr == ',') {
			++ptr;
		}

		// Parse color.a
		const float a = core_strToFloat(ptr, &ptr);

		// Skip to next line
		ptr = core_strchr((char*)ptr, '\n');
		if (*ptr == '\n') {
			++ptr;
		}

		const uint8_t ur = (uint8_t)(r * 255.0f);
		const uint8_t ug = (uint8_t)(g * 255.0f);
		const uint8_t ub = (uint8_t)(b * 255.0f);
		const uint8_t ua = (uint8_t)(a * 255.0f);

		color[n] = SWR_COLOR(ur, ug, ub, ua);
		++n;
	}

	return n == numVertices;
}

static bool meshLoadRenderDocBuffers(mesh_t* m, core_file_base_dir baseDir, const char* ibPath, const char* vbPosPath, const char* vbColorPath, core_allocator_i* allocator)
{
	// Load index buffer
	{
		uint64_t ibFileBufferSize = 0ull;
		uint8_t* ibFileBuffer = loadTextFile(baseDir, ibPath, &ibFileBufferSize, allocator);
		if (!ibFileBuffer) {
			return false;
		}

		m->m_IndexBuffer = meshParseRenderDocIndexBuffer(ibFileBuffer, ibFileBufferSize, &m->m_NumIndices, allocator);
		CORE_FREE(allocator, ibFileBuffer);
		if (!m->m_IndexBuffer) {
			return false;
		}
	}

#if 1
	const uint32_t maxIndex = 65535;
#else
	// NOTE: This is wrong because each draw call has a base vertex (all indices
	// are offset to that vertex)
	// Find the maximum index and allocate vertex buffers up to that number
	const uint32_t numIndices = m->m_NumIndices;
	uint16_t maxIndex = 0;
	for (uint32_t i = 0; i < numIndices; ++i) {
		if (maxIndex < m->m_IndexBuffer[i]) {
			maxIndex = m->m_IndexBuffer[i];
		}
	}
#endif

	m->m_PosBuffer = (float*)CORE_ALLOC(allocator, sizeof(float) * 2 * (maxIndex + 1));
	if (!m->m_PosBuffer) {
		return false;
	}

	m->m_ColorBuffer = (uint32_t*)CORE_ALLOC(allocator, sizeof(uint32_t) * (maxIndex + 1));
	if (!m->m_ColorBuffer) {
		return false;
	}

	m->m_NumVertices = maxIndex + 1;

	// Load vertex positions
	{
		uint64_t posFileBufferSize = 0ull;
		uint8_t* posFileBuffer = loadTextFile(baseDir, vbPosPath, &posFileBufferSize, allocator);
		if (!posFileBuffer) {
			return false;
		}

		if (!meshParseRenderDocPosBuffer(posFileBuffer, posFileBufferSize, m->m_PosBuffer, m->m_NumVertices)) {
			return false;
		}

		CORE_FREE(allocator, posFileBuffer);
	}

	// Load vertex colors
	{
		uint64_t colorFileBufferSize = 0ull;
		uint8_t* colorFileBuffer = loadTextFile(baseDir, vbColorPath, &colorFileBufferSize, allocator);
		if (!colorFileBuffer) {
			return false;
		}

		if (!meshParseRenderDocColorBuffer(colorFileBuffer, colorFileBufferSize, m->m_ColorBuffer, m->m_NumVertices)) {
			return false;
		}

		CORE_FREE(allocator, colorFileBuffer);
	}

	return true;
}
#endif
