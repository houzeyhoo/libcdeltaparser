//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//
// libcdeltaparser, copyright (c) 2021 houz                                                              	    //
// A parser-library for Growtopia's items.dat, written fully in C & embeddable in C/C++ code.                   //
// For license, please read license.txt included in the project repo.                                           //
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-//

#ifndef LIBCDELTAPARSER_H
#define LIBCDELTAPARSER_H

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define _ITEMSDAT_MAX_VERSION 12 // Maximum items.dat version supported by this parser
#define _ITEMSDAT_SECRET_LEN 16  // Length of the secret string used for name decryption

// POD item object
struct item
{
    int id;
    unsigned short properties;
    unsigned char type;
    char material;
    // Warning: name field in DB v2 is plaintext, v3 and forwards is encrypted using XOR with a shared key.
    char* name;
    char* file_name;
    int file_hash;
    char visual_type;
    int cook_time;
    char tex_x;
    char tex_y;
    char storage_type;
    char layer;
    char collision_type;
    char hardness;
    int regen_time;
    char clothing_type;
    short rarity;
    unsigned char max_hold;
    char* alt_file_path;
    int alt_file_hash;
    int anim_ms;

    // DB v4 - Pet name/prefix/suffix
    char* pet_name;
    char* pet_prefix;
    char* pet_suffix;

    // DB v5 - Pet ability
    char* pet_ability;

    // Back to normal
    char seed_base;
    char seed_over;
    char tree_base;
    char tree_over;
    int bg_col;
    int fg_col;

    // These two seed values are always zero and they're unused.
    short seed1;
    short seed2;

    int bloom_time;

    // DB v7 - animation type.
    int anim_type;

    // DB v8 - animation stuff continued.
    char* anim_string;
    char* anim_tex;
    char* anim_string2;
    int dlayer1;
    int dlayer2;

    // DB v9 - second properties enum, ran out of space for first.
    unsigned short properties2;

    // DB v10 - tile/pile range - used by extractors and such.
    int tile_range;
    int pile_range;

    // DB v11 - custom punch, a failed experiment that doesn't seem to work ingame.
    char* custom_punch;

    // DB v12 - some flags at end of item
};

struct itemsdat
{
    short version;
    int item_count;

    struct item* items;
};

//_read_byte - reads 1 byte from buf, copies to dest. Increments bp (buffer position) by 1.
static inline void _read_byte(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 1);
    *bp += 1;
}

//_read_short - reads 2 bytes from buf, copies to dest. Increments bp (buffer position) by 2.
static inline void _read_short(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 2);
    *bp += 2;
}

//_read_int32 - reads 4 bytes from buf, copies to dest. Increments bp (buffer position) by 4.
static inline void _read_int32(char* buf, unsigned int* bp, void* dest)
{
    memcpy(dest, buf + *bp, 4);
    *bp += 4;
}

//_read_str - reads chars from buf, copies to *dest. Increments bp by string size.
static inline void _read_str(char* buf, unsigned int* bp, char** dest)
{
    unsigned int bp_in = *bp;
    unsigned short len = 0;
    _read_short(buf, bp, &len);
    *dest = (char*)malloc(len + 1); //+terminator

    memset(*dest + len, '\0', 1);
    strncpy(*dest, buf + *bp, len);
    *bp += len;
}

//_read_str - reads chars from buf, copies to *dest. Increments bp by string size. Used for encrypted item names.
static inline void _read_str_encr(char* buf, unsigned int* bp, char** dest, int itemID)
{
    unsigned int bp_in = *bp;
    // Reverse of MemorySerializeStringEncrypted from Proton SDK
    static const char* SECRET = "PBG892FXX982ABC*";
    static unsigned short secret_len = _ITEMSDAT_SECRET_LEN;

    unsigned short len = 0;
    _read_short(buf, bp, &len);
    *dest = (char*)malloc(len + 1); //+terminator
    memset(*dest + len, '\0', 1);

    itemID %= secret_len;
    for (unsigned int i = 0; i < len; i++)
    {
        char y = buf[*bp + i] ^ SECRET[itemID++];
        memcpy(*dest + i, &y, 1);
        if (itemID >= secret_len) itemID = 0;
    }
    *bp += len;
}

