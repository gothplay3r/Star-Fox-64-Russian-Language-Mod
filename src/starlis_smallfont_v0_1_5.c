#include "modding.h"

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;
typedef float f32;

typedef struct OSContPad {
    unsigned short button;
    signed char stick_x;
    signed char stick_y;
    unsigned char errno;
} OSContPad;

typedef struct Gfx {
    u32 words[2];
} Gfx;

extern Gfx* gMasterDisp;
extern OSContPad gControllerPress[4];
extern s32 gMainController;
extern s32 gGameFrameCount;
extern s32 gGameState;
extern s32 sCurrentPlanetId;
extern s32 sMapState;
extern s32 gRadioState;
extern s32 gHideRadio;
extern s32 gCurrentRadioPortrait;
extern f32 gRadioMsgRadioId;
extern s32 gTeamShields[8];
extern s32 gTeamHelpTimer;
extern void* gTeamHelpActor;
extern s32 gVsMatchType;
extern s32 D_800D4A98;
extern s32 sVsWinner;
extern void RCP_SetupDL(Gfx** gfx_ptr, s32 setup_dl);
extern void Lib_TextureRect_IA8(Gfx** gfx_ptr, u8* texture, u32 width, u32 height, f32 x_pos, f32 y_pos, f32 x_scale,
                                f32 y_scale);
extern char sSmallChars[];
extern u8* sSmallCharTex[];

#include "small_font_ranking_labels.ia8.inc"

#define SF64_RU_LABEL_W(name) ((sizeof(name) / sizeof(name[0])) / 12)
#define SF64_RU_DRAW_LABEL_MASK(name, x_pos, y_pos) \
    Lib_TextureRect_IA8(&gMasterDisp, (u8*) name, SF64_RU_LABEL_W(name), 12, x_pos, y_pos, 1.0f, 1.0f)

static u8 sf64_ru_ranking_header_cover_ia8[144 * 12];
static u8 sf64_ru_small_cover_ia8[96 * 12];
static u8 sf64_ru_levelselect_cover_ia8[192 * 12];
static u8 sf64_ru_levelselect_wide_cover_ia8[192 * 24];
static s32 sf64_ru_levelselect_mission = 0;
static s32 sf64_ru_levelselect_difficulty = 0;
static s32 sf64_ru_levelselect_start_option = 0;

static void sf64_ru_set_prim_color(u32 rgba) {
    Gfx* command = gMasterDisp++;

    command->words[0] = 0xFA000000u;
    command->words[1] = rgba;
}

static void sf64_ru_fill_cover(u8* texture, s32 byte_count) {
    s32 i;

    for (i = 0; i < byte_count; i++) {
        texture[i] = 0xFF;
    }
}

static void sf64_ru_draw_black_cover(s32 width, s32 height, f32 x_pos, f32 y_pos) {
    sf64_ru_fill_cover(sf64_ru_small_cover_ia8, sizeof(sf64_ru_small_cover_ia8));
    sf64_ru_set_prim_color(0x000000FFu);
    Lib_TextureRect_IA8(&gMasterDisp, sf64_ru_small_cover_ia8, width, height, x_pos, y_pos, 1.0f, 1.0f);
}

static void sf64_ru_draw_outlined_label_scaled(u8* texture, u32 width, f32 x_pos, f32 y_pos, f32 x_scale, f32 y_scale) {
    sf64_ru_set_prim_color(0x000000FFu);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos - 1.0f, y_pos - 1.0f, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos, y_pos - 1.0f, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos + 1.0f, y_pos - 1.0f, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos - 1.0f, y_pos, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos + 1.0f, y_pos, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos - 1.0f, y_pos + 1.0f, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos, y_pos + 1.0f, x_scale, y_scale);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos + 1.0f, y_pos + 1.0f, x_scale, y_scale);
    sf64_ru_set_prim_color(0xFFFF00FFu);
    Lib_TextureRect_IA8(&gMasterDisp, texture, width, 12, x_pos, y_pos, x_scale, y_scale);
}

static void sf64_ru_draw_outlined_label(u8* texture, u32 width, f32 x_pos, f32 y_pos) {
    sf64_ru_draw_outlined_label_scaled(texture, width, x_pos, y_pos, 1.0f, 1.0f);
}

#define SF64_RU_DRAW_LABEL_OUTLINED_IA8(name, x_pos, y_pos) \
    sf64_ru_draw_outlined_label((u8*) name, SF64_RU_LABEL_W(name), x_pos, y_pos)

static void sf64_ru_draw_levelselect_cover(void) {
    sf64_ru_fill_cover(sf64_ru_levelselect_wide_cover_ia8, sizeof(sf64_ru_levelselect_wide_cover_ia8));
    sf64_ru_set_prim_color(0x000000FFu);
    Lib_TextureRect_IA8(&gMasterDisp, sf64_ru_levelselect_wide_cover_ia8, 192, 24, 18.0f, 224.0f, 1.0f, 1.0f);
}

