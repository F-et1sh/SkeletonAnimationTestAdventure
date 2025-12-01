/** \file mikktspace/mikktspace.c
 *  \ingroup mikktspace
 */
/**
 *  Copyright (C) 2011 by Morten S. Mikkelsen
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty.  In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *  3. This notice may not be removed or altered from any source distribution.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <stdlib.h>

#include "mikktspace.h"

enum {
    TFALSE = 0,
    TTRUE  = 1
};

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

enum {
    INTERNAL_RND_SORT_SEED = 39871946
};

// internal structure
typedef struct {
    float x, y, z;
} SVec3;

static tbool veq(const SVec3 v1, const SVec3 v2) {
    return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
}

static SVec3 vadd(const SVec3 v1, const SVec3 v2) {
    SVec3 v_res;

    v_res.x = v1.x + v2.x;
    v_res.y = v1.y + v2.y;
    v_res.z = v1.z + v2.z;

    return v_res;
}

static SVec3 vsub(const SVec3 v1, const SVec3 v2) {
    SVec3 v_res;

    v_res.x = v1.x - v2.x;
    v_res.y = v1.y - v2.y;
    v_res.z = v1.z - v2.z;

    return v_res;
}

static SVec3 vscale(const float f_s, const SVec3 v) {
    SVec3 v_res;

    v_res.x = f_s * v.x;
    v_res.y = f_s * v.y;
    v_res.z = f_s * v.z;

    return v_res;
}

static float LengthSquared(const SVec3 v) {
    return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

static float Length(const SVec3 v) {
    return sqrtf(LengthSquared(v));
}

static SVec3 Normalize(const SVec3 v) {
    return vscale(1 / Length(v), v);
}

static float vdot(const SVec3 v1, const SVec3 v2) {
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

static tbool NotZero(const float f_x) {
    // could possibly use FLT_EPSILON instead
    return fabsf(f_x) > FLT_MIN;
}

static tbool VNotZero(const SVec3 v) {
    // might change this to an epsilon based test
    return NotZero(v.x) || NotZero(v.y) || NotZero(v.z);
}

typedef struct {
    int  iNrFaces;
    int* pTriMembers;
} SSubGroup;

typedef struct {
    int   iNrFaces;
    int*  pFaceIndices;
    int   iVertexRepresentitive;
    tbool bOrientPreservering;
} SGroup;

//
enum {
    MARK_DEGENERATE    = 1,
    QUAD_ONE_DEGEN_TRI = 2,
    GROUP_WITH_ANY     = 4,
    ORIENT_PRESERVING  = 8
};

typedef struct {
    int     FaceNeighbors[3];
    SGroup* AssignedGroup[3];

    // normalized first order face derivatives
    SVec3 vOs, vOt;
    float fMagS, fMagT; // original magnitudes

    // determines if the current and the next triangle are a quad.
    int           iOrgFaceNumber;
    int           iFlag, iTSpacesOffs;
    unsigned char vert_num[4];
} STriInfo;

typedef struct {
    SVec3 vOs;
    float fMagS;
    SVec3 vOt;
    float fMagT;
    int   iCounter; // this is to average back into quads.
    tbool bOrient;
} STSpace;

static int   GenerateInitialVerticesIndexList(STriInfo p_tri_infos[], int pi_tri_list_out[], const SMikkTSpaceContext* p_context, int i_nr_triangles_in);
static void  GenerateSharedVerticesIndexList(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* p_context, int i_nr_triangles_in);
static void  InitTriInfo(STriInfo p_tri_infos[], const int pi_tri_list_in[], const SMikkTSpaceContext* p_context, int i_nr_triangles_in);
static int   Build4RuleGroups(STriInfo p_tri_infos[], SGroup p_groups[], int pi_group_triangles_buffer[], const int pi_tri_list_in[], int i_nr_triangles_in);
static tbool GenerateTSpaces(STSpace ps_tspace[], const STriInfo p_tri_infos[], const SGroup p_groups[], int i_nr_active_groups, const int pi_tri_list_in[], float f_thres_cos, const SMikkTSpaceContext* p_context);

static int MakeIndex(const int i_face, const int i_vert) {
    assert(iVert >= 0 && iVert < 4 && iFace >= 0);
    return (i_face << 2) | (i_vert & 0x3);
}

static void IndexToData(int* pi_face, int* pi_vert, const int i_index_in) {
    pi_vert[0] = i_index_in & 0x3;
    pi_face[0] = i_index_in >> 2;
}

static STSpace AvgTSpace(const STSpace* p_t_s0, const STSpace* p_t_s1) {
    STSpace ts_res;

    // this if is important. Due to floating point precision
    // averaging when ts0==ts1 will cause a slight difference
    // which results in tangent space splits later on
    if (p_t_s0->fMagS == p_t_s1->fMagS && p_t_s0->fMagT == p_t_s1->fMagT &&
        veq(p_t_s0->vOs, p_t_s1->vOs) && veq(p_t_s0->vOt, p_t_s1->vOt)) {
        ts_res.fMagS = p_t_s0->fMagS;
        ts_res.fMagT = p_t_s0->fMagT;
        ts_res.vOs   = p_t_s0->vOs;
        ts_res.vOt   = p_t_s0->vOt;
    }
    else {
        ts_res.fMagS = 0.5F * (p_t_s0->fMagS + p_t_s1->fMagS);
        ts_res.fMagT = 0.5F * (p_t_s0->fMagT + p_t_s1->fMagT);
        ts_res.vOs   = vadd(p_t_s0->vOs, p_t_s1->vOs);
        ts_res.vOt   = vadd(p_t_s0->vOt, p_t_s1->vOt);
        if (VNotZero(ts_res.vOs)) {
            ts_res.vOs = Normalize(ts_res.vOs);
        }
        if (VNotZero(ts_res.vOt)) {
            ts_res.vOt = Normalize(ts_res.vOt);
        }
    }

    return ts_res;
}

static SVec3 GetPosition(const SMikkTSpaceContext* p_context, int index);
static SVec3 GetNormal(const SMikkTSpaceContext* p_context, int index);
static SVec3 GetTexCoord(const SMikkTSpaceContext* p_context, int index);

// degen triangles
static void DegenPrologue(STriInfo p_tri_infos[], int pi_tri_list_out[], int i_nr_triangles_in, int i_tot_tris);
static void DegenEpilogue(STSpace ps_tspace[], STriInfo p_tri_infos[], const int pi_tri_list_in[], const SMikkTSpaceContext* p_context, int i_nr_triangles_in, int i_tot_tris);

tbool genTangSpaceDefault(const SMikkTSpaceContext* p_context) {
    return genTangSpace(p_context, 180.0F);
}

tbool genTangSpace(const SMikkTSpaceContext* p_context, const float f_angular_threshold) {
    // count nr_triangles
    int*        piTriListIn            = NULL;
    int*        piGroupTrianglesBuffer = NULL;
    STriInfo*   p_tri_infos            = NULL;
    SGroup*     p_groups               = NULL;
    STSpace*    ps_tspace              = NULL;
    int         iNrTrianglesIn         = 0;
    int         f                      = 0;
    int         t                      = 0;
    int         i                      = 0;
    int         iNrTSPaces             = 0;
    int         iTotTris               = 0;
    int         iDegenTriangles        = 0;
    int         iNrMaxGroups           = 0;
    int         iNrActiveGroups        = 0;
    int         index                  = 0;
    const int   i_nr_faces             = p_context->m_pInterface->m_getNumFaces(p_context);
    tbool       b_res                  = TFALSE;
    const float f_thres_cos            = (float) cosf((f_angular_threshold * (float) M_PI) / 180.0F);

    // verify all call-backs have been set
    if (p_context->m_pInterface->m_getNumFaces == NULL ||
        p_context->m_pInterface->m_getNumVerticesOfFace == NULL ||
        p_context->m_pInterface->m_getPosition == NULL ||
        p_context->m_pInterface->m_getNormal == NULL ||
        p_context->m_pInterface->m_getTexCoord == NULL) {
        return TFALSE;
    }

    // count triangles on supported faces
    for (f = 0; f < i_nr_faces; f++) {
        const int verts = p_context->m_pInterface->m_getNumVerticesOfFace(p_context, f);
        if (verts == 3) {
            ++iNrTrianglesIn;
        }
        else if (verts == 4) {
            iNrTrianglesIn += 2;
        }
    }
    if (iNrTrianglesIn <= 0) {
        return TFALSE;
    }

    // allocate memory for an index list
    piTriListIn = (int*) malloc(sizeof(int) * 3 * iNrTrianglesIn);
    p_tri_infos = (STriInfo*) malloc(sizeof(STriInfo) * iNrTrianglesIn);
    if (piTriListIn == NULL || p_tri_infos == NULL) {
        if (piTriListIn != NULL) {
            free(piTriListIn);
        }
        if (p_tri_infos != NULL) {
            free(p_tri_infos);
        }
        return TFALSE;
    }

    // make an initial triangle --> face index list
    iNrTSPaces = GenerateInitialVerticesIndexList(p_tri_infos, piTriListIn, p_context, iNrTrianglesIn);

    // make a welded index list of identical positions and attributes (pos, norm, texc)
    //printf("gen welded index list begin\n");
    GenerateSharedVerticesIndexList(piTriListIn, p_context, iNrTrianglesIn);
    //printf("gen welded index list end\n");

    // Mark all degenerate triangles
    iTotTris        = iNrTrianglesIn;
    iDegenTriangles = 0;
    for (t = 0; t < iTotTris; t++) {
        const int   i0 = piTriListIn[(t * 3) + 0];
        const int   i1 = piTriListIn[(t * 3) + 1];
        const int   i2 = piTriListIn[(t * 3) + 2];
        const SVec3 p0 = GetPosition(p_context, i0);
        const SVec3 p1 = GetPosition(p_context, i1);
        const SVec3 p2 = GetPosition(p_context, i2);
        if (veq(p0, p1) || veq(p0, p2) || veq(p1, p2)) // degenerate
        {
            p_tri_infos[t].iFlag |= MARK_DEGENERATE;
            ++iDegenTriangles;
        }
    }
    iNrTrianglesIn = iTotTris - iDegenTriangles;

    // mark all triangle pairs that belong to a quad with only one
    // good triangle. These need special treatment in DegenEpilogue().
    // Additionally, move all good triangles to the start of
    // pTriInfos[] and piTriListIn[] without changing order and
    // put the degenerate triangles last.
    DegenPrologue(p_tri_infos, piTriListIn, iNrTrianglesIn, iTotTris);

    // evaluate triangle level attributes and neighbor list
    //printf("gen neighbors list begin\n");
    InitTriInfo(p_tri_infos, piTriListIn, p_context, iNrTrianglesIn);
    //printf("gen neighbors list end\n");

    // based on the 4 rules, identify groups based on connectivity
    iNrMaxGroups           = iNrTrianglesIn * 3;
    p_groups               = (SGroup*) malloc(sizeof(SGroup) * iNrMaxGroups);
    piGroupTrianglesBuffer = (int*) malloc(sizeof(int) * iNrTrianglesIn * 3);
    if (p_groups == NULL || piGroupTrianglesBuffer == NULL) {
        if (p_groups != NULL) {
            free(p_groups);
        }
        if (piGroupTrianglesBuffer != NULL) {
            free(piGroupTrianglesBuffer);
        }
        free(piTriListIn);
        free(p_tri_infos);
        return TFALSE;
    }
    //printf("gen 4rule groups begin\n");
    iNrActiveGroups =
        Build4RuleGroups(p_tri_infos, p_groups, piGroupTrianglesBuffer, piTriListIn, iNrTrianglesIn);
    //printf("gen 4rule groups end\n");

    //

    ps_tspace = (STSpace*) malloc(sizeof(STSpace) * iNrTSPaces);
    if (ps_tspace == NULL) {
        free(piTriListIn);
        free(p_tri_infos);
        free(p_groups);
        free(piGroupTrianglesBuffer);
        return TFALSE;
    }
    memset(ps_tspace, 0, sizeof(STSpace) * iNrTSPaces);
    for (t = 0; t < iNrTSPaces; t++) {
        ps_tspace[t].vOs.x = 1.0F;
        ps_tspace[t].vOs.y = 0.0F;
        ps_tspace[t].vOs.z = 0.0F;
        ps_tspace[t].fMagS = 1.0F;
        ps_tspace[t].vOt.x = 0.0F;
        ps_tspace[t].vOt.y = 1.0F;
        ps_tspace[t].vOt.z = 0.0F;
        ps_tspace[t].fMagT = 1.0F;
    }

    // make tspaces, each group is split up into subgroups if necessary
    // based on fAngularThreshold. Finally a tangent space is made for
    // every resulting subgroup
    //printf("gen tspaces begin\n");
    b_res = GenerateTSpaces(ps_tspace, p_tri_infos, p_groups, iNrActiveGroups, piTriListIn, f_thres_cos, p_context);
    //printf("gen tspaces end\n");

    // clean up
    free(p_groups);
    free(piGroupTrianglesBuffer);

    if (!b_res) // if an allocation in GenerateTSpaces() failed
    {
        // clean up and return false
        free(p_tri_infos);
        free(piTriListIn);
        free(ps_tspace);
        return TFALSE;
    }

    // degenerate quads with one good triangle will be fixed by copying a space from
    // the good triangle to the coinciding vertex.
    // all other degenerate triangles will just copy a space from any good triangle
    // with the same welded index in piTriListIn[].
    DegenEpilogue(ps_tspace, p_tri_infos, piTriListIn, p_context, iNrTrianglesIn, iTotTris);

    free(p_tri_infos);
    free(piTriListIn);

    index = 0;
    for (f = 0; f < i_nr_faces; f++) {
        const int verts = p_context->m_pInterface->m_getNumVerticesOfFace(p_context, f);
        if (verts != 3 && verts != 4) {
            continue;
        }

        // I've decided to let degenerate triangles and group-with-anythings
        // vary between left/right hand coordinate systems at the vertices.
        // All healthy triangles on the other hand are built to always be either or.

        /*// force the coordinate system orientation to be uniform for every face.
		// (this is already the case for good triangles but not for
		// degenerate ones and those with bGroupWithAnything==true)
		bool bOrient = psTspace[index].bOrient;
		if (psTspace[index].iCounter == 0)	// tspace was not derived from a group
		{
			// look for a space created in GenerateTSpaces() by iCounter>0
			bool bNotFound = true;
			int i=1;
			while (i<verts && bNotFound)
			{
				if (psTspace[index+i].iCounter > 0) bNotFound=false;
				else ++i;
			}
			if (!bNotFound) bOrient = psTspace[index+i].bOrient;
		}*/

        // set data
        for (i = 0; i < verts; i++) {
            const STSpace* p_t_space = &ps_tspace[index];
            float          tang[]    = { p_t_space->vOs.x, p_t_space->vOs.y, p_t_space->vOs.z };
            float          bitang[]  = { p_t_space->vOt.x, p_t_space->vOt.y, p_t_space->vOt.z };
            if (p_context->m_pInterface->m_setTSpace != NULL) {
                p_context->m_pInterface->m_setTSpace(p_context, tang, bitang, p_t_space->fMagS, p_t_space->fMagT, p_t_space->bOrient, f, i);
            }
            if (p_context->m_pInterface->m_setTSpaceBasic != NULL) {
                p_context->m_pInterface->m_setTSpaceBasic(p_context, tang, p_t_space->bOrient == TTRUE ? 1.0F : (-1.0F), f, i);
            }

            ++index;
        }
    }

    free(ps_tspace);

    return TTRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    float vert[3];
    int   index;
} STmpVert;

