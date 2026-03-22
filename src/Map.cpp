// THIS IS DECOMPILED PROPRIETARY CODE - USE AT YOUR OWN RISK.
//
// The original code belongs to Daisuke "Pixel" Amaya.
//
// Modifications and custom code are under the MIT licence.
// See LICENCE.txt for details.

#include "Map.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "WindowsWrapper.h"

#include "CommonDefines.h"
#include "Draw.h"
#include "File.h"
#include "Main.h"
#include "NpChar.h"

#define PXM_BUFFER_SIZE 0x4B000

MAP_DATA gMap;

const char *code_pxma = "PXM";

int last_surface_made = SURFACE_ID_MAP_IMG;
int total_made = 0;
int current_made = 0;
unsigned char GetAttributeL(int x, int y, unsigned char *data)
{
    size_t a;

    if (x < 0 || y < 0 || x >= gMap.width || y >= gMap.length)
        return 0;

    a = *(data + x + (y * gMap.width));	// Yes, the original code really does do this instead of a regular array access
    return gMap.atrb[a];
}
void ResetAllMapImages(){
    current_made = 0;
    last_surface_made = SURFACE_ID_MAP_IMG;
}

int GetRenderedMapImage(unsigned char *data, int w, int h, bool i9_style){
    SurfaceID surface_id = static_cast<SurfaceID>(last_surface_made++);
    SurfaceID surface_id_white = static_cast<SurfaceID>(last_surface_made++);
    //int i_size = 16384/2;
    int i_width = w*0x10;
    int i_height = h*0x10;
    //if(current_made >= total_made){
    MakeSurface_Generic(i_width, i_height, surface_id, FALSE, TRUE);
    MakeSurface_Generic(i_width, i_height, surface_id_white, FALSE, TRUE);
    //total_made += 2;
    //}
    current_made += 2;
    ClearTexture(surface_id);
    ClearTexture(surface_id_white);

    int i, j;

    RECT rect;
    int offset;

    for (j = 0; j < h; ++j)
    {
        for (i = 0; i < w; ++i)
        {
            // Get attribute
            offset = (j * gMap.width) + i;

            // Draw tile
            rect.left = (data[offset] % 16) * 16;
            rect.top = (data[offset] / 16) * 16;
            rect.right = rect.left + 16;
            rect.bottom = rect.top + 16;
            bool draw_tile = true;
            if(i9_style){
                switch (GetAttributeL(i, j, data)) {
                    case 0x43:
                    case 0x42:
                        draw_tile = false;
                        break;
                    default:
                        break;
                }
            }else{
                switch (GetAttributeL(i, j, data)) {
                    case 0x01:
                    case 0x40:
                    case 0x43:
                    case 0x42:
                        draw_tile = false;
                        break;
                    default:
                        break;
                }
            }

            if(draw_tile){
                Surface2Surface(i*16, j*16, &rect, surface_id, SURFACE_ID_LEVEL_TILESET);
            }
        }
    }
    RECT rc = {0,0,i_width,i_height};
    Surface2Surface(0,0, &rc, surface_id_white, surface_id);
    MakeEveryPixelWhiteExceptThoseThatHaveAlphaZero(surface_id_white);
    printf("%d, %d xxxxxx", i, j);
    return surface_id;
}
BOOL InitMapData2(void)
{
	gMap.data = (unsigned char*)malloc(PXM_BUFFER_SIZE);
	return TRUE;
}

