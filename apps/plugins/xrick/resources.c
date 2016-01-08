/*
 * xrick/resources.c
 *
 * Copyright (C) 2008-2014 Pierluigi Vicinanza. All rights reserved.
 *
 * The use and distribution terms for this software are contained in the file
 * named README, which can be found in the root of this distribution. By
 * using this software in any fashion, you are agreeing to be bound by the
 * terms of this license.
 *
 * You must not remove this notice, or any other, from this software.
 */

#include "xrick/resources.h"

#include "xrick/draw.h"
#include "xrick/ents.h"
#include "xrick/maps.h"
#include "xrick/util.h"
#include "xrick/data/sprites.h"
#include "xrick/data/tiles.h"
#include "xrick/data/pics.h"
#include "xrick/system/basic_funcs.h"

#include "xrick/system/miniz_config.h"
#include "miniz/miniz.c"

/*
 * prototypes
 */
static bool readFile(const unsigned id);
static bool checkCrc32(const unsigned id);
static bool readHeader(file_t fp, const unsigned id);
static bool loadString(file_t fp, char ** str, const char terminator);
static void unloadString(char ** buffer);
static bool loadResourceFilelist(file_t fp);
static void unloadResourceFilelist(void);
static bool loadResourceEntdata(file_t fp);
static void unloadResourceEntdata(void);
static bool loadRawData(file_t fp, void ** buffer, const size_t size, size_t * count);
static void unloadRawData(void ** buffer, size_t * count);
static bool loadResourceMaps(file_t fp);
static void unloadResourceMaps(void);
static bool loadResourceSubmaps(file_t fp);
static void unloadResourceSubmaps(void);
static bool loadResourceImapsteps(file_t fp);
static void unloadResourceImapsteps(void);
static bool loadResourceImaptext(file_t fp);
static void unloadResourceImaptext(void);
static bool loadResourceHighScores(file_t fp);
static void unloadResourceHighScores(void);
static bool loadResourceSpritesData(file_t fp);
static void unloadResourceSpritesData(void);
static bool loadResourceTilesData(file_t fp);
static void unloadResourceTilesData(void);
static bool loadImage(file_t fp, img_t ** image);
static void unloadImage(img_t ** image);
#ifdef GFXST
static bool loadPicture(file_t fp, pic_t ** picture);
static void unloadPicture(pic_t ** picture);
#endif /* GFXST */
#ifdef ENABLE_SOUND
static bool fromResourceIdToSound(const unsigned id, sound_t *** sound);
static bool loadSound(const unsigned id);
static void unloadSound(const unsigned id);
#endif /* ENABLE_SOUND */

/*
 * local vars
 */
static char * resourceFiles[Resource_MAX_COUNT] =
{
    BOOTSTRAP_RESOURCE_NAME,
    /* the rest initialised to NULL by default */
};

/*
 * load 16b length + not-terminated string
 */
static bool loadString(file_t fp, char ** buffer, const char terminator)
{
    size_t length;
    U16 u16Temp;
    char * bufferTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    length = letoh16(u16Temp);

    bufferTemp = sysmem_push(length + 1);
    *buffer = bufferTemp;
    if (!bufferTemp)
    {
        return false;
    }

    if (length)
    {
        if (sysfile_read(fp, bufferTemp, length, 1) != 1)
        {
            return false;
        }
    }

    bufferTemp[length] = terminator;

    return true;
}

/*
 *
 */
static void unloadString(char ** buffer)
{
    sysmem_pop(*buffer);
    *buffer = NULL;
}

/*
 *
 */
static bool loadResourceFilelist(file_t fp)
{
    unsigned id;

    for (id = Resource_PALETTE; id < Resource_MAX_COUNT; ++id)
    {
        if (!loadString(fp, &(resourceFiles[id]), 0x00))
        {
            return false;
        }
    }
    return true;
}

/*
 *
 */
static void unloadResourceFilelist()
{
    unsigned id;

    for (id = Resource_MAX_COUNT - 1; id >= Resource_PALETTE; --id)
    {
        unloadString(&(resourceFiles[id]));
    }
}

/*
 *
 */