static const int G_I_CELLS = 2048;

#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

// it is IMPORTANT that this function is called to evaluate the hash since
// inlining could potentially reorder instructions and generate different
// results for the same effective input value fVal.
static NOINLINE int FindGridCell(const float f_min, const float f_max, const float f_val) {
    const float f_index = G_I_CELLS * ((f_val - f_min) / (f_max - f_min));
    const int   i_index = (int) f_index;
    return i_index < G_I_CELLS ? (i_index >= 0 ? i_index : 0) : (G_I_CELLS - 1);
}

static void MergeVertsFast(int pi_tri_list_in_and_out[], STmpVert p_tmp_vert[], const SMikkTSpaceContext* p_context, int i_l_in, int i_r_in);
static void MergeVertsSlow(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* p_context, const int p_table[], int i_entries);
static void GenerateSharedVerticesIndexListSlow(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* p_context, int i_nr_triangles_in);

static void GenerateSharedVerticesIndexList(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* pContext, const int i_nr_triangles_in) {

    // Generate bounding box
    int*      piHashTable   = NULL;
    int*      piHashCount   = NULL;
    int*      piHashOffsets = NULL;
    int*      piHashCount2  = NULL;
    STmpVert* p_tmp_vert    = NULL;
    int       i             = 0;
    int       iChannel      = 0;
    int       k             = 0;
    int       e             = 0;
    int       i_max_count   = 0;
    SVec3     vMin          = GetPosition(pContext, 0);
    SVec3     vMax          = vMin;
    SVec3     vDim;
    float     fMin;
    float     fMax;
    for (i = 1; i < (i_nr_triangles_in * 3); i++) {
        const int index = pi_tri_list_in_and_out[i];

        const SVec3 v_p = GetPosition(pContext, index);
        if (vMin.x > v_p.x) {
            vMin.x = v_p.x;
        }
        else if (vMax.x < v_p.x) {
            vMax.x = v_p.x;
        }
        if (vMin.y > v_p.y) {
            vMin.y = v_p.y;
        }
        else if (vMax.y < v_p.y) {
            vMax.y = v_p.y;
        }
        if (vMin.z > v_p.z) {
            vMin.z = v_p.z;
        }
        else if (vMax.z < v_p.z) {
            vMax.z = v_p.z;
        }
    }

    vDim     = vsub(vMax, vMin);
    iChannel = 0;
    fMin     = vMin.x;
    fMax     = vMax.x;
    if (vDim.y > vDim.x && vDim.y > vDim.z) {
        iChannel = 1;
        fMin     = vMin.y;
        fMax     = vMax.y;
    }
    else if (vDim.z > vDim.x) {
        iChannel = 2;
        fMin     = vMin.z;
        fMax     = vMax.z;
    }

    // make allocations
    piHashTable   = (int*) malloc(sizeof(int) * i_nr_triangles_in * 3);
    piHashCount   = (int*) malloc(sizeof(int) * G_I_CELLS);
    piHashOffsets = (int*) malloc(sizeof(int) * G_I_CELLS);
    piHashCount2  = (int*) malloc(sizeof(int) * G_I_CELLS);

    if (piHashTable == NULL || piHashCount == NULL || piHashOffsets == NULL || piHashCount2 == NULL) {
        if (piHashTable != NULL) {
            free(piHashTable);
        }
        if (piHashCount != NULL) {
            free(piHashCount);
        }
        if (piHashOffsets != NULL) {
            free(piHashOffsets);
        }
        if (piHashCount2 != NULL) {
            free(piHashCount2);
        }
        GenerateSharedVerticesIndexListSlow(pi_tri_list_in_and_out, pContext, i_nr_triangles_in);
        return;
    }
    memset(piHashCount, 0, sizeof(int) * G_I_CELLS);
    memset(piHashCount2, 0, sizeof(int) * G_I_CELLS);

    // count amount of elements in each cell unit
    for (i = 0; i < (i_nr_triangles_in * 3); i++) {
        const int   index  = pi_tri_list_in_and_out[i];
        const SVec3 v_p    = GetPosition(pContext, index);
        const float f_val  = iChannel == 0 ? v_p.x : (iChannel == 1 ? v_p.y : v_p.z);
        const int   i_cell = FindGridCell(fMin, fMax, f_val);
        ++piHashCount[i_cell];
    }

    // evaluate start index of each cell.
    piHashOffsets[0] = 0;
    for (k = 1; k < G_I_CELLS; k++) {
        piHashOffsets[k] = piHashOffsets[k - 1] + piHashCount[k - 1];
    }

    // insert vertices
    for (i = 0; i < (i_nr_triangles_in * 3); i++) {
        const int   index   = pi_tri_list_in_and_out[i];
        const SVec3 v_p     = GetPosition(pContext, index);
        const float f_val   = iChannel == 0 ? v_p.x : (iChannel == 1 ? v_p.y : v_p.z);
        const int   i_cell  = FindGridCell(fMin, fMax, f_val);
        int*        p_table = NULL;

        assert(piHashCount2[iCell] < piHashCount[iCell]);
        p_table                       = &piHashTable[piHashOffsets[i_cell]];
        p_table[piHashCount2[i_cell]] = i; // vertex i has been inserted.
        ++piHashCount2[i_cell];
    }
    for (k = 0; k < G_I_CELLS; k++) {
        assert(piHashCount2[k] == piHashCount[k]); // verify the count
    }
    free(piHashCount2);

    // find maximum amount of entries in any hash entry
    i_max_count = piHashCount[0];
    for (k = 1; k < G_I_CELLS; k++) {
        if (i_max_count < piHashCount[k]) {
            i_max_count = piHashCount[k];
        }
    }
    p_tmp_vert = (STmpVert*) malloc(sizeof(STmpVert) * i_max_count);

    // complete the merge
    for (k = 0; k < G_I_CELLS; k++) {
        // extract table of cell k and amount of entries in it
        int*      p_table   = &piHashTable[piHashOffsets[k]];
        const int i_entries = piHashCount[k];
        if (i_entries < 2) {
            continue;
        }

        if (p_tmp_vert != NULL) {
            for (e = 0; e < i_entries; e++) {
                int         i         = p_table[e];
                const SVec3 v_p       = GetPosition(pContext, pi_tri_list_in_and_out[i]);
                p_tmp_vert[e].vert[0] = v_p.x;
                p_tmp_vert[e].vert[1] = v_p.y;
                p_tmp_vert[e].vert[2] = v_p.z;
                p_tmp_vert[e].index   = i;
            }
            MergeVertsFast(pi_tri_list_in_and_out, p_tmp_vert, pContext, 0, i_entries - 1);
        }
        else {
            MergeVertsSlow(pi_tri_list_in_and_out, pContext, p_table, i_entries);
        }
    }

    if (p_tmp_vert != NULL) {
        free(p_tmp_vert);
    }
    free(piHashTable);
    free(piHashCount);
    free(piHashOffsets);
}