BOOL LoadMapData2(const char *path_map)
{
    FILE *fp;
    char check[3];
    std::string path;
    std::string path_pxl;

    // Get path
    path = gDataPath + '/' + path_map;
    path_pxl = path.substr(0, path.length() - 4);
    path_pxl = path_pxl + ".pxl";

    // Check for PXL first
    fp = fopen(path_pxl.c_str(), "rb");
    if(fp == NULL){
        printf("No PXL found for %s, trying PXM instead.\n", path_pxl.c_str());
        // Try PXM instead...
        fp = fopen(path.c_str(), "rb");
        if (fp == NULL)
            return FALSE;
    }else{
        // PXL found
    }

    // Make sure file begins with "PXM" (PXLs also start with PXM to minimize boosters lab changes)
    fread(check, 1, 3, fp);

    if (memcmp(check, code_pxma, 3))
    {
        fclose(fp);
        return FALSE;
    }

    unsigned char dum;
    fread(&dum, 1, 1, fp);
    // Get width and height
    gMap.width = File_ReadLE16(fp);
    gMap.length = File_ReadLE16(fp);

    if (gMap.data == NULL)
    {
        fclose(fp);
        return FALSE;
    }

    // Reset previous map
    ResetAllMapImages();
    for (int i = 0; i < 512; ++i) {
        gMap.layer_depth[i] = -1;
        gMap.image_data[i] = 0;
        free(gMap.full_data[i]);
        gMap.full_data[i] = NULL;
    }
    gMap.main_layer = -1;
    // Read tile data
    if(dum == 0x21){ // The PXL format,
        short reverseCompatLayer = File_ReadLE16(fp); // The main layer
        gMap.main_layer = reverseCompatLayer;
        for (int layer = 0; layer < 512; ++layer) {
            unsigned char layerDataType;
            fread(&layerDataType, 1, 1, fp);
            if(layerDataType == 0x22){
                unsigned int depth = File_ReadLE16(fp);
                gMap.layer_depth[layer] = depth;
                gMap.full_data[layer] = static_cast<unsigned char *>(malloc(gMap.width * gMap.length));
                fread(gMap.full_data[layer], 1, gMap.width * gMap.length, fp);
                int surf_id = GetRenderedMapImage(gMap.full_data[layer], gMap.width, gMap.length, true);
                //int surf_id = 3;
                gMap.image_data[layer] = surf_id;
            }else if(layerDataType == 0x11){ //Empty (Depth only)
                unsigned short depth = File_ReadLE16(fp);
                gMap.layer_depth[layer] = depth;
                gMap.image_data[layer] = -1;
            }
        }
        gMap.data = gMap.full_data[reverseCompatLayer]; // Dynamically updated later
        if(gMap.data == NULL){
            gMap.data = static_cast<unsigned char *>(malloc(gMap.width * gMap.length));
        }
    }else{
        gMap.data = static_cast<unsigned char *>(malloc(PXM_BUFFER_SIZE));
        fread(gMap.data, 1, gMap.width * gMap.length, fp);
        gMap.image_data[0] = GetRenderedMapImage(gMap.data, gMap.width, gMap.length, false);
    }


    fclose(fp);
    // gMap.image_data[0] = GetRenderedMapImage(gMap.data, gMap.width, gMap.length);
    return TRUE;
}

BOOL LoadAttributeData(const char *path_atrb)
{
	FILE *fp;
	std::string path;

	// Open file
	path = gDataPath + '/' + path_atrb;

	fp = fopen(path.c_str(), "rb");
	if (fp == NULL)
		return FALSE;

	// Read data
	fread(gMap.atrb, 1, sizeof(gMap.atrb), fp);
	fclose(fp);
	return TRUE;
}

void EndMapData(void)
{
	free(gMap.data);
}

void ReleasePartsImage(void)
{
	ReleaseSurface(SURFACE_ID_LEVEL_TILESET);
}

void GetMapData(unsigned char **data, short *mw, short *ml)
{
	if (data != NULL)
		*data = gMap.data;

	if (mw != NULL)
		*mw = gMap.width;

	if (ml != NULL)
		*ml = gMap.length;
}

unsigned char GetAttribute(int x, int y)
{
	size_t a;

	if (x < 0 || y < 0 || x >= gMap.width || y >= gMap.length)
		return 0;

	a = *(gMap.data + x + (y * gMap.width));	// Yes, the original code really does do this instead of a regular array access
	return gMap.atrb[a];
}

void DeleteMapParts(int x, int y)
{
	*(gMap.data + x + (y * gMap.width)) = 0;
}