static bool loadResourceEntdata(file_t fp)
{
    size_t i;
    U16 u16Temp;
    resource_entdata_t dataTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    ent_nbr_entdata = letoh16(u16Temp);

    ent_entdata = sysmem_push(ent_nbr_entdata * sizeof(*ent_entdata));
    if (!ent_entdata)
    {
        return false;
    }

    for (i = 0; i < ent_nbr_entdata; ++i)
    {
        if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
        {
            return false;
        }
        ent_entdata[i].w = dataTemp.w;
        ent_entdata[i].h = dataTemp.h;
        memcpy(&u16Temp, dataTemp.spr, sizeof(U16));
        ent_entdata[i].spr = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.sni, sizeof(U16));
        ent_entdata[i].sni = letoh16(u16Temp);
        ent_entdata[i].trig_w = dataTemp.trig_w;
        ent_entdata[i].trig_h = dataTemp.trig_h;
        ent_entdata[i].snd = dataTemp.snd;
    }
    return true;
}

/*
 *
 */
static void unloadResourceEntdata()
{
    sysmem_pop(ent_entdata);
    ent_entdata = NULL;
    ent_nbr_entdata = 0;
}

/*
 *
 */
static bool loadRawData(file_t fp, void ** buffer, const size_t size, size_t * count)
{
    U16 u16Temp;
    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    *count = letoh16(u16Temp);

    *buffer = sysmem_push((*count) * size);
    if (!(*buffer))
    {
        return false;
    }

    if (sysfile_read(fp, *buffer, size, *count) != (int)(*count))
    {
        return false;
    }
    return true;
}

/*
 *
 */
static void unloadRawData(void ** buffer, size_t * count)
{
    sysmem_pop(*buffer);
    *buffer = NULL;
    *count = 0;
}

/*
 *
 */
static bool loadResourceMaps(file_t fp)
{
    size_t i;
    U16 u16Temp;
    resource_map_t dataTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    map_nbr_maps = letoh16(u16Temp);

    map_maps = sysmem_push(map_nbr_maps * sizeof(*map_maps));
    if (!map_maps)
    {
        return false;
    }

    for (i = 0; i < map_nbr_maps; ++i)
    {
#ifdef ENABLE_SOUND
        sound_t **soundTemp;
#endif
        if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
        {
            return false;
        }
        memcpy(&u16Temp, dataTemp.x, sizeof(U16));
        map_maps[i].x = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.y, sizeof(U16));
        map_maps[i].y = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.row, sizeof(U16));
        map_maps[i].row = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.submap, sizeof(U16));
        map_maps[i].submap = letoh16(u16Temp);
#ifdef ENABLE_SOUND
        memcpy(&u16Temp, dataTemp.tuneId, sizeof(U16));
        if (!fromResourceIdToSound(letoh16(u16Temp), &soundTemp))
        {
            return false;
        }
        map_maps[i].tune = *soundTemp;
#endif /* ENABLE_SOUND */
    }
    return true;
}

/*
 *
 */
static void unloadResourceMaps()
{
    sysmem_pop(map_maps);
    map_maps = NULL;
    map_nbr_maps = 0;
}

/*
 *
 */
static bool loadResourceSubmaps(file_t fp)
{
    size_t i;
    U16 u16Temp;
    resource_submap_t dataTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    map_nbr_submaps = letoh16(u16Temp);

    map_submaps = sysmem_push(map_nbr_submaps * sizeof(*map_submaps));
    if (!map_submaps)
    {
        return false;
    }

    for (i = 0; i < map_nbr_submaps; ++i)
    {
        if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
        {
            return false;
        }
        memcpy(&u16Temp, dataTemp.page, sizeof(U16));
        map_submaps[i].page = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.bnum, sizeof(U16));
        map_submaps[i].bnum = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.connect, sizeof(U16));
        map_submaps[i].connect = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.mark, sizeof(U16));
        map_submaps[i].mark = letoh16(u16Temp);
    }
    return true;
}

/*
 *
 */
static void unloadResourceSubmaps()
{
    sysmem_pop(map_submaps);
    map_submaps = NULL;
    map_nbr_submaps = 0;
}

/*
 *
 */
static bool loadResourceImapsteps(file_t fp)
{
    size_t i;
    U16 u16Temp;
    resource_imapsteps_t dataTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    screen_nbr_imapstesps = letoh16(u16Temp);

    screen_imapsteps = sysmem_push(screen_nbr_imapstesps * sizeof(*screen_imapsteps));
    if (!screen_imapsteps)
    {
        return false;
    }

    for (i = 0; i < screen_nbr_imapstesps; ++i)
    {
        if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
        {
            return false;
        }
        memcpy(&u16Temp, dataTemp.count, sizeof(U16));
        screen_imapsteps[i].count = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.dx, sizeof(U16));
        screen_imapsteps[i].dx = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.dy, sizeof(U16));
        screen_imapsteps[i].dy = letoh16(u16Temp);
        memcpy(&u16Temp, dataTemp.base, sizeof(U16));
        screen_imapsteps[i].base = letoh16(u16Temp);
    }
    return true;
}