static void MergeVertsFast(int pi_tri_list_in_and_out[], STmpVert p_tmp_vert[], const SMikkTSpaceContext* p_context, const int iL_in, const int iR_in) {
    // make bbox
    int   c       = 0;
    int   l       = 0;
    int   channel = 0;
    float fvMin[3];
    float fvMax[3];
    float dx   = 0;
    float dy   = 0;
    float dz   = 0;
    float fSep = 0;
    for (c = 0; c < 3; c++) {
        fvMin[c] = p_tmp_vert[iL_in].vert[c];
        fvMax[c] = fvMin[c];
    }
    for (l = (iL_in + 1); l <= iR_in; l++) {
        for (c = 0; c < 3; c++) {
            if (fvMin[c] > p_tmp_vert[l].vert[c]) {
                fvMin[c] = p_tmp_vert[l].vert[c];
            }
            if (fvMax[c] < p_tmp_vert[l].vert[c]) {
                fvMax[c] = p_tmp_vert[l].vert[c];
            }
        }
    }

    dx = fvMax[0] - fvMin[0];
    dy = fvMax[1] - fvMin[1];
    dz = fvMax[2] - fvMin[2];

    channel = 0;
    if (dy > dx && dy > dz) {
        channel = 1;
    }
    else if (dz > dx) {
        channel = 2;
    }

    fSep = 0.5F * (fvMax[channel] + fvMin[channel]);

    // stop if all vertices are NaNs
    if (!isfinite(fSep)) {
        return;
    }

    // terminate recursion when the separation/average value
    // is no longer strictly between fMin and fMax values.
    if (fSep >= fvMax[channel] || fSep <= fvMin[channel]) {
        // complete the weld
        for (l = iL_in; l <= iR_in; l++) {
            int         i     = p_tmp_vert[l].index;
            const int   index = pi_tri_list_in_and_out[i];
            const SVec3 v_p   = GetPosition(p_context, index);
            const SVec3 v_n   = GetNormal(p_context, index);
            const SVec3 v_t   = GetTexCoord(p_context, index);

            tbool b_not_found = TTRUE;
            int   l2          = iL_in;
            int   i2rec       = -1;
            while (l2 < l && b_not_found) {
                const int   i2     = p_tmp_vert[l2].index;
                const int   index2 = pi_tri_list_in_and_out[i2];
                const SVec3 v_p2   = GetPosition(p_context, index2);
                const SVec3 v_n2   = GetNormal(p_context, index2);
                const SVec3 v_t2   = GetTexCoord(p_context, index2);
                i2rec              = i2;

                //if (vP==vP2 && vN==vN2 && vT==vT2)
                if (v_p.x == v_p2.x && v_p.y == v_p2.y && v_p.z == v_p2.z &&
                    v_n.x == v_n2.x && v_n.y == v_n2.y && v_n.z == v_n2.z &&
                    v_t.x == v_t2.x && v_t.y == v_t2.y && v_t.z == v_t2.z) {
                    b_not_found = TFALSE;
                }
                else {
                    ++l2;
                }
            }

            // merge if previously found
            if (!b_not_found) {
                pi_tri_list_in_and_out[i] = pi_tri_list_in_and_out[i2rec];
            }
        }
    }
    else {
        int iL = iL_in;
        int iR = iR_in;
        assert((iR_in - iL_in) > 0); // at least 2 entries

        // separate (by fSep) all points between iL_in and iR_in in pTmpVert[]
        while (iL < iR) {
            tbool bReadyLeftSwap  = TFALSE;
            tbool bReadyRightSwap = TFALSE;
            while ((!bReadyLeftSwap) && iL < iR) {
                assert(iL >= iL_in && iL <= iR_in);
                bReadyLeftSwap = !(p_tmp_vert[iL].vert[channel] < fSep);
                if (!bReadyLeftSwap) {
                    ++iL;
                }
            }
            while ((!bReadyRightSwap) && iL < iR) {
                assert(iR >= iL_in && iR <= iR_in);
                bReadyRightSwap = p_tmp_vert[iR].vert[channel] < fSep;
                if (!bReadyRightSwap) {
                    --iR;
                }
            }
            assert((iL < iR) || !(bReadyLeftSwap && bReadyRightSwap));

            if (bReadyLeftSwap && bReadyRightSwap) {
                const STmpVert s_tmp = p_tmp_vert[iL];
                assert(iL < iR);
                p_tmp_vert[iL] = p_tmp_vert[iR];
                p_tmp_vert[iR] = s_tmp;
                ++iL;
                --iR;
            }
        }

        assert(iL == (iR + 1) || (iL == iR));
        if (iL == iR) {
            const tbool b_ready_right_swap = p_tmp_vert[iR].vert[channel] < fSep;
            if (b_ready_right_swap) {
                ++iL;
            }
            else {
                --iR;
            }
        }

        // only need to weld when there is more than 1 instance of the (x,y,z)
        if (iL_in < iR) {
            MergeVertsFast(pi_tri_list_in_and_out, p_tmp_vert, p_context, iL_in, iR); // weld all left of fSep
        }
        if (iL < iR_in) {
            MergeVertsFast(pi_tri_list_in_and_out, p_tmp_vert, p_context, iL, iR_in); // weld all right of (or equal to) fSep
        }
    }
}

static void MergeVertsSlow(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* p_context, const int p_table[], const int i_entries) {
    // this can be optimized further using a tree structure or more hashing.
    int e = 0;
    for (e = 0; e < i_entries; e++) {
        int         i     = p_table[e];
        const int   index = pi_tri_list_in_and_out[i];
        const SVec3 v_p   = GetPosition(p_context, index);
        const SVec3 v_n   = GetNormal(p_context, index);
        const SVec3 v_t   = GetTexCoord(p_context, index);

        tbool b_not_found = TTRUE;
        int   e2          = 0;
        int   i2rec       = -1;
        while (e2 < e && b_not_found) {
            const int   i2     = p_table[e2];
            const int   index2 = pi_tri_list_in_and_out[i2];
            const SVec3 v_p2   = GetPosition(p_context, index2);
            const SVec3 v_n2   = GetNormal(p_context, index2);
            const SVec3 v_t2   = GetTexCoord(p_context, index2);
            i2rec              = i2;

            if (veq(v_p, v_p2) && veq(v_n, v_n2) && veq(v_t, v_t2)) {
                b_not_found = TFALSE;
            }
            else {
                ++e2;
            }
        }

        // merge if previously found
        if (!b_not_found) {
            pi_tri_list_in_and_out[i] = pi_tri_list_in_and_out[i2rec];
        }
    }
}

static void GenerateSharedVerticesIndexListSlow(int pi_tri_list_in_and_out[], const SMikkTSpaceContext* p_context, const int i_nr_triangles_in) {
    int iNumUniqueVerts = 0;
    int t               = 0;
    int i               = 0;
    for (t = 0; t < i_nr_triangles_in; t++) {
        for (i = 0; i < 3; i++) {
            const int offs  = (t * 3) + i;
            const int index = pi_tri_list_in_and_out[offs];

            const SVec3 v_p = GetPosition(p_context, index);
            const SVec3 v_n = GetNormal(p_context, index);
            const SVec3 v_t = GetTexCoord(p_context, index);

            tbool b_found   = TFALSE;
            int   t2        = 0;
            int   index2rec = -1;
            while (!b_found && t2 <= t) {
                int j = 0;
                while (!b_found && j < 3) {
                    const int   index2 = pi_tri_list_in_and_out[(t2 * 3) + j];
                    const SVec3 v_p2   = GetPosition(p_context, index2);
                    const SVec3 v_n2   = GetNormal(p_context, index2);
                    const SVec3 v_t2   = GetTexCoord(p_context, index2);

                    if (veq(v_p, v_p2) && veq(v_n, v_n2) && veq(v_t, v_t2)) {
                        b_found = TTRUE;
                    }
                    else {
                        ++j;
                    }
                }
                if (!b_found) {
                    ++t2;
                }
            }

            assert(bFound);
            // if we found our own
            if (index2rec == index) {
                ++iNumUniqueVerts;
            }

            pi_tri_list_in_and_out[offs] = index2rec;
        }
    }
}