//Parses itemsdat. You need to pass an initialized FILE* structure to it and an itemsdat structure aswell.
//Return codes: 0 - all is good,  1 - invalid FILE* structure, 2 - invalid items.dat (bad version), 3 - invalid items.dat (parse cond i==item_id failed)
//WARNING: YOU MUST FREE THE ITEMSDAT->ITEMS ARRAY ONCE YOU'RE DONE WITH IT!
static inline int parse_itemsdat(FILE* f, struct itemsdat* itemsdat)
{
    if (f == NULL) return 1;
    if (ferror(f)) return 1;

    fseek(f, 0, SEEK_END);
    unsigned int file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc(file_size);
    unsigned int buf_pos = 0;
    fread(buf, 1, file_size, f);

    _read_short(buf, &buf_pos, &itemsdat->version);
    _read_int32(buf, &buf_pos, &itemsdat->item_count);

    if (itemsdat->version > _ITEMSDAT_MAX_VERSION)
    {
        free(buf);
        return 2;
    }
    itemsdat->items = (struct item*)calloc(itemsdat->item_count, sizeof(struct item));

    for (int i = 0; i < itemsdat->item_count; i++)
    {
        struct item item = { 0 };
        _read_int32(buf, &buf_pos, &item.id);

        if (item.id != i)
        {
            free(buf);
            return 3;
        }
        _read_short(buf, &buf_pos, &item.properties);
        _read_byte(buf, &buf_pos, &item.type);
        _read_byte(buf, &buf_pos, &item.material);

        if (itemsdat->version >= 3)
            _read_str_encr(buf, &buf_pos, &item.name, item.id); //encrypted on itemsdat ver > 3
        else
            _read_str(buf, &buf_pos, &item.name);

        _read_str(buf, &buf_pos, &item.file_name);
        _read_int32(buf, &buf_pos, &item.file_hash);
        _read_byte(buf, &buf_pos, &item.visual_type);
        _read_int32(buf, &buf_pos, &item.cook_time);
        _read_byte(buf, &buf_pos, &item.tex_x);
        _read_byte(buf, &buf_pos, &item.tex_y);
        _read_byte(buf, &buf_pos, &item.storage_type);
        _read_byte(buf, &buf_pos, &item.layer);
        _read_byte(buf, &buf_pos, &item.collision_type);
        _read_byte(buf, &buf_pos, &item.hardness);
        _read_int32(buf, &buf_pos, &item.regen_time);
        _read_byte(buf, &buf_pos, &item.clothing_type);
        _read_short(buf, &buf_pos, &item.rarity);
        _read_byte(buf, &buf_pos, &item.max_hold);
        _read_str(buf, &buf_pos, &item.alt_file_path);
        _read_int32(buf, &buf_pos, &item.alt_file_hash);
        _read_int32(buf, &buf_pos, &item.anim_ms);

        if (itemsdat->version >= 4)
        {
            _read_str(buf, &buf_pos, &item.pet_name);
            _read_str(buf, &buf_pos, &item.pet_prefix);
            _read_str(buf, &buf_pos, &item.pet_suffix);

            if (itemsdat->version >= 5)
                _read_str(buf, &buf_pos, &item.pet_ability);
        }

        _read_byte(buf, &buf_pos, &item.seed_base);
        _read_byte(buf, &buf_pos, &item.seed_over);
        _read_byte(buf, &buf_pos, &item.tree_base);
        _read_byte(buf, &buf_pos, &item.tree_over);
        _read_int32(buf, &buf_pos, &item.bg_col);
        _read_int32(buf, &buf_pos, &item.fg_col);
        _read_short(buf, &buf_pos, &item.seed1);
        _read_short(buf, &buf_pos, &item.seed2);
        _read_int32(buf, &buf_pos, &item.bloom_time);

        if (itemsdat->version >= 7)
        {
            _read_int32(buf, &buf_pos, &item.anim_type);
            _read_str(buf, &buf_pos, &item.anim_string);
        }
        if (itemsdat->version >= 8)
        {
            _read_str(buf, &buf_pos, &item.anim_tex);
            _read_str(buf, &buf_pos, &item.anim_string2);
            _read_int32(buf, &buf_pos, &item.dlayer1);
            _read_int32(buf, &buf_pos, &item.dlayer2);
        }
        if (itemsdat->version >= 9)
        {
            _read_short(buf, &buf_pos, &item.properties2);
            buf_pos += 62; //skip irrelevant client data
        }
        if (itemsdat->version >= 10)
        {
            _read_int32(buf, &buf_pos, &item.tile_range);
            _read_int32(buf, &buf_pos, &item.pile_range);
        }
        if (itemsdat->version >= 11)
        {
            _read_str(buf, &buf_pos, &item.custom_punch);
        }
        if (itemsdat->version >= 12)
        {
            buf_pos += 13; //skip more useless data
        }

        itemsdat->items[i] = item;
    }
    free(buf);
    return 0;
}
#endif /* LIBCDELTAPARSER_H */