/*
 *
 */
static void unloadResourceImapsteps()
{
    sysmem_pop(screen_imapsteps);
    screen_imapsteps = NULL;
    screen_nbr_imapstesps = 0;
}

/*
 *
 */
static bool loadResourceImaptext(file_t fp)
{
    size_t i;
    U16 u16Temp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    screen_nbr_imaptext = letoh16(u16Temp);

    screen_imaptext = sysmem_push(screen_nbr_imaptext * sizeof(*screen_imaptext));
    if (!screen_imapsteps)
    {
        return false;
    }

    for (i = 0; i < screen_nbr_imaptext; ++i)
    {
        if (!loadString(fp, (char **)(&(screen_imaptext[i])), 0xFE))
        {
            return false;
        }
    }
    return true;
}

/*
 *
 */
static void unloadResourceImaptext()
{
    int i;

    for (i = screen_nbr_imaptext - 1; i >= 0; --i)
    {
        unloadString((char **)(&(screen_imaptext[i])));
    }

    sysmem_pop(screen_imaptext);
    screen_imaptext = NULL;
    screen_nbr_imaptext = 0;
}

/*
 *
 */
static bool loadResourceHighScores(file_t fp)
{
    size_t i;
    U16 u16Temp;
    U32 u32Temp;
    resource_hiscore_t dataTemp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    screen_nbr_hiscores = letoh16(u16Temp);

    screen_highScores = sysmem_push(screen_nbr_hiscores * sizeof(*screen_highScores));
    if (!screen_highScores)
    {
        return false;
    }

    for (i = 0; i < screen_nbr_hiscores; ++i)
    {
        if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
        {
            return false;
        }
        memcpy(&u32Temp, dataTemp.score, sizeof(U32));
        screen_highScores[i].score = letoh32(u32Temp);
        memcpy(screen_highScores[i].name, dataTemp.name, HISCORE_NAME_SIZE);
    }
    return true;
}

/*
 *
 */
static void unloadResourceHighScores()
{
    sysmem_pop(screen_highScores);
    screen_highScores = NULL;
    screen_nbr_hiscores = 0;
}

/*
 *
 */
static bool loadResourceSpritesData(file_t fp)
{
    size_t i, j;
    U16 u16Temp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    sprites_nbr_sprites = letoh16(u16Temp);

    sprites_data = sysmem_push(sprites_nbr_sprites * sizeof(*sprites_data));
    if (!sprites_data)
    {
        return false;
    }

#ifdef GFXST
    for (i = 0; i < sprites_nbr_sprites; ++i)
    {
        for (j = 0; j < SPRITES_NBR_DATA; ++j)
        {
            U32 u32Temp;
            if (sysfile_read(fp, &u32Temp, sizeof(u32Temp), 1) != 1)
            {
                return false;
            }
            sprites_data[i][j] = letoh32(u32Temp);
        }
    }
#endif /* GFXST */

#ifdef GFXPC
    for (i = 0; i < sprites_nbr_sprites; ++i)
    {
        for (j = 0; j < SPRITES_NBR_COLS; ++j)
        {
            size_t k;
            for (k = 0; k < SPRITES_NBR_ROWS; ++k)
            {
                resource_spriteX_t dataTemp;
                if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
                {
                    return false;
                }
                memcpy(&u16Temp, dataTemp.mask, sizeof(U16));
                sprites_data[i][j][k].mask = letoh16(u16Temp);
                memcpy(&u16Temp, dataTemp.pict, sizeof(U16));
                sprites_data[i][j][k].pict = letoh16(u16Temp);
            }
        }
    }
#endif /* GFXPC */

    return true;
}

/*
 *
 */
static void unloadResourceSpritesData()
{
    sysmem_pop(sprites_data);
    sprites_data = NULL;
    sprites_nbr_sprites = 0;
}

/*
 *
 */