void ShiftMapParts(int x, int y)
{
	*(gMap.data + x + (y * gMap.width)) -= 1;
}

BOOL ChangeMapParts(int x, int y, unsigned char no)
{
	int i;

	if (*(gMap.data + x + (y * gMap.width)) == no)
		return FALSE;

	*(gMap.data + x + (y * gMap.width)) = no;

	for (i = 0; i < 3; ++i)
		SetNpChar(4, x * 0x200 * 0x10, y * 0x200 * 0x10, 0, 0, 0, NULL, 0);

	return TRUE;
}

void PutStage_Back(int fx, int fy)
{
	int i, j;
	RECT rect;
	int offset;

	// Get range to draw
	int num_x = ((WINDOW_WIDTH + (16 - 1)) / 16) + 1;
	int num_y = ((WINDOW_HEIGHT + (16 - 1)) / 16) + 1;
	int put_x = ((fx / 0x200) + 8) / 16;
	int put_y = ((fy / 0x200) + 8) / 16;

	int atrb;

	for (j = put_y; j < put_y + num_y; ++j)
	{
		for (i = put_x; i < put_x + num_x; ++i)
		{
			// Get attribute
			offset = (j * gMap.width) + i;
			atrb = GetAttribute(i, j);

			if (atrb >= 0x20)
				continue;

			// Draw tile
			rect.left = (gMap.data[offset] % 16) * 16;
			rect.top = (gMap.data[offset] / 16) * 16;
			rect.right = rect.left + 16;
			rect.bottom = rect.top + 16;

			PutBitmap3(&grcGame, ((i * 16) - 8) - (fx / 0x200), ((j * 16) - 8) - (fy / 0x200), &rect, SURFACE_ID_LEVEL_TILESET);
		}
	}
}
#define POINT_X (WINDOW_WIDTH/2.0)
#define POINT_Y (WINDOW_HEIGHT/2.0)
double LAYER_START_PCT = 0.5;
double LAYER_END_PCT = 1.1;
double FOG_START = 0.1;
double FOG_END = 0.2;
double LAYER_SPACING = 0.01; // i8 only
int interpolate(int a, int b, double pct){
    return (static_cast<double>(b)*pct + static_cast<double>(a)*(1.0-pct));
}
int fog_r, fog_g, fog_b;
void PutStage_BKG(int fx, int fy, double layerfx, int layer_no)
{
    double x = fx + 8*0x200 - POINT_X/layerfx*0x200 + POINT_X*0x200;
    double y = fy + 8*0x200 - POINT_Y/layerfx*0x200 + POINT_Y*0x200;
    double w = WINDOW_WIDTH/layerfx;
    double h = WINDOW_HEIGHT/layerfx;
    DRECT rc = {x/0x200, y/0x200, x/0x200+w,y/0x200+h};

    DRECT rcView = {0,0, grcFull.right/layerfx + 2, grcFull.bottom/layerfx + 2};

    double xo = 0;
    double yo = 0;
    if(rc.left < 0){
        xo = -rc.left*0x200;
        rc.left = 0;
    }
    if(rc.top < 0){
        yo = -rc.top*0x200;
        rc.top = 0;
    }
    if(rc.right > gMap.width*0x10){
        rc.right = gMap.width*0x10;
    }
    if(rc.bottom > gMap.length*0x10){
        rc.bottom = gMap.length*0x10;
    }

    // 0.5-0.6
    double fog_delta = FOG_END - FOG_START;
    double pct = layerfx - FOG_START;
    pct /= fog_delta;
    pct = 1 - pct;
    if(pct < 0){
        pct = 0;
    }


    // Don't even bother drawing pct >1 layers
    if(pct < 1.00001) {
        if (pct <= 0.0001) {
            PutBitmapEx(&rcView, ((xo * layerfx) / 0x200), ((yo * layerfx) / 0x200), &rc,
                        static_cast<SurfaceID>(gMap.image_data[layer_no]), layerfx,
                        255, 255, 255);
        } else {
            PutBitmapInterpolate(&rcView, (xo * layerfx) / 0x200, (yo * layerfx) / 0x200, &rc,
                                 static_cast<SurfaceID>(gMap.image_data[layer_no]),
                                 static_cast<SurfaceID>(gMap.image_data[layer_no] + 1), layerfx, fog_r, fog_g, fog_b,
                                 pct);
        }
    }
}
void PutStage_Front(int fx, int fy)
{
	RECT rcSnack = {256, 48, 272, 64};
	int i, j;
	RECT rect;
	int offset;

	// Get range to draw
	int num_x = ((WINDOW_WIDTH + (16 - 1)) / 16) + 1;
	int num_y = ((WINDOW_HEIGHT + (16 - 1)) / 16) + 1;
	int put_x = ((fx / 0x200) + 8) / 16;
	int put_y = ((fy / 0x200) + 8) / 16;

	int atrb;

	for (j = put_y; j < put_y + num_y; ++j)
	{
		for (i = put_x; i < put_x + num_x; ++i)
		{
			// Get attribute
			offset = (j * gMap.width) + i;
			atrb = GetAttribute(i, j);

			if (atrb < 0x40 || atrb >= 0x80)
				continue;

			// Draw tile
			rect.left = (gMap.data[offset] % 16) * 16;
			rect.top = (gMap.data[offset] / 16) * 16;
			rect.right = rect.left + 16;
			rect.bottom = rect.top + 16;

			PutBitmap3(&grcGame, ((i * 16) - 8) - (fx / 0x200), ((j * 16) - 8) - (fy / 0x200), &rect, SURFACE_ID_LEVEL_TILESET);

			if (atrb == 0x43)
				PutBitmap3(&grcGame, ((i * 16) - 8) - (fx / 0x200), ((j * 16) - 8) - (fy / 0x200), &rcSnack, SURFACE_ID_NPC_SYM);
		}
	}
}