static void sf64_ru_draw_levelselect_planet_name(void) {
    switch (sCurrentPlanetId) {
        case 0:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_meteo_ia8, 88.0f, 226.0f);
            break;
        case 1:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_zona_6_ia8, 88.0f, 226.0f);
            break;
        case 2:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_bols_ia8, 88.0f, 226.0f);
            break;
        case 3:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_sektor_z_ia8, 88.0f, 226.0f);
            break;
        case 4:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_sektor_x_ia8, 88.0f, 226.0f);
            break;
        case 5:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_sektor_y_ia8, 88.0f, 226.0f);
            break;
        case 6:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_katina_ia8, 88.0f, 226.0f);
            break;
        case 7:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_makbet_ia8, 88.0f, 226.0f);
            break;
        case 8:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_zoness_ia8, 88.0f, 226.0f);
            break;
        case 9:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_korneriya_ia8, 88.0f, 226.0f);
            break;
        case 10:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_titaniya_ia8, 88.0f, 226.0f);
            break;
        case 11:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_akvas_ia8, 88.0f, 226.0f);
            break;
        case 12:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_fortuna_ia8, 88.0f, 226.0f);
            break;
        case 13:
            if ((sf64_ru_levelselect_mission == 6) && (sf64_ru_levelselect_difficulty == 2)) {
                SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_venom_2_ia8, 88.0f, 226.0f);
            } else {
                SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_venom_1_ia8, 88.0f, 226.0f);
            }
            break;
        case 14:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_solar_ia8, 88.0f, 226.0f);
            break;
    }
}

static void sf64_ru_update_levelselect_tracker(void) {
    unsigned short button;

    if ((gMainController < 0) || (gMainController >= 4)) {
        return;
    }

    button = gControllerPress[gMainController].button;

    if (button & 0x0200) {
        sf64_ru_levelselect_mission--;
        if (sf64_ru_levelselect_mission < 0) {
            sf64_ru_levelselect_mission = 6;
        }
        sf64_ru_levelselect_start_option = 0;
    } else if (button & 0x0100) {
        sf64_ru_levelselect_mission++;
        if (sf64_ru_levelselect_mission > 6) {
            sf64_ru_levelselect_mission = 0;
        }
        sf64_ru_levelselect_start_option = 0;
    } else if ((button & 0x0800) && (sf64_ru_levelselect_mission != 0)) {
        sf64_ru_levelselect_difficulty++;
        if (sf64_ru_levelselect_difficulty > 2) {
            sf64_ru_levelselect_difficulty = 0;
        }
        if ((sf64_ru_levelselect_difficulty == 1) &&
            ((sf64_ru_levelselect_mission == 1) || (sf64_ru_levelselect_mission == 5) ||
             (sf64_ru_levelselect_mission == 6))) {
            sf64_ru_levelselect_difficulty = 2;
        }
        sf64_ru_levelselect_start_option = 0;
    } else if ((button & 0x0400) && (sf64_ru_levelselect_mission != 0)) {
        sf64_ru_levelselect_difficulty--;
        if ((sf64_ru_levelselect_difficulty != 2) &&
            ((sf64_ru_levelselect_mission == 1) || (sf64_ru_levelselect_mission == 5) ||
             (sf64_ru_levelselect_mission == 6))) {
            sf64_ru_levelselect_difficulty--;
        }
        if (sf64_ru_levelselect_difficulty < 0) {
            sf64_ru_levelselect_difficulty = 2;
        }
        sf64_ru_levelselect_start_option = 0;
    }

    if (button & 0x0020) {
        sf64_ru_levelselect_start_option ^= 1;
    }
}

static void sf64_ru_draw_levelselect_second_line(void) {
    if (!sf64_ru_levelselect_start_option) {
        return;
    }

    if ((sCurrentPlanetId == 4) || (sCurrentPlanetId == 0)) {
        SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_varp_zona_ia8, 88.0f, 236.0f);
    } else if (sCurrentPlanetId == 13) {
        SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_andross_ia8, 88.0f, 236.0f);
    } else if (sCurrentPlanetId == 1) {
        SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_beta_sb_ia8, 88.0f, 236.0f);
    }
}

RECOMP_HOOK_RETURN("Title_RankingData_Draw")
void sf64_ru_after_title_ranking_data_draw(void) {
    sf64_ru_fill_cover(sf64_ru_ranking_header_cover_ia8, sizeof(sf64_ru_ranking_header_cover_ia8));

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_set_prim_color(0x000000FFu);
    Lib_TextureRect_IA8(&gMasterDisp, sf64_ru_ranking_header_cover_ia8, 144, 12, 88.0f, 36.0f, 1.0f, 1.0f);

    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_mesto_ia8, 90.0f, 36.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_imya_ia8, 150.0f, 36.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_sbito_ia8, 190.0f, 36.0f);
}