static bool loadResourceTilesData(file_t fp)
{
    size_t i, j, k;
    U16 u16Temp;

    if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
    {
        return false;
    }
    tiles_nbr_banks = letoh16(u16Temp);

    tiles_data = sysmem_push(tiles_nbr_banks * TILES_NBR_TILES * sizeof(*tiles_data));
    if (!tiles_data)
    {
        return false;
    }

    for (i = 0; i < tiles_nbr_banks ; ++i)
    {
        for (j = 0; j < TILES_NBR_TILES; ++j)
        {
            for (k = 0; k < TILES_NBR_LINES ; ++k)
            {
#ifdef GFXPC
                if (sysfile_read(fp, &u16Temp, sizeof(u16Temp), 1) != 1)
                {
                    return false;
                }
                tiles_data[i * TILES_NBR_TILES + j][k] = letoh16(u16Temp);
#endif /* GFXPC */
#ifdef GFXST
                U32 u32Temp;
                if (sysfile_read(fp, &u32Temp, sizeof(u32Temp), 1) != 1)
                {
                    return false;
                }
                tiles_data[i * TILES_NBR_TILES + j][k] = letoh32(u32Temp);
#endif /* GFXST */
            }
        }
    }
    return true;
}

/*
 *
 */
static void unloadResourceTilesData()
{
    sysmem_pop(tiles_data);
    tiles_data = NULL;
    tiles_nbr_banks = 0;
}

/*
 *
 */
static bool loadImage(file_t fp, img_t ** image)
{
    U16 u16Temp;
    size_t pixelCount, colorCount;
    resource_pic_t dataTemp;
    img_t * imgTemp;
    void * vp;
    bool success;

    imgTemp = sysmem_push(sizeof(*imgTemp));
    *image = imgTemp;
    if (!imgTemp)
    {
        return false;
    }

    if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
    {
        return false;
    }
    memcpy(&u16Temp, dataTemp.width, sizeof(U16));
    imgTemp->width = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.height, sizeof(U16));
    imgTemp->height = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.xPos, sizeof(U16));
    imgTemp->xPos = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.yPos, sizeof(U16));
    imgTemp->yPos = letoh16(u16Temp);

    vp = imgTemp->colors;
    success = loadRawData(fp, &vp, sizeof(*imgTemp->colors), &colorCount);
    imgTemp->ncolors = colorCount;
    imgTemp->colors = vp;
    if (!success)
    {
        return false;
    }

    pixelCount = (imgTemp->width * imgTemp->height);  /*we use 8b per pixel*/

    imgTemp->pixels = sysmem_push(pixelCount * sizeof(U8));
    if (!imgTemp->pixels)
    {
        return false;
    }

    if (sysfile_read(fp, imgTemp->pixels, sizeof(U8), pixelCount) != (int)pixelCount)
    {
        return false;
    }
    return true;
}

/*
 *
 */
static void unloadImage(img_t ** image)
{
    if (*image)
    {
        sysmem_pop((*image)->pixels);
        sysmem_pop((*image)->colors);
    }
    sysmem_pop(*image);
    *image = NULL;
}

/*
 *
 */
#ifdef GFXST
static bool loadPicture(file_t fp, pic_t ** picture)
{
    U16 u16Temp;
    size_t i, pixelWords32b;
    resource_pic_t dataTemp;
    pic_t * picTemp;

    picTemp = sysmem_push(sizeof(*picTemp));
    *picture = picTemp;
    if (!picTemp)
    {
        return false;
    }

    if (sysfile_read(fp, &dataTemp, sizeof(dataTemp), 1) != 1)
    {
        return false;
    }
    memcpy(&u16Temp, dataTemp.width, sizeof(U16));
    picTemp->width = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.height, sizeof(U16));
    picTemp->height = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.xPos, sizeof(U16));
    picTemp->xPos = letoh16(u16Temp);
    memcpy(&u16Temp, dataTemp.yPos, sizeof(U16));
    picTemp->yPos = letoh16(u16Temp);

    pixelWords32b = (picTemp->width * picTemp->height) / 8;  /*we use 4b per pixel*/

    picTemp->pixels = sysmem_push(pixelWords32b * sizeof(U32));
    if (!picTemp->pixels)
    {
        return false;
    }

    for (i = 0; i < pixelWords32b; ++i)
    {
        U32 u32Temp;
        if (sysfile_read(fp, &u32Temp, sizeof(u32Temp), 1) != 1)
        {
            return false;
        }
        picTemp->pixels[i] = letoh32(u32Temp);
    }
    return true;
}

/*
 *
 */
static void unloadPicture(pic_t ** picture)
{
    if (*picture)
    {
        sysmem_pop((*picture)->pixels);
    }
    sysmem_pop(*picture);
    *picture = NULL;
}
#endif /* GFXST */

#ifdef ENABLE_SOUND

/*
 *
 */