static int GenerateInitialVerticesIndexList(STriInfo p_tri_infos[], int pi_tri_list_out[], const SMikkTSpaceContext* p_context, const int i_nr_triangles_in) {
    int iTSpacesOffs    = 0;
    int f               = 0;
    int t               = 0;
    int i_dst_tri_index = 0;
    for (f = 0; f < p_context->m_pInterface->m_getNumFaces(p_context); f++) {
        const int verts = p_context->m_pInterface->m_getNumVerticesOfFace(p_context, f);
        if (verts != 3 && verts != 4) {
            continue;
        }

        p_tri_infos[i_dst_tri_index].iOrgFaceNumber = f;
        p_tri_infos[i_dst_tri_index].iTSpacesOffs   = iTSpacesOffs;

        if (verts == 3) {
            unsigned char* p_verts                     = p_tri_infos[i_dst_tri_index].vert_num;
            p_verts[0]                                 = 0;
            p_verts[1]                                 = 1;
            p_verts[2]                                 = 2;
            pi_tri_list_out[(i_dst_tri_index * 3) + 0] = MakeIndex(f, 0);
            pi_tri_list_out[(i_dst_tri_index * 3) + 1] = MakeIndex(f, 1);
            pi_tri_list_out[(i_dst_tri_index * 3) + 2] = MakeIndex(f, 2);
            ++i_dst_tri_index; // next
        }
        else {
            {
                p_tri_infos[i_dst_tri_index + 1].iOrgFaceNumber = f;
                p_tri_infos[i_dst_tri_index + 1].iTSpacesOffs   = iTSpacesOffs;
            }

            {
                // need an order independent way to evaluate
                // tspace on quads. This is done by splitting
                // along the shortest diagonal.
                const int   i0                = MakeIndex(f, 0);
                const int   i1                = MakeIndex(f, 1);
                const int   i2                = MakeIndex(f, 2);
                const int   i3                = MakeIndex(f, 3);
                const SVec3 t0                = GetTexCoord(p_context, i0);
                const SVec3 t1                = GetTexCoord(p_context, i1);
                const SVec3 t2                = GetTexCoord(p_context, i2);
                const SVec3 t3                = GetTexCoord(p_context, i3);
                const float dist_sq_02        = LengthSquared(vsub(t2, t0));
                const float dist_sq_13        = LengthSquared(vsub(t3, t1));
                tbool       b_quad_diag_is_02 = 0;
                if (dist_sq_02 < dist_sq_13) {
                    b_quad_diag_is_02 = TTRUE;
                }
                else if (dist_sq_13 < dist_sq_02) {
                    b_quad_diag_is_02 = TFALSE;
                }
                else {
                    const SVec3 p0         = GetPosition(p_context, i0);
                    const SVec3 p1         = GetPosition(p_context, i1);
                    const SVec3 p2         = GetPosition(p_context, i2);
                    const SVec3 p3         = GetPosition(p_context, i3);
                    const float dist_sq_02 = LengthSquared(vsub(p2, p0));
                    const float dist_sq_13 = LengthSquared(vsub(p3, p1));

                    b_quad_diag_is_02 = dist_sq_13 < dist_sq_02 ? TFALSE : TTRUE;
                }

                if (b_quad_diag_is_02) {
                    {
                        unsigned char* p_verts_a = p_tri_infos[i_dst_tri_index].vert_num;
                        p_verts_a[0]             = 0;
                        p_verts_a[1]             = 1;
                        p_verts_a[2]             = 2;
                    }
                    pi_tri_list_out[(i_dst_tri_index * 3) + 0] = i0;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 1] = i1;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 2] = i2;
                    ++i_dst_tri_index; // next
                    {
                        unsigned char* p_verts_b = p_tri_infos[i_dst_tri_index].vert_num;
                        p_verts_b[0]             = 0;
                        p_verts_b[1]             = 2;
                        p_verts_b[2]             = 3;
                    }
                    pi_tri_list_out[(i_dst_tri_index * 3) + 0] = i0;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 1] = i2;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 2] = i3;
                    ++i_dst_tri_index; // next
                }
                else {
                    {
                        unsigned char* p_verts_a = p_tri_infos[i_dst_tri_index].vert_num;
                        p_verts_a[0]             = 0;
                        p_verts_a[1]             = 1;
                        p_verts_a[2]             = 3;
                    }
                    pi_tri_list_out[(i_dst_tri_index * 3) + 0] = i0;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 1] = i1;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 2] = i3;
                    ++i_dst_tri_index; // next
                    {
                        unsigned char* p_verts_b = p_tri_infos[i_dst_tri_index].vert_num;
                        p_verts_b[0]             = 1;
                        p_verts_b[1]             = 2;
                        p_verts_b[2]             = 3;
                    }
                    pi_tri_list_out[(i_dst_tri_index * 3) + 0] = i1;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 1] = i2;
                    pi_tri_list_out[(i_dst_tri_index * 3) + 2] = i3;
                    ++i_dst_tri_index; // next
                }
            }
        }

        iTSpacesOffs += verts;
        assert(iDstTriIndex <= iNrTrianglesIn);
    }

    for (t = 0; t < i_nr_triangles_in; t++) {
        p_tri_infos[t].iFlag = 0;
    }

    // return total amount of tspaces
    return iTSpacesOffs;
}

static SVec3 GetPosition(const SMikkTSpaceContext* p_context, const int index) {
    int   iF;
    int   iI;
    SVec3 res;
    float pos[3];
    IndexToData(&iF, &iI, index);
    p_context->m_pInterface->m_getPosition(p_context, pos, iF, iI);
    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];
    return res;
}

static SVec3 GetNormal(const SMikkTSpaceContext* p_context, const int index) {
    int   iF;
    int   iI;
    SVec3 res;
    float norm[3];
    IndexToData(&iF, &iI, index);
    p_context->m_pInterface->m_getNormal(p_context, norm, iF, iI);
    res.x = norm[0];
    res.y = norm[1];
    res.z = norm[2];
    return res;
}