void PutMapDataVector(int fx, int fy)
{
	int i, j;
	RECT rect;
	int offset;

	int num_x;
	int num_y;
	int put_x;
	int put_y;

	static unsigned char count = 0;

	int atrb;

	// Animate the wind
	count += 2;

	// Get range to draw
	num_x = ((WINDOW_WIDTH + (16 - 1)) / 16) + 1;
	num_y = ((WINDOW_HEIGHT + (16 - 1)) / 16) + 1;
	put_x = ((fx / 0x200) + 8) / 16;
	put_y = ((fy / 0x200) + 8) / 16;

	for (j = put_y; j < put_y + num_y; ++j)
	{
		for (i = put_x; i < put_x + num_x; ++i)
		{
			// Get attribute
			offset = (j * gMap.width) + i;
			atrb = GetAttribute(i, j);

			if (atrb != 0x80
				&& atrb != 0x81
				&& atrb != 0x82
				&& atrb != 0x83
				&& atrb != 0xA0
				&& atrb != 0xA1
				&& atrb != 0xA2
				&& atrb != 0xA3)
				continue;

			switch (atrb)
			{
				case 128:
				case 160:
					rect.left = 224 + (count % 16);
					rect.right = rect.left + 16;
					rect.top = 48;
					rect.bottom = rect.top + 16;
					break;

				case 129:
				case 161:
					rect.left = 224;
					rect.right = rect.left + 16;
					rect.top = 48 + (count % 16);
					rect.bottom = rect.top + 16;
					break;

				case 130:
				case 162:
					rect.left = 240 - (count % 16);
					rect.right = rect.left + 16;
					rect.top = 48;
					rect.bottom = rect.top + 16;
					break;

				case 131:
				case 163:
					rect.left = 224;
					rect.right = rect.left + 16;
					rect.top = 64 - (count % 16);
					rect.bottom = rect.top + 16;
					break;
			}

			PutBitmap3(&grcGame, ((i * 16) - 8) - (fx / 0x200), ((j * 16) - 8) - (fy / 0x200), &rect, SURFACE_ID_CARET);
		}
	}
}