static bool fromResourceIdToSound(const unsigned id, sound_t *** sound)
{
    switch (id)
    {
        case Resource_SOUNDBOMBSHHT: *sound = &soundBombshht; return true;
        case Resource_SOUNDBONUS: *sound = &soundBonus; return true;
        case Resource_SOUNDBOX: *sound = &soundBox; return true;
        case Resource_SOUNDBULLET: *sound = &soundBullet; return true;
        case Resource_SOUNDCRAWL: *sound = &soundCrawl; return true;
        case Resource_SOUNDDIE: *sound = &soundDie; return true;
        case Resource_SOUNDENTITY0: *sound = &(soundEntity[0]); return true;
        case Resource_SOUNDENTITY1: *sound = &(soundEntity[1]); return true;
        case Resource_SOUNDENTITY2: *sound = &(soundEntity[2]); return true;
        case Resource_SOUNDENTITY3: *sound = &(soundEntity[3]); return true;
        case Resource_SOUNDENTITY4: *sound = &(soundEntity[4]); return true;
        case Resource_SOUNDENTITY5: *sound = &(soundEntity[5]); return true;
        case Resource_SOUNDENTITY6: *sound = &(soundEntity[6]); return true;
        case Resource_SOUNDENTITY7: *sound = &(soundEntity[7]); return true;
        case Resource_SOUNDENTITY8: *sound = &(soundEntity[8]); return true;
        case Resource_SOUNDEXPLODE: *sound = &soundExplode; return true;
        case Resource_SOUNDGAMEOVER: *sound = &soundGameover; return true;
        case Resource_SOUNDJUMP: *sound = &soundJump; return true;
        case Resource_SOUNDPAD: *sound = &soundPad; return true;
        case Resource_SOUNDSBONUS1: *sound = &soundSbonus1; return true;
        case Resource_SOUNDSBONUS2: *sound = &soundSbonus2; return true;
        case Resource_SOUNDSTICK: *sound = &soundStick; return true;
        case Resource_SOUNDTUNE0: *sound = &soundTune0; return true;
        case Resource_SOUNDTUNE1: *sound = &soundTune1; return true;
        case Resource_SOUNDTUNE2: *sound = &soundTune2; return true;
        case Resource_SOUNDTUNE3: *sound = &soundTune3; return true;
        case Resource_SOUNDTUNE4: *sound = &soundTune4; return true;
        case Resource_SOUNDTUNE5: *sound = &soundTune5; return true;
        case Resource_SOUNDWALK: *sound = &soundWalk; return true;
        default:
        {
            sys_error("(resources) no associated sound found for ID %d", id);
            return false;
        }
    }
}

/*
 *
 */
static bool loadSound(const unsigned id)
{
    sound_t ** sound;
    file_t fp;
    wave_header_t header;
    U16 u16Temp;
    U32 u32Temp;
    int bytesRead;
    bool isHeaderValid;

    if (!fromResourceIdToSound(id, &sound))
    {
        return false;
    }

    *sound = sysmem_push(sizeof(**sound));
    if (!*sound)
    {
        return false;
    }

    (*sound)->buf = NULL;
    (*sound)->dispose = true; /* sounds are "fire and forget" by default */

    (*sound)->name = u_strdup(resourceFiles[id]);
    if (!(*sound)->name)
    {
        return false;
    }

    fp = sysfile_open(resourceFiles[id]);
    if (!fp)
    {
        sys_error("(resources) unable to open \"%s\"", resourceFiles[id]);
        return false;
    }

    bytesRead = sysfile_read(fp, &header, sizeof(header), 1);
    sysfile_close(fp);
    if (bytesRead != 1)
    {
        sys_error("(resources) unable to read WAVE header from \"%s\"", resourceFiles[id]);
        return false;
    }

    isHeaderValid = false;
    for (;;)
    {
        if (memcmp(header.riffChunkId, "RIFF", 4) ||
            memcmp(header.riffType, "WAVE", 4) ||
            memcmp(header.formatChunkId, "fmt ", 4) ||
            memcmp(header.dataChunkId, "data", 4))
        {
            break;
        }
        memcpy(&u16Temp, header.audioFormat, sizeof(u16Temp));
        if (letoh16(u16Temp) != Wave_AUDIO_FORMAT)
        {
            break;
        }
        memcpy(&u16Temp, header.channelCount, sizeof(u16Temp));
        if (letoh16(u16Temp) != Wave_CHANNEL_COUNT)
        {
            break;
        }
        memcpy(&u32Temp, header.sampleRate, sizeof(u32Temp));
        if (letoh32(u32Temp) != Wave_SAMPLE_RATE)
        {
            isHeaderValid = false;
            break;
        }
        memcpy(&u16Temp, header.bitsPerSample, sizeof(u16Temp));
        if (letoh16(u16Temp) != Wave_BITS_PER_SAMPLE)
        {
            isHeaderValid = false;
            break;
        }

        memcpy(&u32Temp, header.dataChunkSize, sizeof(u32Temp));
        (*sound)->len = letoh32(u32Temp);

        isHeaderValid = true;
        break;
    }
    if (!isHeaderValid)
    {
        sys_error("(resources) incompatible WAVE header for \"%s\"", resourceFiles[id]);
        return false;
    }
    return true;
}