static SVec3 GetTexCoord(const SMikkTSpaceContext* p_context, const int index) {
    int   iF;
    int   iI;
    SVec3 res;
    float texc[2];
    IndexToData(&iF, &iI, index);
    p_context->m_pInterface->m_getTexCoord(p_context, texc, iF, iI);
    res.x = texc[0];
    res.y = texc[1];
    res.z = 1.0F;
    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union {
    struct
    {
        int i0, i1, f;
    };
    int array[3];
} SEdge;

static void BuildNeighborsFast(STriInfo p_tri_infos[], SEdge* p_edges, const int pi_tri_list_in[], int i_nr_triangles_in);
static void BuildNeighborsSlow(STriInfo p_tri_infos[], const int pi_tri_list_in[], int i_nr_triangles_in);

// returns the texture area times 2
static float CalcTexArea(const SMikkTSpaceContext* p_context, const int indices[]) {
    const SVec3 t1 = GetTexCoord(p_context, indices[0]);
    const SVec3 t2 = GetTexCoord(p_context, indices[1]);
    const SVec3 t3 = GetTexCoord(p_context, indices[2]);

    const float t21x = t2.x - t1.x;
    const float t21y = t2.y - t1.y;
    const float t31x = t3.x - t1.x;
    const float t31y = t3.y - t1.y;

    const float f_signed_area_s_tx2 = (t21x * t31y) - (t21y * t31x);

    return f_signed_area_s_tx2 < 0 ? (-f_signed_area_s_tx2) : f_signed_area_s_tx2;
}

static void InitTriInfo(STriInfo p_tri_infos[], const int pi_tri_list_in[], const SMikkTSpaceContext* p_context, const int i_nr_triangles_in) {
    int f = 0;
    int i = 0;
    int t = 0;
    // pTriInfos[f].iFlag is cleared in GenerateInitialVerticesIndexList() which is called before this function.

    // generate neighbor info list
    for (f = 0; f < i_nr_triangles_in; f++) {
        for (i = 0; i < 3; i++) {
            p_tri_infos[f].FaceNeighbors[i] = -1;
            p_tri_infos[f].AssignedGroup[i] = NULL;

            p_tri_infos[f].vOs.x = 0.0F;
            p_tri_infos[f].vOs.y = 0.0F;
            p_tri_infos[f].vOs.z = 0.0F;
            p_tri_infos[f].vOt.x = 0.0F;
            p_tri_infos[f].vOt.y = 0.0F;
            p_tri_infos[f].vOt.z = 0.0F;
            p_tri_infos[f].fMagS = 0;
            p_tri_infos[f].fMagT = 0;

            // assumed bad
            p_tri_infos[f].iFlag |= GROUP_WITH_ANY;
        }
    }

    // evaluate first order derivatives
    for (f = 0; f < i_nr_triangles_in; f++) {
        // initial values
        const SVec3 v1 = GetPosition(p_context, pi_tri_list_in[(f * 3) + 0]);
        const SVec3 v2 = GetPosition(p_context, pi_tri_list_in[(f * 3) + 1]);
        const SVec3 v3 = GetPosition(p_context, pi_tri_list_in[(f * 3) + 2]);
        const SVec3 t1 = GetTexCoord(p_context, pi_tri_list_in[(f * 3) + 0]);
        const SVec3 t2 = GetTexCoord(p_context, pi_tri_list_in[(f * 3) + 1]);
        const SVec3 t3 = GetTexCoord(p_context, pi_tri_list_in[(f * 3) + 2]);

        const float t21x = t2.x - t1.x;
        const float t21y = t2.y - t1.y;
        const float t31x = t3.x - t1.x;
        const float t31y = t3.y - t1.y;
        const SVec3 d1   = vsub(v2, v1);
        const SVec3 d2   = vsub(v3, v1);

        const float f_signed_area_s_tx2 = (t21x * t31y) - (t21y * t31x);
        //assert(fSignedAreaSTx2!=0);
        SVec3 v_os = vsub(vscale(t31y, d1), vscale(t21y, d2));  // eq 18
        SVec3 v_ot = vadd(vscale(-t31x, d1), vscale(t21x, d2)); // eq 19

        p_tri_infos[f].iFlag |= (f_signed_area_s_tx2 > 0 ? ORIENT_PRESERVING : 0);

        if (NotZero(f_signed_area_s_tx2)) {
            const float f_abs_area = fabsf(f_signed_area_s_tx2);
            const float f_len_os   = Length(v_os);
            const float f_len_ot   = Length(v_ot);
            const float f_s        = (p_tri_infos[f].iFlag & ORIENT_PRESERVING) == 0 ? (-1.0F) : 1.0F;
            if (NotZero(f_len_os)) {
                p_tri_infos[f].vOs = vscale(f_s / f_len_os, v_os);
            }
            if (NotZero(f_len_ot)) {
                p_tri_infos[f].vOt = vscale(f_s / f_len_ot, v_ot);
            }

            // evaluate magnitudes prior to normalization of vOs and vOt
            p_tri_infos[f].fMagS = f_len_os / f_abs_area;
            p_tri_infos[f].fMagT = f_len_ot / f_abs_area;

            // if this is a good triangle
            if (NotZero(p_tri_infos[f].fMagS) && NotZero(p_tri_infos[f].fMagT)) {
                p_tri_infos[f].iFlag &= (~GROUP_WITH_ANY);
            }
        }
    }

    // force otherwise healthy quads to a fixed orientation
    while (t < (i_nr_triangles_in - 1)) {
        const int i_fo_a = p_tri_infos[t].iOrgFaceNumber;
        const int i_fo_b = p_tri_infos[t + 1].iOrgFaceNumber;
        if (i_fo_a == i_fo_b) // this is a quad
        {
            const tbool b_is_deg_a = (p_tri_infos[t].iFlag & MARK_DEGENERATE) != 0 ? TTRUE : TFALSE;
            const tbool b_is_deg_b = (p_tri_infos[t + 1].iFlag & MARK_DEGENERATE) != 0 ? TTRUE : TFALSE;

            // bad triangles should already have been removed by
            // DegenPrologue(), but just in case check bIsDeg_a and bIsDeg_a are false
            if ((b_is_deg_a || b_is_deg_b) == TFALSE) {
                const tbool b_orient_a = (p_tri_infos[t].iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
                const tbool b_orient_b = (p_tri_infos[t + 1].iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
                // if this happens the quad has extremely bad mapping!!
                if (b_orient_a != b_orient_b) {
                    //printf("found quad with bad mapping\n");
                    tbool b_choose_orient_first_tri = TFALSE;
                    if ((p_tri_infos[t + 1].iFlag & GROUP_WITH_ANY) != 0) {
                        b_choose_orient_first_tri = TTRUE;
                    }
                    else if (CalcTexArea(p_context, &pi_tri_list_in[(t * 3) + 0]) >= CalcTexArea(p_context, &pi_tri_list_in[((t + 1) * 3) + 0])) {
                        b_choose_orient_first_tri = TTRUE;
                    }

                    // force match
                    {
                        const int t0 = b_choose_orient_first_tri ? t : (t + 1);
                        const int t1 = b_choose_orient_first_tri ? (t + 1) : t;
                        p_tri_infos[t1].iFlag &= (~ORIENT_PRESERVING);                        // clear first
                        p_tri_infos[t1].iFlag |= (p_tri_infos[t0].iFlag & ORIENT_PRESERVING); // copy bit
                    }
                }
            }
            t += 2;
        }
        else {
            ++t;
        }
    }

    // match up edge pairs
    {
        SEdge* p_edges = (SEdge*) malloc(sizeof(SEdge) * i_nr_triangles_in * 3);
        if (p_edges == NULL) {
            BuildNeighborsSlow(p_tri_infos, pi_tri_list_in, i_nr_triangles_in);
        }
        else {
            BuildNeighborsFast(p_tri_infos, p_edges, pi_tri_list_in, i_nr_triangles_in);

            free(p_edges);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static tbool AssignRecur(const int pi_tri_list_in[], STriInfo ps_tri_infos[], int i_my_tri_index, SGroup* p_group);
static void  AddTriToGroup(SGroup* p_group, int i_tri_index);

static int Build4RuleGroups(STriInfo p_tri_infos[], SGroup p_groups[], int pi_group_triangles_buffer[], const int pi_tri_list_in[], const int i_nr_triangles_in) {
    const int i_nr_max_groups    = i_nr_triangles_in * 3;
    int       i_nr_active_groups = 0;
    int       iOffset            = 0;
    int       f                  = 0;
    int       i                  = 0;
    (void) i_nr_max_groups; /* quiet warnings in non debug mode */
    for (f = 0; f < i_nr_triangles_in; f++) {
        for (i = 0; i < 3; i++) {
            // if not assigned to a group
            if ((p_tri_infos[f].iFlag & GROUP_WITH_ANY) == 0 && p_tri_infos[f].AssignedGroup[i] == NULL) {
                tbool     b_or_pre = 0;
                int       neigh_indexL;
                int       neigh_indexR;
                const int vert_index = pi_tri_list_in[(f * 3) + i];
                assert(iNrActiveGroups < iNrMaxGroups);
                p_tri_infos[f].AssignedGroup[i]                        = &p_groups[i_nr_active_groups];
                p_tri_infos[f].AssignedGroup[i]->iVertexRepresentitive = vert_index;
                p_tri_infos[f].AssignedGroup[i]->bOrientPreservering   = (p_tri_infos[f].iFlag & ORIENT_PRESERVING) != 0;
                p_tri_infos[f].AssignedGroup[i]->iNrFaces              = 0;
                p_tri_infos[f].AssignedGroup[i]->pFaceIndices          = &pi_group_triangles_buffer[iOffset];
                ++i_nr_active_groups;

                AddTriToGroup(p_tri_infos[f].AssignedGroup[i], f);
                b_or_pre     = (p_tri_infos[f].iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
                neigh_indexL = p_tri_infos[f].FaceNeighbors[i];
                neigh_indexR = p_tri_infos[f].FaceNeighbors[i > 0 ? (i - 1) : 2];
                if (neigh_indexL >= 0) // neighbor
                {
                    const tbool b_answer =
                        AssignRecur(pi_tri_list_in, p_tri_infos, neigh_indexL, p_tri_infos[f].AssignedGroup[i]);

                    const tbool b_or_pre2 = (p_tri_infos[neigh_indexL].iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
                    const tbool b_diff    = b_or_pre != b_or_pre2 ? TTRUE : TFALSE;
                    assert(bAnswer || bDiff);
                    (void) b_answer, (void) b_diff; /* quiet warnings in non debug mode */
                }
                if (neigh_indexR >= 0) // neighbor
                {
                    const tbool b_answer =
                        AssignRecur(pi_tri_list_in, p_tri_infos, neigh_indexR, p_tri_infos[f].AssignedGroup[i]);

                    const tbool b_or_pre2 = (p_tri_infos[neigh_indexR].iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
                    const tbool b_diff    = b_or_pre != b_or_pre2 ? TTRUE : TFALSE;
                    assert(bAnswer || bDiff);
                    (void) b_answer, (void) b_diff; /* quiet warnings in non debug mode */
                }

                // update offset
                iOffset += p_tri_infos[f].AssignedGroup[i]->iNrFaces;
                // since the groups are disjoint a triangle can never
                // belong to more than 3 groups. Subsequently something
                // is completely screwed if this assertion ever hits.
                assert(iOffset <= iNrMaxGroups);
            }
        }
    }

    return i_nr_active_groups;
}

static void AddTriToGroup(SGroup* p_group, const int i_tri_index) {
    p_group->pFaceIndices[p_group->iNrFaces] = i_tri_index;
    ++p_group->iNrFaces;
}

static tbool AssignRecur(const int pi_tri_list_in[], STriInfo ps_tri_infos[], const int i_my_tri_index, SGroup* p_group) {
    STriInfo* pMyTriInfo = &ps_tri_infos[i_my_tri_index];

    // track down vertex
    const int  i_vert_rep = p_group->iVertexRepresentitive;
    const int* p_verts    = &pi_tri_list_in[(3 * i_my_tri_index) + 0];
    int        i          = -1;
    if (p_verts[0] == i_vert_rep) {
        i = 0;
    }
    else if (p_verts[1] == i_vert_rep) {
        i = 1;
    }
    else if (p_verts[2] == i_vert_rep) {
        i = 2;
    }
    assert(i >= 0 && i < 3);

    // early out
    if (pMyTriInfo->AssignedGroup[i] == p_group) {
        return TTRUE;
    }
    if (pMyTriInfo->AssignedGroup[i] != NULL) return TFALSE;
    if ((pMyTriInfo->iFlag & GROUP_WITH_ANY) != 0) {
        // first to group with a group-with-anything triangle
        // determines it's orientation.
        // This is the only existing order dependency in the code!!
        if (pMyTriInfo->AssignedGroup[0] == NULL &&
            pMyTriInfo->AssignedGroup[1] == NULL &&
            pMyTriInfo->AssignedGroup[2] == NULL) {
            pMyTriInfo->iFlag &= (~ORIENT_PRESERVING);
            pMyTriInfo->iFlag |= (p_group->bOrientPreservering ? ORIENT_PRESERVING : 0);
        }
    }
    {
        const tbool b_orient = (pMyTriInfo->iFlag & ORIENT_PRESERVING) != 0 ? TTRUE : TFALSE;
        if (b_orient != p_group->bOrientPreservering) {
            return TFALSE;
        }
    }

    AddTriToGroup(p_group, i_my_tri_index);
    pMyTriInfo->AssignedGroup[i] = p_group;

    {
        const int neigh_index_l = pMyTriInfo->FaceNeighbors[i];
        const int neigh_index_r = pMyTriInfo->FaceNeighbors[i > 0 ? (i - 1) : 2];
        if (neigh_index_l >= 0) {
            AssignRecur(pi_tri_list_in, ps_tri_infos, neigh_index_l, p_group);
        }
        if (neigh_index_r >= 0) {
            AssignRecur(pi_tri_list_in, ps_tri_infos, neigh_index_r, p_group);
        }
    }

    return TTRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static tbool   CompareSubGroups(const SSubGroup* pg1, const SSubGroup* pg2);
static void    QuickSort(int* p_sort_buffer, int i_left, int i_right, unsigned int u_seed);
static STSpace EvalTspace(const int face_indices[], int i_faces, const int pi_tri_list_in[], const STriInfo p_tri_infos[], const SMikkTSpaceContext* p_context, int i_vertex_representitive);

static tbool GenerateTSpaces(STSpace ps_tspace[], const STriInfo p_tri_infos[], const SGroup p_groups[], const int i_nr_active_groups, const int pi_tri_list_in[], const float f_thres_cos, const SMikkTSpaceContext* p_context) {
    STSpace*   p_sub_group_tspace = NULL;
    SSubGroup* p_uni_sub_groups   = NULL;
    int*       p_tmp_members      = NULL;
    int        iMaxNrFaces        = 0;
    int        iUniqueTspaces     = 0;
    int        g                  = 0;
    int        i                  = 0;
    for (g = 0; g < i_nr_active_groups; g++) {
        if (iMaxNrFaces < p_groups[g].iNrFaces) {
            iMaxNrFaces = p_groups[g].iNrFaces;
        }
    }

    if (iMaxNrFaces == 0) {
        return TTRUE;
    }

    // make initial allocations
    p_sub_group_tspace = (STSpace*) malloc(sizeof(STSpace) * iMaxNrFaces);
    p_uni_sub_groups   = (SSubGroup*) malloc(sizeof(SSubGroup) * iMaxNrFaces);
    p_tmp_members      = (int*) malloc(sizeof(int) * iMaxNrFaces);
    if (p_sub_group_tspace == NULL || p_uni_sub_groups == NULL || p_tmp_members == NULL) {
        if (p_sub_group_tspace != NULL) {
            free(p_sub_group_tspace);
        }
        if (p_uni_sub_groups != NULL) {
            free(p_uni_sub_groups);
        }
        if (p_tmp_members != NULL) {
            free(p_tmp_members);
        }
        return TFALSE;
    }

    iUniqueTspaces = 0;
    for (g = 0; g < i_nr_active_groups; g++) {
        const SGroup* p_group          = &p_groups[g];
        int           iUniqueSubGroups = 0;
        int           s                = 0;

        for (i = 0; i < p_group->iNrFaces; i++) // triangles
        {
            const int f          = p_group->pFaceIndices[i]; // triangle number
            int       index      = -1;
            int       iVertIndex = -1;
            int       iOF_1      = -1;
            int       iMembers   = 0;
            int       j          = 0;
            int       l          = 0;
            SSubGroup tmp_group;
            tbool     b_found = 0;
            SVec3     n;
            SVec3     vOs;
            SVec3     vOt;
            if (p_tri_infos[f].AssignedGroup[0] == p_group) {
                index = 0;
            }
            else if (p_tri_infos[f].AssignedGroup[1] == p_group) {
                index = 1;
            }
            else if (p_tri_infos[f].AssignedGroup[2] == p_group) {
                index = 2;
            }
            assert(index >= 0 && index < 3);

            iVertIndex = pi_tri_list_in[(f * 3) + index];
            assert(iVertIndex == pGroup->iVertexRepresentitive);

            // is normalized already
            n = GetNormal(p_context, iVertIndex);

            // project
            vOs = vsub(p_tri_infos[f].vOs, vscale(vdot(n, p_tri_infos[f].vOs), n));
            vOt = vsub(p_tri_infos[f].vOt, vscale(vdot(n, p_tri_infos[f].vOt), n));
            if (VNotZero(vOs)) {
                vOs = Normalize(vOs);
            }
            if (VNotZero(vOt)) {
                vOt = Normalize(vOt);
            }

            // original face number
            iOF_1 = p_tri_infos[f].iOrgFaceNumber;

            iMembers = 0;
            for (j = 0; j < p_group->iNrFaces; j++) {
                const int t      = p_group->pFaceIndices[j]; // triangle number
                const int i_of_2 = p_tri_infos[t].iOrgFaceNumber;

                // project
                SVec3 v_os2 = vsub(p_tri_infos[t].vOs, vscale(vdot(n, p_tri_infos[t].vOs), n));
                SVec3 v_ot2 = vsub(p_tri_infos[t].vOt, vscale(vdot(n, p_tri_infos[t].vOt), n));
                if (VNotZero(v_os2)) {
                    v_os2 = Normalize(v_os2);
                }
                if (VNotZero(v_ot2)) {
                    v_ot2 = Normalize(v_ot2);
                }

                {
                    const tbool b_any = ((p_tri_infos[f].iFlag | p_tri_infos[t].iFlag) & GROUP_WITH_ANY) != 0 ? TTRUE : TFALSE;
                    // make sure triangles which belong to the same quad are joined.
                    const tbool b_same_org_face = iOF_1 == i_of_2 ? TTRUE : TFALSE;

                    const float f_cos_s = vdot(vOs, v_os2);
                    const float f_cos_t = vdot(vOt, v_ot2);

                    assert(f != t || bSameOrgFace); // sanity check
                    if (b_any || b_same_org_face || (f_cos_s > f_thres_cos && f_cos_t > f_thres_cos)) {
                        p_tmp_members[iMembers++] = t;
                    }
                }
            }

            // sort pTmpMembers
            tmp_group.iNrFaces    = iMembers;
            tmp_group.pTriMembers = p_tmp_members;
            if (iMembers > 1) {
                unsigned int u_seed = INTERNAL_RND_SORT_SEED; // could replace with a random seed?
                QuickSort(p_tmp_members, 0, iMembers - 1, u_seed);
            }

            // look for an existing match
            b_found = TFALSE;
            l       = 0;
            while (l < iUniqueSubGroups && !b_found) {
                b_found = CompareSubGroups(&tmp_group, &p_uni_sub_groups[l]);
                if (!b_found) {
                    ++l;
                }
            }

            // assign tangent space index
            assert(bFound || l == iUniqueSubGroups);
            //piTempTangIndices[f*3+index] = iUniqueTspaces+l;

            // if no match was found we allocate a new subgroup
            if (!b_found) {
                // insert new subgroup
                int* p_indices = (int*) malloc(sizeof(int) * iMembers);
                if (p_indices == NULL) {
                    // clean up and return false
                    int s = 0;
                    for (s = 0; s < iUniqueSubGroups; s++) {
                        free(p_uni_sub_groups[s].pTriMembers);
                    }
                    free(p_uni_sub_groups);
                    free(p_tmp_members);
                    free(p_sub_group_tspace);
                    return TFALSE;
                }
                p_uni_sub_groups[iUniqueSubGroups].iNrFaces    = iMembers;
                p_uni_sub_groups[iUniqueSubGroups].pTriMembers = p_indices;
                memcpy(p_indices, tmp_group.pTriMembers, iMembers * sizeof(int));
                p_sub_group_tspace[iUniqueSubGroups] =
                    EvalTspace(tmp_group.pTriMembers, iMembers, pi_tri_list_in, p_tri_infos, p_context, p_group->iVertexRepresentitive);
                ++iUniqueSubGroups;
            }

            // output tspace
            {
                const int i_offs   = p_tri_infos[f].iTSpacesOffs;
                const int i_vert   = p_tri_infos[f].vert_num[index];
                STSpace*  p_ts_out = &ps_tspace[i_offs + i_vert];
                assert(pTS_out->iCounter < 2);
                assert(((pTriInfos[f].iFlag & ORIENT_PRESERVING) != 0) == pGroup->bOrientPreservering);
                if (p_ts_out->iCounter == 1) {
                    *p_ts_out          = AvgTSpace(p_ts_out, &p_sub_group_tspace[l]);
                    p_ts_out->iCounter = 2; // update counter
                    p_ts_out->bOrient  = p_group->bOrientPreservering;
                }
                else {
                    assert(pTS_out->iCounter == 0);
                    *p_ts_out          = p_sub_group_tspace[l];
                    p_ts_out->iCounter = 1; // update counter
                    p_ts_out->bOrient  = p_group->bOrientPreservering;
                }
            }
        }

        // clean up and offset iUniqueTspaces
        for (s = 0; s < iUniqueSubGroups; s++) {
            free(p_uni_sub_groups[s].pTriMembers);
        }
        iUniqueTspaces += iUniqueSubGroups;
    }

    // clean up
    free(p_uni_sub_groups);
    free(p_tmp_members);
    free(p_sub_group_tspace);

    return TTRUE;
}

static STSpace EvalTspace(const int face_indices[], const int i_faces, const int pi_tri_list_in[], const STriInfo p_tri_infos[], const SMikkTSpaceContext* p_context, const int i_vertex_representitive) {
    STSpace res;
    float   f_angle_sum = 0;
    int     face        = 0;
    res.vOs.x           = 0.0F;
    res.vOs.y           = 0.0F;
    res.vOs.z           = 0.0F;
    res.vOt.x           = 0.0F;
    res.vOt.y           = 0.0F;
    res.vOt.z           = 0.0F;
    res.fMagS           = 0;
    res.fMagT           = 0;

    for (face = 0; face < i_faces; face++) {
        const int f = face_indices[face];

        // only valid triangles get to add their contribution
        if ((p_tri_infos[f].iFlag & GROUP_WITH_ANY) == 0) {
            SVec3 n;
            SVec3 vOs;
            SVec3 vOt;
            SVec3 p0;
            SVec3 p1;
            SVec3 p2;
            SVec3 v1;
            SVec3 v2;
            float fCos;
            float fAngle;
            float fMagS;
            float fMagT;
            int   i     = -1;
            int   index = -1;
            int   i0    = -1;
            int   i1    = -1;
            int   i2    = -1;
            if (pi_tri_list_in[(3 * f) + 0] == i_vertex_representitive) {
                i = 0;
            }
            else if (pi_tri_list_in[(3 * f) + 1] == i_vertex_representitive) {
                i = 1;
            }
            else if (pi_tri_list_in[(3 * f) + 2] == i_vertex_representitive) {
                i = 2;
            }
            assert(i >= 0 && i < 3);

            // project
            index = pi_tri_list_in[(3 * f) + i];
            n     = GetNormal(p_context, index);
            vOs   = vsub(p_tri_infos[f].vOs, vscale(vdot(n, p_tri_infos[f].vOs), n));
            vOt   = vsub(p_tri_infos[f].vOt, vscale(vdot(n, p_tri_infos[f].vOt), n));
            if (VNotZero(vOs)) {
                vOs = Normalize(vOs);
            }
            if (VNotZero(vOt)) {
                vOt = Normalize(vOt);
            }

            i2 = pi_tri_list_in[(3 * f) + (i < 2 ? (i + 1) : 0)];
            i1 = pi_tri_list_in[(3 * f) + i];
            i0 = pi_tri_list_in[(3 * f) + (i > 0 ? (i - 1) : 2)];

            p0 = GetPosition(p_context, i0);
            p1 = GetPosition(p_context, i1);
            p2 = GetPosition(p_context, i2);
            v1 = vsub(p0, p1);
            v2 = vsub(p2, p1);

            // project
            v1 = vsub(v1, vscale(vdot(n, v1), n));
            if (VNotZero(v1)) {
                v1 = Normalize(v1);
            }
            v2 = vsub(v2, vscale(vdot(n, v2), n));
            if (VNotZero(v2)) {
                v2 = Normalize(v2);
            }

            // weight contribution by the angle
            // between the two edge vectors
            fCos   = vdot(v1, v2);
            fCos   = fCos > 1 ? 1 : (fCos < (-1) ? (-1) : fCos);
            fAngle = (float) acosf(fCos);
            fMagS  = p_tri_infos[f].fMagS;
            fMagT  = p_tri_infos[f].fMagT;

            res.vOs = vadd(res.vOs, vscale(fAngle, vOs));
            res.vOt = vadd(res.vOt, vscale(fAngle, vOt));
            res.fMagS += (fAngle * fMagS);
            res.fMagT += (fAngle * fMagT);
            f_angle_sum += fAngle;
        }
    }

    // normalize
    if (VNotZero(res.vOs)) {
        res.vOs = Normalize(res.vOs);
    }
    if (VNotZero(res.vOt)) {
        res.vOt = Normalize(res.vOt);
    }
    if (f_angle_sum > 0) {
        res.fMagS /= f_angle_sum;
        res.fMagT /= f_angle_sum;
    }

    return res;
}

static tbool CompareSubGroups(const SSubGroup* pg1, const SSubGroup* pg2) {
    tbool b_still_same = TTRUE;
    int   i            = 0;
    if (pg1->iNrFaces != pg2->iNrFaces) {
        return TFALSE;
    }
    while (i < pg1->iNrFaces && b_still_same) {
        b_still_same = pg1->pTriMembers[i] == pg2->pTriMembers[i] ? TTRUE : TFALSE;
        if (b_still_same) {
            ++i;
        }
    }
    return b_still_same;
}

static void QuickSort(int* p_sort_buffer, int i_left, int i_right, unsigned int u_seed) {
    int iL;
    int iR;
    int n;
    int index;
    int iMid;
    int iTmp;

    // Random
    unsigned int t = u_seed & 31;
    t              = (u_seed << t) | (u_seed >> (32 - t));
    u_seed         = u_seed + t + 3;
    // Random end

    iL = i_left;
    iR = i_right;
    n  = (iR - iL) + 1;
    assert(n >= 0);
    index = (int) (u_seed % n);

    iMid = p_sort_buffer[index + iL];

    do {
        while (p_sort_buffer[iL] < iMid) {
            ++iL;
        }
        while (p_sort_buffer[iR] > iMid) {
            --iR;
        }

        if (iL <= iR) {
            iTmp              = p_sort_buffer[iL];
            p_sort_buffer[iL] = p_sort_buffer[iR];
            p_sort_buffer[iR] = iTmp;
            ++iL;
            --iR;
        }
    }
    while (iL <= iR);

    if (i_left < iR) {
        QuickSort(p_sort_buffer, i_left, iR, u_seed);
    }
    if (iL < i_right) {
        QuickSort(p_sort_buffer, iL, i_right, u_seed);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static void QuickSortEdges(SEdge* p_sort_buffer, int i_left, int i_right, int channel, unsigned int u_seed);
static void GetEdge(int* i0_out, int* i1_out, int* edgenum_out, const int indices[], int i0_in, int i1_in);

static void BuildNeighborsFast(STriInfo p_tri_infos[], SEdge* p_edges, const int pi_tri_list_in[], const int i_nr_triangles_in) {
    // build array of edges
    unsigned int u_seed         = INTERNAL_RND_SORT_SEED; // could replace with a random seed?
    int          iEntries       = 0;
    int          iCurStartIndex = -1;
    int          f              = 0;
    int          i              = 0;
    for (f = 0; f < i_nr_triangles_in; f++) {
        for (i = 0; i < 3; i++) {
            const int i0            = pi_tri_list_in[(f * 3) + i];
            const int i1            = pi_tri_list_in[(f * 3) + (i < 2 ? (i + 1) : 0)];
            p_edges[(f * 3) + i].i0 = i0 < i1 ? i0 : i1;    // put minimum index in i0
            p_edges[(f * 3) + i].i1 = !(i0 < i1) ? i0 : i1; // put maximum index in i1
            p_edges[(f * 3) + i].f  = f;                    // record face number
        }
    }

    // sort over all edges by i0, this is the pricy one.
    QuickSortEdges(p_edges, 0, (i_nr_triangles_in * 3) - 1, 0, u_seed); // sort channel 0 which is i0

    // sub sort over i1, should be fast.
    // could replace this with a 64 bit int sort over (i0,i1)
    // with i0 as msb in the quicksort call above.
    iEntries       = i_nr_triangles_in * 3;
    iCurStartIndex = 0;
    for (i = 1; i < iEntries; i++) {
        if (p_edges[iCurStartIndex].i0 != p_edges[i].i0) {
            const int i_l = iCurStartIndex;
            const int i_r = i - 1;
            //const int iElems = i-iL;
            iCurStartIndex = i;
            QuickSortEdges(p_edges, i_l, i_r, 1, u_seed); // sort channel 1 which is i1
        }
    }

    // sub sort over f, which should be fast.
    // this step is to remain compliant with BuildNeighborsSlow() when
    // more than 2 triangles use the same edge (such as a butterfly topology).
    iCurStartIndex = 0;
    for (i = 1; i < iEntries; i++) {
        if (p_edges[iCurStartIndex].i0 != p_edges[i].i0 || p_edges[iCurStartIndex].i1 != p_edges[i].i1) {
            const int i_l = iCurStartIndex;
            const int i_r = i - 1;
            //const int iElems = i-iL;
            iCurStartIndex = i;
            QuickSortEdges(p_edges, i_l, i_r, 2, u_seed); // sort channel 2 which is f
        }
    }

    // pair up, adjacent triangles
    for (i = 0; i < iEntries; i++) {
        const int i0             = p_edges[i].i0;
        const int i1             = p_edges[i].i1;
        const int f              = p_edges[i].f;
        tbool     b_unassigned_a = 0;

        int i0_A;
        int i1_A;
        int edgenum_A;
        int edgenum_B = 0;                                                               // 0,1 or 2
        GetEdge(&i0_A, &i1_A, &edgenum_A, &pi_tri_list_in[(ptrdiff_t) (f * 3)], i0, i1); // resolve index ordering and edge_num
        b_unassigned_a = p_tri_infos[f].FaceNeighbors[edgenum_A] == -1 ? TTRUE : TFALSE;

        if (b_unassigned_a) {
            // get true index ordering
            int   j = i + 1;
            int   t;
            tbool b_not_found = TTRUE;
            while (j < iEntries && i0 == p_edges[j].i0 && i1 == p_edges[j].i1 && b_not_found) {
                tbool b_unassigned_b = 0;
                int   i0_B;
                int   i1_B;
                t = p_edges[j].f;
                // flip i0_B and i1_B
                GetEdge(&i1_B, &i0_B, &edgenum_B, &pi_tri_list_in[(ptrdiff_t) (t * 3)], p_edges[j].i0, p_edges[j].i1); // resolve index ordering and edge_num
                //assert(!(i0_A==i1_B && i1_A==i0_B));
                b_unassigned_b = p_tri_infos[t].FaceNeighbors[edgenum_B] == -1 ? TTRUE : TFALSE;
                if (i0_A == i0_B && i1_A == i1_B && b_unassigned_b) {
                    b_not_found = TFALSE;
                }
                else {
                    ++j;
                }
            }

            if (!b_not_found) {
                int t                                   = p_edges[j].f;
                p_tri_infos[f].FaceNeighbors[edgenum_A] = t;
                //assert(pTriInfos[t].FaceNeighbors[edgenum_B]==-1);
                p_tri_infos[t].FaceNeighbors[edgenum_B] = f;
            }
        }
    }
}

static void BuildNeighborsSlow(STriInfo p_tri_infos[], const int pi_tri_list_in[], const int i_nr_triangles_in) {
    int f = 0;
    int i = 0;
    for (f = 0; f < i_nr_triangles_in; f++) {
        for (i = 0; i < 3; i++) {
            // if unassigned
            if (p_tri_infos[f].FaceNeighbors[i] == -1) {
                const int i0_a = pi_tri_list_in[(f * 3) + i];
                const int i1_a = pi_tri_list_in[(f * 3) + (i < 2 ? (i + 1) : 0)];

                // search for a neighbor
                tbool b_found = TFALSE;
                int   t       = 0;
                int   j       = 0;
                while (!b_found && t < i_nr_triangles_in) {
                    if (t != f) {
                        j = 0;
                        while (!b_found && j < 3) {
                            // in rev order
                            const int i1_b = pi_tri_list_in[(t * 3) + j];
                            const int i0_b = pi_tri_list_in[(t * 3) + (j < 2 ? (j + 1) : 0)];
                            //assert(!(i0_A==i1_B && i1_A==i0_B));
                            if (i0_a == i0_b && i1_a == i1_b) {
                                b_found = TTRUE;
                            }
                            else {
                                ++j;
                            }
                        }
                    }

                    if (!b_found) {
                        ++t;
                    }
                }

                // assign neighbors
                if (b_found) {
                    p_tri_infos[f].FaceNeighbors[i] = t;
                    //assert(pTriInfos[t].FaceNeighbors[j]==-1);
                    p_tri_infos[t].FaceNeighbors[j] = f;
                }
            }
        }
    }
}

static void QuickSortEdges(SEdge* pSortBuffer, int iLeft, int iRight, const int channel, unsigned int u_seed) {
    unsigned int t = 0;
    int          iL;
    int          iR;
    int          n;
    int          index;
    int          iMid;

    // early out
    SEdge     sTmp;
    const int iElems = iRight - iLeft + 1;
    if (iElems < 2) {
        return;
    }
    if (iElems == 2) {
        if (pSortBuffer[iLeft].array[channel] > pSortBuffer[iRight].array[channel]) {
            sTmp                = pSortBuffer[iLeft];
            pSortBuffer[iLeft]  = pSortBuffer[iRight];
            pSortBuffer[iRight] = sTmp;
        }
        return;
    }

    // Random
    t      = u_seed & 31;
    t      = (u_seed << t) | (u_seed >> (32 - t));
    u_seed = u_seed + t + 3;
    // Random end

    iL = iLeft;
    iR = iRight;
    n  = (iR - iL) + 1;
    assert(n >= 0);
    index = (int) (u_seed % n);

    iMid = pSortBuffer[index + iL].array[channel];

    do {
        while (pSortBuffer[iL].array[channel] < iMid) {
            ++iL;
        }
        while (pSortBuffer[iR].array[channel] > iMid) {
            --iR;
        }

        if (iL <= iR) {
            sTmp            = pSortBuffer[iL];
            pSortBuffer[iL] = pSortBuffer[iR];
            pSortBuffer[iR] = sTmp;
            ++iL;
            --iR;
        }
    }
    while (iL <= iR);

    if (iLeft < iR) {
        QuickSortEdges(pSortBuffer, iLeft, iR, channel, u_seed);
    }
    if (iL < iRight) {
        QuickSortEdges(pSortBuffer, iL, iRight, channel, u_seed);
    }
}

// resolve ordering and edge number
static void GetEdge(int* i0_out, int* i1_out, int* edgenum_out, const int indices[], const int i0_in, const int i1_in) {
    *edgenum_out = -1;

    // test if first index is on the edge
    if (indices[0] == i0_in || indices[0] == i1_in) {
        // test if second index is on the edge
        if (indices[1] == i0_in || indices[1] == i1_in) {
            edgenum_out[0] = 0; // first edge
            i0_out[0]      = indices[0];
            i1_out[0]      = indices[1];
        }
        else {
            edgenum_out[0] = 2; // third edge
            i0_out[0]      = indices[2];
            i1_out[0]      = indices[0];
        }
    }
    else {
        // only second and third index is on the edge
        edgenum_out[0] = 1; // second edge
        i0_out[0]      = indices[1];
        i1_out[0]      = indices[2];
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Degenerate triangles ////////////////////////////////////

static void DegenPrologue(STriInfo p_tri_infos[], int pi_tri_list_out[], const int i_nr_triangles_in, const int i_tot_tris) {
    int   i_next_good_triangle_search_index = -1;
    tbool b_still_finding_good_ones         = 0;

    // locate quads with only one good triangle
    int t = 0;
    while (t < (i_tot_tris - 1)) {
        const int i_fo_a = p_tri_infos[t].iOrgFaceNumber;
        const int i_fo_b = p_tri_infos[t + 1].iOrgFaceNumber;
        if (i_fo_a == i_fo_b) // this is a quad
        {
            const tbool b_is_deg_a = (p_tri_infos[t].iFlag & MARK_DEGENERATE) != 0 ? TTRUE : TFALSE;
            const tbool b_is_deg_b = (p_tri_infos[t + 1].iFlag & MARK_DEGENERATE) != 0 ? TTRUE : TFALSE;
            if ((b_is_deg_a ^ b_is_deg_b) != 0) {
                p_tri_infos[t].iFlag |= QUAD_ONE_DEGEN_TRI;
                p_tri_infos[t + 1].iFlag |= QUAD_ONE_DEGEN_TRI;
            }
            t += 2;
        }
        else {
            ++t;
        }
    }

    // reorder list so all degen triangles are moved to the back
    // without reordering the good triangles
    i_next_good_triangle_search_index = 1;
    t                                 = 0;
    b_still_finding_good_ones         = TTRUE;
    while (t < i_nr_triangles_in && b_still_finding_good_ones) {
        const tbool b_is_good = (p_tri_infos[t].iFlag & MARK_DEGENERATE) == 0 ? TTRUE : TFALSE;
        if (b_is_good) {
            if (i_next_good_triangle_search_index < (t + 2)) {
                i_next_good_triangle_search_index = t + 2;
            }
        }
        else {
            int t0;
            int t1;
            // search for the first good triangle.
            tbool b_just_a_degenerate = TTRUE;
            while (b_just_a_degenerate && i_next_good_triangle_search_index < i_tot_tris) {
                const tbool b_is_good = (p_tri_infos[i_next_good_triangle_search_index].iFlag & MARK_DEGENERATE) == 0 ? TTRUE : TFALSE;
                if (b_is_good) {
                    b_just_a_degenerate = TFALSE;
                }
                else {
                    ++i_next_good_triangle_search_index;
                }
            }

            t0 = t;
            t1 = i_next_good_triangle_search_index;
            ++i_next_good_triangle_search_index;
            assert(iNextGoodTriangleSearchIndex > (t + 1));

            // swap triangle t0 and t1
            if (!b_just_a_degenerate) {
                int i = 0;
                for (i = 0; i < 3; i++) {
                    const int index               = pi_tri_list_out[(t0 * 3) + i];
                    pi_tri_list_out[(t0 * 3) + i] = pi_tri_list_out[(t1 * 3) + i];
                    pi_tri_list_out[(t1 * 3) + i] = index;
                }
                {
                    const STriInfo tri_info = p_tri_infos[t0];
                    p_tri_infos[t0]         = p_tri_infos[t1];
                    p_tri_infos[t1]         = tri_info;
                }
            }
            else {
                b_still_finding_good_ones = TFALSE; // this is not supposed to happen
            }
        }

        if (b_still_finding_good_ones) {
            ++t;
        }
    }

    assert(bStillFindingGoodOnes); // code will still work.
    assert(iNrTrianglesIn == t);
}

static void DegenEpilogue(STSpace ps_tspace[], STriInfo p_tri_infos[], const int pi_tri_list_in[], const SMikkTSpaceContext* p_context, const int i_nr_triangles_in, const int i_tot_tris) {
    int t = 0;
    int i = 0;
    // deal with degenerate triangles
    // punishment for degenerate triangles is O(N^2)
    for (t = i_nr_triangles_in; t < i_tot_tris; t++) {
        // degenerate triangles on a quad with one good triangle are skipped
        // here but processed in the next loop
        const tbool b_skip = (p_tri_infos[t].iFlag & QUAD_ONE_DEGEN_TRI) != 0 ? TTRUE : TFALSE;

        if (!b_skip) {
            for (i = 0; i < 3; i++) {
                const int index1 = pi_tri_list_in[(t * 3) + i];
                // search through the good triangles
                tbool b_not_found = TTRUE;
                int   j           = 0;
                while (b_not_found && j < (3 * i_nr_triangles_in)) {
                    const int index2 = pi_tri_list_in[j];
                    if (index1 == index2) {
                        b_not_found = TFALSE;
                    }
                    else {
                        ++j;
                    }
                }

                if (!b_not_found) {
                    const int i_tri      = j / 3;
                    const int i_vert     = j % 3;
                    const int i_src_vert = p_tri_infos[i_tri].vert_num[i_vert];
                    const int i_src_offs = p_tri_infos[i_tri].iTSpacesOffs;
                    const int i_dst_vert = p_tri_infos[t].vert_num[i];
                    const int i_dst_offs = p_tri_infos[t].iTSpacesOffs;

                    // copy tspace
                    ps_tspace[i_dst_offs + i_dst_vert] = ps_tspace[i_src_offs + i_src_vert];
                }
            }
        }
    }

    // deal with degenerate quads with one good triangle
    for (t = 0; t < i_nr_triangles_in; t++) {
        // this triangle belongs to a quad where the
        // other triangle is degenerate
        if ((p_tri_infos[t].iFlag & QUAD_ONE_DEGEN_TRI) != 0) {
            SVec3          v_dst_p;
            int            iOrgF           = -1;
            int            i               = 0;
            tbool          b_not_found     = 0;
            unsigned char* p_v             = p_tri_infos[t].vert_num;
            int            i_flag          = (1 << p_v[0]) | (1 << p_v[1]) | (1 << p_v[2]);
            int            i_missing_index = 0;
            if ((i_flag & 2) == 0) {
                i_missing_index = 1;
            }
            else if ((i_flag & 4) == 0) {
                i_missing_index = 2;
            }
            else if ((i_flag & 8) == 0) {
                i_missing_index = 3;
            }

            iOrgF       = p_tri_infos[t].iOrgFaceNumber;
            v_dst_p     = GetPosition(p_context, MakeIndex(iOrgF, i_missing_index));
            b_not_found = TTRUE;
            i           = 0;
            while (b_not_found && i < 3) {
                const int   i_vert  = p_v[i];
                const SVec3 v_src_p = GetPosition(p_context, MakeIndex(iOrgF, i_vert));
                if (veq(v_src_p, v_dst_p) == TTRUE) {
                    const int i_offs                    = p_tri_infos[t].iTSpacesOffs;
                    ps_tspace[i_offs + i_missing_index] = ps_tspace[i_offs + i_vert];
                    b_not_found                         = TFALSE;
                }
                else {
                    ++i;
                }
            }
            assert(!bNotFound);
        }
    }
}