RECOMP_HOOK_RETURN("Option_RankingMenu_Draw")
void sf64_ru_after_option_ranking_menu_draw(void) {
    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_draw_black_cover(80, 12, 232.0f, 213.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_nazhmi_a_ia8, 232.0f, 213.0f);
}

RECOMP_HOOK_RETURN("Map_TotalHits_Draw")
void sf64_ru_after_map_total_hits_draw(void) {
    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_draw_black_cover(88, 10, 20.0f, 12.0f);
    sf64_ru_draw_black_cover(36, 10, 140.0f, 12.0f);

    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_itogo_ia8, 24.0f, 14.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_top_ia8, 143.0f, 14.0f);
}

RECOMP_HOOK_RETURN("Map_Update")
void sf64_ru_after_map_level_select(void) {
    if ((sMapState != 3) || (sCurrentPlanetId < 0) || (sCurrentPlanetId >= 15)) {
        return;
    }

    sf64_ru_update_levelselect_tracker();

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_draw_levelselect_cover();

    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_planeta_ia8, 20.0f, 226.0f);
    sf64_ru_draw_levelselect_planet_name();
    sf64_ru_draw_levelselect_second_line();
}

RECOMP_HOOK_RETURN("func_versus_800C04DC")
void sf64_ru_after_versus_total_score(void) {
    if ((gVsMatchType != 2) || ((D_800D4A98 & 0x20) == 0) || (sVsWinner == 99)) {
        return;
    }

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_draw_black_cover(96, 12, 116.0f, 108.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_itog_ia8, 118.0f, 110.0f);
    SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_schet_ia8, 163.0f, 110.0f);
}


static s32 sf64_ru_streq(const char* a, const char* b) {
    while ((*a != 0) && (*b != 0)) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == 0) && (*b == 0);
}

static void sf64_ru_display_original_small_text(s32 xPos, s32 yPos, f32 xScale, f32 yScale, char* text) {
    u32 char_index;
    f32 x_pos_current = xPos;
    s32 width;

    while (text[0] != 0) {
        char_index = 0;
        while ((sSmallChars[char_index] != 0) && (sSmallChars[char_index] != text[0])) {
            char_index++;
        }
        if (sSmallChars[char_index] == text[0]) {
            if (sSmallCharTex[char_index] != 0) {
                width = 8;
                if (char_index > 30) {
                    width = 16;
                }
                Lib_TextureRect_IA8(&gMasterDisp, sSmallCharTex[char_index], width, 8, x_pos_current, yPos, xScale, yScale);
            }
            switch (text[0]) {
                case '!':
                case ':':
                case 'I':
                    x_pos_current += 4.0f * xScale;
                    break;
                case '-':
                    x_pos_current += 6.0f * xScale;
                    break;
                default:
                    if (char_index >= 30) {
                        x_pos_current += 9.0f * xScale;
                    } else {
                        x_pos_current += 8.0f * xScale;
                    }
                    break;
            }
        }
        text++;
    }
}

RECOMP_PATCH void Graphics_DisplaySmallText(s32 xPos, s32 yPos, f32 xScale, f32 yScale, char* text) {
    f32 help_x;

    if (sf64_ru_streq(text, "HELP!!")) {
        help_x = (xPos > 160) ? (xPos - 10.0f) : xPos;
        SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_pomogi_ia8, help_x, yPos);
        return;
    }

    if (sf64_ru_streq(text, "DOWN")) {
        sf64_ru_draw_outlined_label_scaled((u8*) sf64_ru_label_sbit_ia8, SF64_RU_LABEL_W(sf64_ru_label_sbit_ia8),
                                          xPos + 4.0f, yPos - 1.0f, 0.86f, 0.86f);
        return;
    }

    sf64_ru_display_original_small_text(xPos, yPos, xScale, yScale, text);
}

RECOMP_PATCH void HUD_RadioCharacterName_Draw(void) {
    s32 radio_id;

    if (gGameState != 7) {
        return;
    }

    radio_id = gRadioMsgRadioId;
    RCP_SetupDL(&gMasterDisp, 76);
    switch (radio_id) {
        case 0:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_fox_ia8, 73.0f, 171.0f);
            break;
        case 10:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_falco_ia8, 73.0f, 171.0f);
            break;
        case 20:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_slippy_ia8, 73.0f, 171.0f);
            break;
        case 30:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_peppy_ia8, 73.0f, 171.0f);
            break;
        case 40:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_katt_ia8, 73.0f, 171.0f);
            break;
        case 170:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_bill_ia8, 73.0f, 171.0f);
            break;
        case 200:
        case 240:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_wolf_ia8, 73.0f, 171.0f);
            break;
        case 210:
        case 250:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_pigma_ia8, 73.0f, 171.0f);
            break;
        case 220:
        case 260:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_leon_ia8, 73.0f, 171.0f);
            break;
        case 230:
        case 270:
            SF64_RU_DRAW_LABEL_OUTLINED_IA8(sf64_ru_label_andrew_ia8, 73.0f, 171.0f);
            break;
    }
}