/*
 *
 */
static void unloadSound(const unsigned id)
{
    sound_t ** sound;

    if (!fromResourceIdToSound(id, &sound))
    {
        return;
    }

    if (!*sound)
    {
        return;
    }

    sysmem_pop((*sound)->name);
    sysmem_pop(*sound);
    *sound = NULL;
}
#endif /* ENABLE_SOUND */


/*
 *
 */
static bool readHeader(file_t fp, const unsigned id)
{
    resource_header_t header;
    U16 u16Temp;

    if (sysfile_read(fp, &header, sizeof(header), 1) != 1)
    {
        sys_error("(resources) unable to read header from \"%s\"", resourceFiles[id]);
        return false;
    }

    if (memcmp(header.magic, resource_magic, sizeof(header.magic)) != 0)
    {
        sys_error("(resources) wrong header for \"%s\"", resourceFiles[id]);
        return false;
    }

    memcpy(&u16Temp, header.version, sizeof(u16Temp));
    u16Temp = htole16(u16Temp);
    if (u16Temp != DATA_VERSION)
    {
        sys_error("(resources) incompatible version for \"%s\"", resourceFiles[id]);
        return false;
    }

    memcpy(&u16Temp, header.resourceId, sizeof(u16Temp));
    u16Temp = htole16(u16Temp);
    if (u16Temp != id)
    {
        sys_error("(resources) mismatching ID for \"%s\"", resourceFiles[id]);
        return false;
    }
    return true;
}

/*
 *
 */
static bool checkCrc32(const unsigned id)
{
    int bytesRead;
    U8 tempBuffer[1024];
    U32 expectedCrc32, calculatedCrc32 = MZ_CRC32_INIT;

    file_t fp = sysfile_open(resourceFiles[id]);
    if (fp == NULL)
    {
        sys_error("(resources) unable to open \"%s\"", resourceFiles[id]);
        return false;
    }

    bytesRead = sysfile_read(fp, tempBuffer, sizeof(U32), 1); /* prepare beginning of buffer for the following loop */
    if (bytesRead != 1)
    {
        sys_error("(resources) not enough data for \"%s\"", resourceFiles[id]);
        sysfile_close(fp);
        return false;
    }
    do
    {
        bytesRead = sysfile_read(fp, tempBuffer + sizeof(U32), sizeof(U8), sizeof(tempBuffer) - sizeof(U32));

        calculatedCrc32 = mz_crc32(calculatedCrc32, tempBuffer, bytesRead);

        memcpy(tempBuffer, tempBuffer + bytesRead, sizeof(U32));
    } while (bytesRead == sizeof(tempBuffer) - sizeof(U32));

    sysfile_close(fp);

    memcpy(&expectedCrc32, tempBuffer, sizeof(U32));
    expectedCrc32 = letoh32(expectedCrc32);
    if (expectedCrc32 != calculatedCrc32)
    {
        sys_error("(resources) crc check failed for \"%s\"", resourceFiles[id]);
        return false;
    }
    return true;
}

/*
 *
 */
static bool readFile(const unsigned id)
{
    bool success;
    file_t fp;
    void * vp;

    switch (id)
    {
#ifndef GFXST
            case Resource_PICHAF: /* fallthrough */
            case Resource_PICCONGRATS: /* fallthrough */
            case Resource_PICSPLASH: return true;
#endif /* ndef GFXST */
#ifndef GFXPC
            case Resource_IMAINHOFT: /* fallthrough */
            case Resource_IMAINRDT: /* fallthrough */
            case Resource_IMAINCDC: /* fallthrough */
            case Resource_SCREENCONGRATS: return true;
#endif /* ndef GFXPC */
            default: break;
    }

    if (resourceFiles[id] == NULL)
    {
        sys_error("(resources) resource ID %d not available", id);
        return false;
    }

    if (!checkCrc32(id))
    {
        return false;
    }

    fp = sysfile_open(resourceFiles[id]);
    if (fp == NULL)
    {
        sys_error("(resources) unable to open \"%s\"", resourceFiles[id]);
        return false;
    }

    success = readHeader(fp, id);

    if (success)
    {
        switch (id)
        {
            case Resource_FILELIST: success = loadResourceFilelist(fp); break;
            case Resource_PALETTE:
            {
                vp = game_colors;
                success = loadRawData(fp, &vp, sizeof(*game_colors), &game_color_count);
                game_colors = vp;
                break;
            }
            case Resource_ENTDATA: success = loadResourceEntdata(fp); break;
            case Resource_SPRSEQ:
            {
                vp = ent_sprseq;
                success = loadRawData(fp, &vp, sizeof(*ent_sprseq), &ent_nbr_sprseq);
                ent_sprseq = vp;
                break;
            }
            case Resource_MVSTEP:
            {
                vp = ent_mvstep;
                success = loadRawData(fp, &vp, sizeof(*ent_mvstep), &ent_nbr_mvstep);
                ent_mvstep = vp;
                break;
            }
            case Resource_MAPS: success = loadResourceMaps(fp); break;
            case Resource_SUBMAPS: success = loadResourceSubmaps(fp); break;
            case Resource_CONNECT:
            {
                vp = map_connect;
                success = loadRawData(fp, &vp, sizeof(*map_connect), &map_nbr_connect);
                map_connect = vp;
                break;
            }
            case Resource_BNUMS:
            {
                vp = map_bnums;
                success = loadRawData(fp, &vp, sizeof(*map_bnums), &map_nbr_bnums);
                map_bnums = vp;
                break;
            }
            case Resource_BLOCKS:
            {
                vp = map_blocks;
                success = loadRawData(fp, &vp, sizeof(*map_blocks), &map_nbr_blocks);
                map_blocks = vp;
                break;
            }
            case Resource_MARKS:
            {
                vp = map_marks;
                success = loadRawData(fp, &vp, sizeof(*map_marks), &map_nbr_marks);
                map_marks = vp;
                break;
            }
            case Resource_EFLGC:
            {
                vp = map_eflg_c;
                success = loadRawData(fp, &vp, sizeof(*map_eflg_c), &map_nbr_eflgc);
                map_eflg_c = vp;
                break;
            }
            case Resource_IMAPSL:
            {
                vp = screen_imapsl;
                success = loadRawData(fp, &vp, sizeof(*screen_imapsl), &screen_nbr_imapsl);
                screen_imapsl = vp;
                break;
            }
            case Resource_IMAPSTEPS: success = loadResourceImapsteps(fp); break;
            case Resource_IMAPSOFS:
            {
                vp = screen_imapsofs;
                success = loadRawData(fp, &vp, sizeof(*screen_imapsofs), &screen_nbr_imapsofs);
                screen_imapsofs = vp;
                break;
            }
            case Resource_IMAPTEXT: success = loadResourceImaptext(fp); break;
            case Resource_GAMEOVERTXT: success = loadString(fp, (char **)(&screen_gameovertxt), 0xFE); break;
            case Resource_PAUSEDTXT: success = loadString(fp, (char **)(&screen_pausedtxt), 0xFE); break;
            case Resource_SPRITESDATA: success = loadResourceSpritesData(fp); break;
            case Resource_TILESDATA: success = loadResourceTilesData(fp); break;
            case Resource_HIGHSCORES: success = loadResourceHighScores(fp); break;
            case Resource_IMGSPLASH: success = loadImage(fp, &img_splash); break;
#ifdef GFXST
            case Resource_PICHAF: success = loadPicture(fp, &pic_haf); break;
            case Resource_PICCONGRATS: success = loadPicture(fp, &pic_congrats); break;
            case Resource_PICSPLASH: success = loadPicture(fp, &pic_splash); break;
#endif /* GFXST */
#ifdef GFXPC
            case Resource_IMAINHOFT: success = loadString(fp, (char **)(&screen_imainhoft), 0xFE); break;
            case Resource_IMAINRDT: success = loadString(fp, (char **)(&screen_imainrdt), 0xFE); break;
            case Resource_IMAINCDC: success = loadString(fp, (char **)(&screen_imaincdc), 0xFE); break;
            case Resource_SCREENCONGRATS: success = loadString(fp, (char **)(&screen_congrats), 0xFE); break;
#endif /* GFXPC */
            default: success = false; break;
        }
    }

    if (!success)
    {
        sys_error("(resources) error when parsing \"%s\"", resourceFiles[id]);
    }

    sysfile_close(fp);
    return success;
}

/*
 *
 */
bool resources_load()
{
    bool success;
    unsigned id;

    /* note: loading order is important: file list first, then sounds, then the rest */

    success = readFile(Resource_FILELIST);

#ifdef ENABLE_SOUND
    for (id = Resource_SOUNDBOMBSHHT; (id <= Resource_SOUNDWALK) && success; ++id)
    {
        success = loadSound(id);
    }
#endif /* ENABLE_SOUND */

    for (id = Resource_PALETTE; (id <= Resource_SCREENCONGRATS) && success; ++id)
    {
        success = readFile(id);
    }
    return success;
}

/*
 *
 */
void resources_unload()
{
    int id;
    void * vp;

    for (id = Resource_SCREENCONGRATS; id >= Resource_PALETTE; --id)
    {
        switch (id)
        {
            case Resource_PALETTE:
            {
                vp = game_colors;
                unloadRawData(&vp, &game_color_count);
                game_colors = vp;
                break;
            }
            case Resource_ENTDATA: unloadResourceEntdata(); break;
            case Resource_SPRSEQ:
            {
                vp = ent_sprseq;
                unloadRawData(&vp, &ent_nbr_sprseq);
                ent_sprseq = vp;
                break;
            }
            case Resource_MVSTEP:
            {
                vp = ent_mvstep;
                unloadRawData(&vp, &ent_nbr_mvstep);
                ent_mvstep = vp;
                break;
            }
            case Resource_MAPS: unloadResourceMaps(); break;
            case Resource_SUBMAPS: unloadResourceSubmaps(); break;
            case Resource_CONNECT:
            {
                vp = map_connect;
                unloadRawData(&vp, &map_nbr_connect);
                map_connect = vp;
                break;
            }
            case Resource_BNUMS:
            {
                vp = map_bnums;
                unloadRawData(&vp, &map_nbr_bnums);
                map_bnums = vp;
                break;
            }
            case Resource_BLOCKS:
            {
                vp = map_blocks;
                unloadRawData(&vp, &map_nbr_blocks);
                map_blocks = vp;
                break;
            }
            case Resource_MARKS:
            {
                vp = map_marks;
                unloadRawData(&vp, &map_nbr_marks);
                map_marks = vp;
                break;
            }
            case Resource_EFLGC:
            {
                vp = map_eflg_c;
                unloadRawData(&vp, &map_nbr_eflgc);
                map_eflg_c = vp;
                break;
            }
            case Resource_IMAPSL:
            {
                vp = screen_imapsl;
                unloadRawData(&vp, &screen_nbr_imapsl);
                screen_imapsl = vp;
                break;
            }
            case Resource_IMAPSTEPS: unloadResourceImapsteps(); break;
            case Resource_IMAPSOFS:
            {
                vp = screen_imapsofs;
                unloadRawData(&vp, &screen_nbr_imapsofs);
                screen_imapsofs = vp;
                break;
            }
            case Resource_IMAPTEXT: unloadResourceImaptext(); break;
            case Resource_GAMEOVERTXT: unloadString((char **)(&screen_gameovertxt)); break;
            case Resource_PAUSEDTXT: unloadString((char **)(&screen_pausedtxt)); break;
            case Resource_SPRITESDATA: unloadResourceSpritesData(); break;
            case Resource_TILESDATA: unloadResourceTilesData(); break;
            case Resource_HIGHSCORES: unloadResourceHighScores(); break;
            case Resource_IMGSPLASH: unloadImage(&img_splash); break;
#ifdef GFXST
            case Resource_PICHAF: unloadPicture(&pic_haf); break;
            case Resource_PICCONGRATS: unloadPicture(&pic_congrats); break;
            case Resource_PICSPLASH: unloadPicture(&pic_splash); break;
#endif /* GFXST */
#ifdef GFXPC
            case Resource_IMAINHOFT: unloadString((char **)(&screen_imainhoft)); break;
            case Resource_IMAINRDT: unloadString((char **)(&screen_imainrdt)); break;
            case Resource_IMAINCDC: unloadString((char **)(&screen_imaincdc)); break;
            case Resource_SCREENCONGRATS: unloadString((char **)(&screen_congrats)); break;
#endif /* GFXPC */
            default: break;
        }
    }

#ifdef ENABLE_SOUND
    for (id = Resource_SOUNDWALK; id >= Resource_SOUNDBOMBSHHT; --id)
    {
        unloadSound(id);
    }
#endif /* ENABLE_SOUND */

    unloadResourceFilelist();
}

/* eof */
