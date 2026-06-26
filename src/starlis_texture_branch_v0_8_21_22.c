#include "modding.h"

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;
typedef float f32;

#define OS_PHYSICAL_TO_K0(address) ((void*) (0x80000000u + (address)))
#define SF64_RU_ARRAY_COUNT(array) ((s32) (sizeof(array) / sizeof((array)[0])))

#include "aMainGameCardTex.ia8.inc"
#include "aTrainingCardTex.ia8.inc"
#include "aVsCardTex.ia8.inc"
#include "aRankingCardTex.ia8.inc"
#include "aSoundCardTex.ia8.inc"
#include "aDataCardTex.ia8.inc"
#include "aExpertCardTex.ia8.inc"
#include "aOptPressRToTestTex.ia8.inc"
#include "aOptSoundModeTex.ia8.inc"
#include "aOptMusicVolumeTex.ia8.inc"
#include "aOptVoiceVolumeTex.ia8.inc"
#include "aBtoCancelTex.ia8.inc"
#include "aOptHeadphoneTex.ia8.inc"
#include "aOptMonoTex.ia8.inc"
#include "aOptStereoTex.ia8.inc"
#include "aOptSfxVolumeTex.expanded.ia8.inc"
#include "aAtoConfirmTex.ia8.inc"
#include "aOptClearSaveDataTex.ia8.inc"
#include "aOptAreYouSureTex.ia8.inc"
#include "aOptYesTex.ia8.inc"
#include "aOptNoTex.ia8.inc"
#include "aOptAreYouREALLYSureTex.ia8.inc"
#include "aOptDataErasedTex.ia8.inc"
#include "aMissionNoFull1.ia8.inc"
#include "aMissionNoFull2.ia8.inc"
#include "aMissionNoFull3.ia8.inc"
#include "aMissionNoFull4.ia8.inc"
#include "aMissionNoFull5.ia8.inc"
#include "aMissionNoFull6.ia8.inc"
#include "aMissionNoFull7.ia8.inc"
#include "aCoTitleCardTex.ia8.inc"
#include "aMeTitleCardTex.ia8.inc"
#include "aKaTitleCardTex.ia8.inc"
#include "aAqTitleCardTex.ia8.inc"
#include "aSyTitleCardTex.ia8.inc"
#include "aSxTitleCardTex.ia8.inc"
#include "aSoTitleCardTex.ia8.inc"
#include "aZoTitleCardTex.ia8.inc"
#include "aTiTitleCardTex.ia8.inc"
#include "aMaTitleCardTex.ia8.inc"
#include "aSzTitleCardTex.ia8.inc"
#include "aBoTitleCardTex.ia8.inc"
#include "aA6TitleCardTex.ia8.inc"
#include "aVeTitleCardTex.ia8.inc"
#include "aVe1TitleCardTex.ia8.inc"
#include "aVe2TitleCardTex.ia8.inc"
#include "aFoTitleCardTex.ia8.inc"
#include "aMapCoTitleCardTex.ia8.inc"
#include "aMapLylatSystemCardTex.ia8.inc"
#include "aMapMissionNo1Full.ia8.inc"
#include "aMapMeLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapSyLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapFoLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapKaLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapAqLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapSxLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapSoLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapZoLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapTiLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapMaLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapSzLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapBoLowerMissionTitle_208x19_ru.ia8.inc"
#include "aMapA6LowerMissionTitle_208x19_ru.ia8.inc"
#include "aTitleRuPressStart.ia8.inc"
#include "aTitleRuCopyrightBlock.ia8.inc"
#include "important_ui_native.ia8.inc"
#include "aTextPeppyRuNative48x10Align1.ia8.inc"
#include "aTextSlippyRuNative56x10Probe1.ia8.inc"
#include "aTextFalcoRuNative48x8Probe1.ia8.inc"
#include "aFalcoMarkManual10x8Probe3.inc"
#include "aFalcoMarkManual10x8FillFixProbe4.inc"
#include "aFalcoMarkManual10x9Probe5.inc"
#include "aFalcoMarkManual10x9Variant2Probe6.inc"
#include "aFalcoMarkManual10x9GrayProbe7.inc"
#include "aFalcoMarkManual10x9GrayFillFixProbe8.inc"
#include "aPeppyMarkManual10x9Probe1.inc"
#include "aPeppyMarkManual8x9Probe2.inc"
#include "aPeppyMarkManual8x8Probe3.inc"
#include "aPeppyMarkManual7x8Probe4.inc"
#include "aSlippyMarkManual8x8Probe1.inc"
#include "aBillMarkManual7x7Probe1.inc"
#include "aJamesMarkManual8x7Probe1.inc"
#include "aFalcoMarkOriginalPaletteFInsideCi4Probe1.inc"
#include "aBossLabelBossRu32x7.ci4.inc"
#include "aSzMissileMarkRu32x16_fix10_lifted.ci4.inc"
#include "invoice_tex_01_02_palette_ru.ci4.inc"
#include "invoice_tex_01_title_ru.ci4.inc"
#include "invoice_tex_02_header_ru.ci4.inc"
#include "invoice_tex_03_palette_ru.ci4.inc"
#include "invoice_tex_03_amount_ru.ci4.inc"
#include "aCheckpointTextRu_pageA_64x16.ci4.inc"
#include "aCheckpointTextRu_pageB_64x16.ci4.inc"
#include "aCheckpointTextRu_tlutA_8.rgba16bytes.inc"
#include "aCheckpointTextRu_tlutB_8.rgba16bytes.inc"

typedef struct Gfx {
    u32 words[2];
} Gfx;

typedef struct Sf64OptionCardTexture {
    s32 type;
    s32 unused;
    const u8* texture;
    void* palette;
    s32 width;
    s32 height;
    f32 x_pos;
    f32 y_pos;
    f32 x_scale;
    f32 y_scale;
    s32 red;
    s32 green;
    s32 blue;
    s32 alpha;
} Sf64OptionCardTexture;

typedef struct Sf64OptionEntry {
    Sf64OptionCardTexture tex;
    u8 rest[0x28];
} Sf64OptionEntry;

typedef struct Sf64RuMenuCard {
    u32 segmented_address;
    const u8* replacement;
    s32 byte_count;
    s32 width;
    s32 height;
    f32 x_pos;
    f32 y_pos;
} Sf64RuMenuCard;

typedef struct Sf64RuTexture {
    u32 segmented_address;
    const u8* replacement;
    s32 byte_count;
} Sf64RuTexture;

typedef struct Sf64LevelTitleCard {
    u8* levelIdxTex;
    s32 width;
    s32 height;
    u8* titleCardTex;
    s32 titleCardWidth;
    s32 titleCardHeight;
} Sf64LevelTitleCard;

typedef struct Sf64MapTitleCard {
    u8* texture;
    s32 width;
    s32 height;
    f32 x_pos;
} Sf64MapTitleCard;

extern Sf64OptionEntry sOptionCardList[6];
extern Sf64LevelTitleCard sLevelTitleCard[];
extern Sf64MapTitleCard sPlanetNameCards[14];
extern Sf64MapTitleCard sPlanetTitleCards[14];
extern u32 gSegments[16];
extern f32 sOptionCardTextPosX[6];
extern f32 sOptionCardTextPosY[6];
extern f32 sOptionCardCurTextPosX[6];
extern f32 sOptionCardCurTextPosY[6];
extern Gfx* gMasterDisp;
extern f32 D_menu_801B9270[4];
extern s32 sExpertModeCursor;
extern s32 sExpertSoundCursor;
extern s32 sMainMenuState;
extern s32 gSceneId;
extern s32 gCurrentLevel;
extern s32 gMainController;
extern u32 gControllerLock;
extern f32 sTitleTextPrimCol;
extern s32 gLevelClearScreenTimer;

extern void RCP_SetupDL(Gfx** gfx_ptr, s32 setup_dl);
extern void Lib_TextureRect_IA8(Gfx** gfx_ptr, u8* texture, u32 width, u32 height, f32 x_pos, f32 y_pos, f32 x_scale,
                                f32 y_scale);
extern s32 HUD_GetLevelIndex(void);

static void sf64_ru_set_prim_color(u32 rgba);

static const u8 sf64_ru_blank_sfx_label_ia8[24 * 13] = { 0 };
static const u8 sf64_ru_blank_title_press_start_ia8[120 * 13] = { 0 };
static const u8 sf64_ru_blank_title_1997_nintendo_ia8[120 * 12] = { 0 };
static const u8 sf64_ru_blank_mission_prefix_ia8[112 * 19] = { 0 };
static const u8 sf64_ru_blank_mission_digit_ia8[16 * 15] = { 0 };
static u8 sf64_ru_mission_header_cover_ia8[160 * 28];
static f32 sf64_ru_last_title_card_x;
static f32 sf64_ru_last_title_card_y;
static s32 sf64_ru_title_card_overlay_pending;
static s32 sf64_ru_level_id_katina = -1;
static s32 sf64_ru_level_id_aquas = -1;
static s32 sf64_ru_level_id_sector_y = -1;
static s32 sf64_ru_level_id_sector_x = -1;
static s32 sf64_ru_level_id_solar = -1;
static s32 sf64_ru_level_id_zoness = -1;
static s32 sf64_ru_level_id_titania = -1;
static s32 sf64_ru_level_id_macbeth = -1;
static s32 sf64_ru_level_id_sector_z = -1;
static s32 sf64_ru_level_id_bolse = -1;
static s32 sf64_ru_level_id_area6 = -1;
#define SF64_RU_CURRENT_LEVEL_VENOM1 6
#define SF64_RU_CURRENT_LEVEL_VENOM2 19

static s32 sf64_ru_level_id_venom1 = -1;
static s32 sf64_ru_level_id_venom2 = -1;
static s32 sf64_ru_venom_saved_current_level = 0;
static s32 sf64_ru_venom_suppress_upper_pending = 0;
static s32 sf64_ru_venom_upper_ru_pending = 0;

static const Sf64RuMenuCard sf64_ru_menu_cards[6] = {
    { 0x08003B50u, sf64_ru_main_game_card_ia8, 56 * 10, 56, 10, 132.0f, 55.0f },
    { 0x08003EB0u, sf64_ru_training_card_ia8, 96 * 10, 96, 10, 111.0f, 79.0f },
    { 0x08004270u, sf64_ru_vs_card_ia8, 24 * 10, 24, 10, 148.0f, 103.0f },
    { 0x080043B0u, sf64_ru_ranking_card_ia8, 64 * 12, 64, 12, 129.0f, 126.0f },
    { 0x080046B0u, sf64_ru_sound_card_ia8, 40 * 10, 40, 10, 137.0f, 151.0f },
    { 0x08004930u, sf64_ru_data_card_ia8, 64 * 12, 64, 12, 126.0f, 174.0f },
};

static const Sf64RuTexture sf64_ru_option_textures[17] = {
    { 0x08005CD0u, sf64_ru_a_opt_press_rto_test_tex_ia8, 112 * 13 },
    { 0x08006280u, sf64_ru_a_opt_sound_mode_tex_ia8, 48 * 13 },
    { 0x080064F0u, sf64_ru_a_opt_music_volume_tex_ia8, 88 * 14 },
    { 0x080069C0u, sf64_ru_a_opt_voice_volume_tex_ia8, 56 * 15 },
    { 0x08006D10u, sf64_ru_blank_sfx_label_ia8, 24 * 13 },
    { 0x08006E50u, sf64_ru_a_bto_cancel_tex_ia8, 96 * 10 },
    { 0x08007210u, sf64_ru_a_opt_headphone_tex_ia8, 88 * 14 },
    { 0x080076E0u, sf64_ru_a_opt_mono_tex_ia8, 56 * 14 },
    { 0x0800CD90u, sf64_ru_a_opt_stereo_tex_ia8, 56 * 13 },
    { 0x080147F0u, sf64_ru_a_opt_sfx_volume_tex_expanded_ia8, 72 * 13 },
    { 0x080080F0u, sf64_ru_a_ato_confirm_tex_ia8, 96 * 10 },
    { 0x080084B0u, sf64_ru_a_opt_clear_save_data_tex_ia8, 176 * 13 },
    { 0x08008DA0u, sf64_ru_a_opt_are_you_sure_tex_ia8, 160 * 19 },
    { 0x08009980u, sf64_ru_a_opt_yes_tex_ia8, 32 * 12 },
    { 0x08009B00u, sf64_ru_a_opt_no_tex_ia8, 40 * 12 },
    { 0x08009CE0u, sf64_ru_a_opt_are_you_reallysure_tex_ia8, 160 * 41 },
    { 0x0800B680u, sf64_ru_a_opt_data_erased_tex_ia8, 144 * 41 },
};

static const Sf64RuTexture sf64_ru_invoice_textures[5] = {
    { 0x08000680u, sf64_ru_invoice_tex_01_02_palette_ci4_tlut, 32 },
    { 0x08000000u, sf64_ru_invoice_tex_01_title_ci4, 128 * 26 / 2 },
    { 0x080006A0u, sf64_ru_invoice_tex_02_header_ci4, 256 * 34 / 2 },
    { 0x080017A0u, sf64_ru_invoice_tex_03_palette_ci4_tlut, 32 },
    { 0x080017C0u, sf64_ru_invoice_tex_03_amount_ci4, 256 * 66 / 2 },
};

static const Sf64RuTexture sf64_ru_mission_textures[1] = {
    { 0x06000000u, sf64_ru_a_co_title_card_tex_ia8, 128 * 28 },
};

static const u8* sf64_ru_mission_no_full_by_level_index[17] = {
    sf64_ru_a_mission_no_full_1_ia8,
    sf64_ru_a_mission_no_full_2_ia8,
    sf64_ru_a_mission_no_full_2_ia8,
    sf64_ru_a_mission_no_full_3_ia8,
    sf64_ru_a_mission_no_full_3_ia8,
    sf64_ru_a_mission_no_full_3_ia8,
    sf64_ru_a_mission_no_full_4_ia8,
    sf64_ru_a_mission_no_full_4_ia8,
    sf64_ru_a_mission_no_full_4_ia8,
    sf64_ru_a_mission_no_full_5_ia8,
    sf64_ru_a_mission_no_full_5_ia8,
    sf64_ru_a_mission_no_full_5_ia8,
    sf64_ru_a_mission_no_full_6_ia8,
    sf64_ru_a_mission_no_full_6_ia8,
    sf64_ru_a_mission_no_full_7_ia8,
    sf64_ru_a_mission_no_full_7_ia8,
    sf64_ru_a_mission_no_full_7_ia8,
};

static const Sf64RuTexture sf64_ru_map_textures[3] = {
    { 0x06007B90u, sf64_ru_a_co_title_card_tex_ia8, 128 * 28 },
    { 0x06008990u, sf64_ru_a_map_co_title_card_tex_ia8, 232 * 19 },
    { 0x0600D590u, sf64_ru_a_map_lylat_system_card_tex_ia8, 168 * 19 },
};

static const Sf64RuTexture sf64_ru_title_textures[2] = {
    { 0x0600DDC0u, sf64_ru_blank_title_1997_nintendo_ia8, 120 * 12 },
    { 0x0600E360u, sf64_ru_blank_title_press_start_ia8, 120 * 13 },
};

static const Sf64RuTexture sf64_ru_common_textures[11] = {
    { 0x01011A40u, sf64_ru_a_boss_label_boss_ru_32x7_ci4, 32 * 7 / 2 },
    /* CheckPoint two-pass native replacement:
       pass A = aCheckpointTextTex + TLUT_A
       pass B = D_1024020 + TLUT_B */
    { 0x01023E10u, sf64_ru_a_checkpoint_text_page_a_ci4, 64 * 16 / 2 },
    { 0x01024010u, sf64_ru_a_checkpoint_text_tlut_a_8_bytes, 8 * 2 },
    { 0x01024020u, sf64_ru_a_checkpoint_text_page_b_ci4, 64 * 16 / 2 },
    { 0x01024220u, sf64_ru_a_checkpoint_text_tlut_b_8_bytes, 8 * 2 },
    { 0x01000000u, sf64_ru_a_continue_tex_ia8, 64 * 10 },
    { 0x01000280u, sf64_ru_a_retry_course_tex_ia8, 96 * 10 },
    { 0x01000640u, sf64_ru_a_restart_game_tex_ia8, 96 * 22 },
    /* Gameplay ally ship mark: Falco. */
    { 0x01024638u, sf64_ru_a_falco_mark_manual10x9_gray_fillfix_probe8, 0x80 },
    /* Gameplay ally ship mark: Peppy. */
    { 0x010244D8u, sf64_ru_a_peppy_mark_manual7x8_probe4, 0x80 },
    /* Gameplay ally ship mark: Slippy. */
    { 0x01024798u, sf64_ru_a_slippy_mark_manual8x8_probe1, 0x80 },
};

/* Bill/Katt/James gameplay marks are not in segment 0x01.
   Exact native routes from datasyms: aBillMarkTex = 0x0D00B688,
   aJamesMarkTex = 0x0D00B7F0. Bill was fixed in v0.8.21.20; this branch keeps
   that fix and adds James on the correct segment-0x0D route as well. */
static const Sf64RuTexture sf64_ru_actor_mark_segment_d_textures[2] = {
    /* Bill gameplay ally mark: exact segment-0x0D native route. */
    { 0x0D00B688u, sf64_ru_a_bill_mark_manual7x7_probe1, 0x80 },
    /* James gameplay ally mark: exact segment-0x0D native route from datasyms. */
    { 0x0D00B7F0u, sf64_ru_a_james_mark_manual8x7_probe1, 0x80 },
};

static const Sf64RuTexture sf64_ru_text_status_textures[11] = {
    { 0x05001110u, sf64_ru_a_text_enemies_down_ia8, 64 * 25 },
    { 0x05001750u, sf64_ru_a_text_accum_total_ia8, 128 * 10 },
    { 0x05001C50u, sf64_ru_a_text_status_of_team_ia8, 120 * 12 },
    { 0x050022F0u, sf64_ru_a_text_accom_ia8, 120 * 23 },
    { 0x05002DC0u, sf64_ru_a_text_plished_ia8, 136 * 23 },
    { 0x05003A00u, sf64_ru_a_text_mission_ia8, 128 * 23 },
    { 0x05004580u, sf64_ru_a_text_comp_ia8, 96 * 23 },
    { 0x05004E20u, sf64_ru_a_text_lete_ia8, 80 * 21 },
    /* Pause/team-status teammate names: vanilla assets are IA8 in segment 0x05.
       This is the correct native route; do not use a post-draw 168x28 overlay here. */
    { 0x05006CB0u, sf64_ru_a_text_peppy_native_48x10_align1_ia8, 48 * 10 },
    { 0x05006E90u, sf64_ru_a_text_slippy_native_56x10_probe1_ia8, 56 * 10 },
    { 0x05006B30u, sf64_ru_a_text_falco_native_48x8_probe1_ia8, 48 * 8 },
};

static const Sf64RuTexture sf64_ru_title_extra_textures[1] = {
    { 0x060123F0u, sf64_ru_a_title_no_controller_tex_ia8, 176 * 24 },
};

static const Sf64RuTexture sf64_ru_training_textures[1] = {
    { 0x06000000u, sf64_ru_a_tr_quit_training_en_tex_ia8, 96 * 12 },
};

static const Sf64RuTexture sf64_ru_sector_z_missile_textures[1] = {
    { 0x06004458u, sf64_ru_a_sz_missile_mark_ru_32x16_fix10_lifted_ci4, 32 * 16 / 2 },
};

static const Sf64RuTexture sf64_ru_map_course_textures[4] = {
    { 0x06000000u, sf64_ru_a_map_retry_course_game_over_tex_ia8, 96 * 22 },
    { 0x06000840u, sf64_ru_a_map_proceed_next_course_tex_ia8, 96 * 22 },
    { 0x06001080u, sf64_ru_a_map_retry_course_lose_1up_tex_ia8, 96 * 22 },
    { 0x060018C0u, sf64_ru_a_map_change_course_tex_ia8, 96 * 10 },
};

static u8* sf64_ru_segmented_to_virtual(u32 address) {
    u32 segment = address >> 24;
    u32 offset = address & 0x00FFFFFFu;

    return (u8*) OS_PHYSICAL_TO_K0(gSegments[segment] + offset);
}

static void sf64_ru_copy_texture(const Sf64RuTexture* replacement) {
    u8* destination = sf64_ru_segmented_to_virtual(replacement->segmented_address);
    s32 byte_index;

    for (byte_index = 0; byte_index < replacement->byte_count; byte_index++) {
        destination[byte_index] = replacement->replacement[byte_index];
    }
}

static void sf64_ru_copy_texture_list(const Sf64RuTexture* replacements, s32 count) {
    s32 texture_index;

    for (texture_index = 0; texture_index < count; texture_index++) {
        sf64_ru_copy_texture(&replacements[texture_index]);
    }
}

static void sf64_ru_patch_segment_d_actor_marks(void) {
    if ((gSceneId >= 3) && (gSegments[13] != 0)) {
        sf64_ru_copy_texture_list(sf64_ru_actor_mark_segment_d_textures, SF64_RU_ARRAY_COUNT(sf64_ru_actor_mark_segment_d_textures));
    }
}

static void sf64_ru_apply_main_menu_layout(void) {
    s32 card_index;

    for (card_index = 0; card_index < 6; card_index++) {
        const Sf64RuMenuCard* replacement = &sf64_ru_menu_cards[card_index];
        Sf64OptionCardTexture* texture = &sOptionCardList[card_index].tex;

        texture->width = replacement->width;
        texture->height = replacement->height;
        texture->x_pos = replacement->x_pos;
        texture->y_pos = replacement->y_pos;
        sOptionCardTextPosX[card_index] = replacement->x_pos;
        sOptionCardTextPosY[card_index] = replacement->y_pos;
        sOptionCardCurTextPosX[card_index] = replacement->x_pos;
        sOptionCardCurTextPosY[card_index] = replacement->y_pos;
    }

    if (sExpertModeCursor != 0) {
        Sf64OptionCardTexture* texture = &sOptionCardList[0].tex;

        texture->texture = sf64_ru_expert_card_ia8;
        texture->width = 64;
        texture->height = 10;
        texture->x_pos = 128.0f;
        texture->y_pos = 55.0f;
        sOptionCardCurTextPosX[0] = 128.0f;
        sOptionCardCurTextPosY[0] = 55.0f;
    }

    if (sExpertSoundCursor != 0) {
        Sf64OptionCardTexture* texture = &sOptionCardList[4].tex;

        texture->texture = sf64_ru_expert_card_ia8;
        texture->width = 64;
        texture->height = 10;
        texture->x_pos = 128.0f;
        texture->y_pos = 151.0f;
        sOptionCardCurTextPosX[4] = 128.0f;
        sOptionCardCurTextPosY[4] = 151.0f;
    }
}

static void sf64_ru_patch_one_planet_name_card(s32 index, const u8* texture) {
    sPlanetNameCards[index].texture = (u8*) texture;
    sPlanetNameCards[index].width = 168;
    sPlanetNameCards[index].height = 28;
    sPlanetNameCards[index].x_pos = 76.0f;
}

static void sf64_ru_patch_one_planet_title_card(s32 index, const u8* texture, s32 width, s32 height, f32 x_pos) {
    sPlanetTitleCards[index].texture = (u8*) texture;
    sPlanetTitleCards[index].width = width;
    sPlanetTitleCards[index].height = height;
    sPlanetTitleCards[index].x_pos = x_pos;
}

static void sf64_ru_patch_briefing_title_tables_batch3_fix1(void) {
    if (gSceneId != 2) {
        return;
    }

    /* Native map/briefing title-card route, Codex-style.
       IMPORTANT: sPlanetNameCards[] order is NOT HUD_GetLevelIndex order.
       Decomp table order is:
       0=Corneria, 1=Meteo, 2=Sector Y, 3=Fortuna, 4=Katina, 5=Aquas,
       6=Sector X, 7=Solar, 8=Zoness, 9=Titania, 10=Macbeth,
       11=Sector Z, 12=Bolse, 13=Area 6.
       Corneria index 0 is intentionally left on its existing special route. */
    sf64_ru_patch_one_planet_name_card(1, sf64_ru_a_me_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(2, sf64_ru_a_sy_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(3, sf64_ru_a_fo_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(4, sf64_ru_a_ka_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(5, sf64_ru_a_aq_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(6, sf64_ru_a_sx_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(7, sf64_ru_a_so_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(8, sf64_ru_a_zo_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(9, sf64_ru_a_ti_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(10, sf64_ru_a_ma_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(11, sf64_ru_a_sz_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(12, sf64_ru_a_bo_title_card_tex_ia8);
    sf64_ru_patch_one_planet_name_card(13, sf64_ru_a_a6_title_card_tex_ia8);

    /* Lower briefing subtitles through native sPlanetTitleCards[].
       Meteo, Sector Y, Fortuna, Katina, Aquas, Sector X, Solar, Zoness, Titania, Macbeth, Sector Z, and Bolse lower subtitles were tested/kept.
       Probe13 adds only Area 6 lower subtitle: Through the Middle.
       Table order: 1=Meteo, 2=Sector Y, 3=Fortuna, 4=Katina, 5=Aquas, 6=Sector X, 7=Solar, 8=Zoness, 9=Titania, 10=Macbeth, 11=Sector Z, 12=Bolse, 13=Area 6.
       Native lower cards use 208x19, x=56.0f.
       Do not batch all remaining lower subtitles until each pair is checked in-game. */
    sf64_ru_patch_one_planet_title_card(1, sf64_ru_a_map_me_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(2, sf64_ru_a_map_sy_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(3, sf64_ru_a_map_fo_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(4, sf64_ru_a_map_ka_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(5, sf64_ru_a_map_aq_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(6, sf64_ru_a_map_sx_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(7, sf64_ru_a_map_so_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(8, sf64_ru_a_map_zo_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(9, sf64_ru_a_map_ti_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(10, sf64_ru_a_map_ma_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(11, sf64_ru_a_map_sz_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(12, sf64_ru_a_map_bo_lower_mission_title_ia8, 208, 19, 56.0f);
    sf64_ru_patch_one_planet_title_card(13, sf64_ru_a_map_a6_lower_mission_title_ia8, 208, 19, 56.0f);
}

static void sf64_ru_copy_fortuna_title_card_direct(void) {
    Sf64RuTexture replacement;

    if ((gSegments[6] == 0) || (HUD_GetLevelIndex() != 3)) {
        return;
    }

    replacement.segmented_address = 0x06000000u;
    replacement.replacement = sf64_ru_a_fo_title_card_tex_ia8;
    replacement.byte_count = 168 * 28;
    sf64_ru_copy_texture(&replacement);
}

static void sf64_ru_apply_meteo_title_card_pointer_probe(void) {
    s32 level_index = HUD_GetLevelIndex();

    if (level_index != 1) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_me_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_katina_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_katina < 0) || (gCurrentLevel != sf64_ru_level_id_katina)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_ka_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_aquas_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_aquas < 0) || (gCurrentLevel != sf64_ru_level_id_aquas)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_aq_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_sector_y_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_sector_y < 0) || (gCurrentLevel != sf64_ru_level_id_sector_y)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_sy_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_sector_x_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_sector_x < 0) || (gCurrentLevel != sf64_ru_level_id_sector_x)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_sx_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_solar_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_solar < 0) || (gCurrentLevel != sf64_ru_level_id_solar)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_so_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_zoness_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_zoness < 0) || (gCurrentLevel != sf64_ru_level_id_zoness)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_zo_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_titania_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_titania < 0) || (gCurrentLevel != sf64_ru_level_id_titania)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_ti_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_macbeth_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_macbeth < 0) || (gCurrentLevel != sf64_ru_level_id_macbeth)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_ma_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_sector_z_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_sector_z < 0) || (gCurrentLevel != sf64_ru_level_id_sector_z)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_sz_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_bolse_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_bolse < 0) || (gCurrentLevel != sf64_ru_level_id_bolse)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_bo_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_area6_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_area6 < 0) || (gCurrentLevel != sf64_ru_level_id_area6)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_a6_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_venom1_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_venom1 < 0) || (gCurrentLevel != sf64_ru_level_id_venom1)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_ve1_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_apply_venom2_title_card_pointer_probe(void) {
    s32 level_index;

    if ((sf64_ru_level_id_venom2 < 0) || (gCurrentLevel != sf64_ru_level_id_venom2)) {
        return;
    }

    level_index = HUD_GetLevelIndex();
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) sf64_ru_a_ve2_title_card_tex_ia8;
    sLevelTitleCard[level_index].titleCardWidth = 168;
    sLevelTitleCard[level_index].titleCardHeight = 28;
}

static void sf64_ru_set_title_card_entry(s32 level_index, const u8* texture, s32 width, s32 height) {
    if ((level_index < 0) || (level_index >= 17)) {
        return;
    }

    sLevelTitleCard[level_index].titleCardTex = (u8*) texture;
    sLevelTitleCard[level_index].titleCardWidth = width;
    sLevelTitleCard[level_index].titleCardHeight = height;
}

static void sf64_ru_patch_venom_title_card_entries_safe(void) {
    /* Native sLevelTitleCard table has three Venom-related entries:
       14 = Venom 1 lower title (aVe1, segment 06),
       15 = Venom 2 lower title (aVe2, segment 06),
       16 = Andross/true-final title (English aAndTitleCardEnTex, segment 0C).
       Entry 16 is the one that survives on pause after the first Venom boss. */
    sf64_ru_set_title_card_entry(14, sf64_ru_a_ve1_title_card_tex_ia8, 168, 28);
    sf64_ru_set_title_card_entry(15, sf64_ru_a_ve2_title_card_tex_ia8, 168, 28);
    sf64_ru_set_title_card_entry(16, sf64_ru_a_ve_title_card_tex_ia8, 168, 28);
}

static void sf64_ru_draw_venom_upper_title_ru(void) {
    if (sf64_ru_venom_upper_ru_pending == 0) {
        return;
    }

    sf64_ru_venom_upper_ru_pending = 0;
    RCP_SetupDL(&gMasterDisp, 76);
    sf64_ru_set_prim_color(0xFFFFFFFFu);
    Lib_TextureRect_IA8(&gMasterDisp, (u8*) sf64_ru_a_ve_title_card_tex_ia8, 168, 28, 76.0f, 52.0f, 1.0f, 1.0f);
}

static s32 sf64_ru_current_level_is_venom_title_special(void) {
    s32 level_index = HUD_GetLevelIndex();

    if ((level_index == 14) || (level_index == 15) || (level_index == 16)) {
        return 1;
    }

    if ((gCurrentLevel == SF64_RU_CURRENT_LEVEL_VENOM1) || (gCurrentLevel == SF64_RU_CURRENT_LEVEL_VENOM2)) {
        return 1;
    }

    if ((sf64_ru_level_id_venom1 >= 0) && (gCurrentLevel == sf64_ru_level_id_venom1)) {
        return 1;
    }

    if ((sf64_ru_level_id_venom2 >= 0) && (gCurrentLevel == sf64_ru_level_id_venom2)) {
        return 1;
    }

    return 0;
}

static void sf64_ru_suppress_venom_upper_title_begin(void) {
    if (sf64_ru_current_level_is_venom_title_special() == 0) {
        return;
    }

    /* HUD_TitleCard_Draw calls HUD_VenomTitleCard_Draw only for the special upper
       English Venom titles. Temporarily masking gCurrentLevel makes the vanilla
       function take its default no-draw path, then the return hook restores it
       before HUD_TitleCard_Draw continues to draw the Russian 168x28 titleCardTex.
       This avoids writing into the texture row pointer, which crashed in the
       previous Lib_TextureRect_IA8 row-blank probe. */
    sf64_ru_venom_saved_current_level = gCurrentLevel;
    sf64_ru_venom_suppress_upper_pending = 1;
    sf64_ru_venom_upper_ru_pending = 1;
    gCurrentLevel = -1;
}

static void sf64_ru_suppress_venom_upper_title_end(void) {
    if (sf64_ru_venom_suppress_upper_pending == 0) {
        return;
    }

    gCurrentLevel = sf64_ru_venom_saved_current_level;
    sf64_ru_venom_suppress_upper_pending = 0;
}

static void sf64_ru_blank_native_mission_number(void) {
    s32 level_index;

    if (gSegments[5] != 0) {
        Sf64RuTexture blank_prefix = { 0x05000500u, sf64_ru_blank_mission_prefix_ia8, 112 * 19 };

        sf64_ru_copy_texture(&blank_prefix);
    }

    for (level_index = 0; level_index < 17; level_index++) {
        sLevelTitleCard[level_index].levelIdxTex = (u8*) sf64_ru_blank_mission_digit_ia8;
        sLevelTitleCard[level_index].width = 16;
        sLevelTitleCard[level_index].height = 15;
    }
}

static const u8* sf64_ru_get_current_mission_no_full(void) {
    s32 level_index = HUD_GetLevelIndex();

    if ((level_index < 0) || (level_index >= 17)) {
        return 0;
    }
    return sf64_ru_mission_no_full_by_level_index[level_index];
}

static void sf64_ru_draw_current_mission_no_full(f32 x_pos, f32 y_pos) {
    const u8* texture = sf64_ru_get_current_mission_no_full();

    if (texture == 0) {
        return;
    }

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_set_prim_color(0xFFFFFFFFu);
    Lib_TextureRect_IA8(&gMasterDisp, (u8*) texture, 136, 20, x_pos, y_pos, 1.0f, 1.0f);
}

static s32 sf64_ru_level_clear_mission_no_visible(void) {
    s32 level_index = HUD_GetLevelIndex();

    if (gLevelClearScreenTimer <= 0) {
        return 0;
    }

    if (level_index == 5) {
        return gLevelClearScreenTimer <= 92;
    }
    return gLevelClearScreenTimer <= 80;
}


RECOMP_HOOK_RETURN("Katina_LevelStart")
void sf64_ru_after_katina_level_start(void) {
    sf64_ru_level_id_katina = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Aquas_CsLevelStart")
void sf64_ru_after_aquas_cs_level_start(void) {
    sf64_ru_level_id_aquas = gCurrentLevel;
}

RECOMP_HOOK_RETURN("SectorY_LevelStart")
void sf64_ru_after_sector_y_level_start(void) {
    sf64_ru_level_id_sector_y = gCurrentLevel;
}

RECOMP_HOOK_RETURN("SectorX_LevelStart")
void sf64_ru_after_sector_x_level_start(void) {
    sf64_ru_level_id_sector_x = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Solar_LevelStart")
void sf64_ru_after_solar_level_start(void) {
    sf64_ru_level_id_solar = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Zoness_LevelStart")
void sf64_ru_after_zoness_level_start(void) {
    sf64_ru_level_id_zoness = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Titania_LevelStart")
void sf64_ru_after_titania_level_start(void) {
    sf64_ru_level_id_titania = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Macbeth_LevelStart")
void sf64_ru_after_macbeth_level_start(void) {
    sf64_ru_level_id_macbeth = gCurrentLevel;
}

RECOMP_HOOK_RETURN("SectorZ_LevelStart")
void sf64_ru_after_sector_z_level_start(void) {
    sf64_ru_level_id_sector_z = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Bolse_LevelStart")
void sf64_ru_after_bolse_level_start(void) {
    sf64_ru_level_id_bolse = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Area6_LevelStart")
void sf64_ru_after_area6_level_start(void) {
    sf64_ru_level_id_area6 = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Venom1_LevelStart")
void sf64_ru_after_venom1_level_start(void) {
    sf64_ru_level_id_venom1 = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Venom1_LevelStart2")
void sf64_ru_after_venom1_level_start2(void) {
    sf64_ru_level_id_venom1 = gCurrentLevel;
}

RECOMP_HOOK_RETURN("Venom2_LevelStart")
void sf64_ru_after_venom2_level_start(void) {
    sf64_ru_level_id_venom2 = gCurrentLevel;
}

RECOMP_HOOK("HUD_RadarMissileAlarm_Draw")
void sf64_ru_before_hud_radar_missile_alarm_draw(void) {
    if (gSegments[6] != 0) {
        sf64_ru_copy_texture_list(sf64_ru_sector_z_missile_textures, SF64_RU_ARRAY_COUNT(sf64_ru_sector_z_missile_textures));
    }
}

RECOMP_HOOK_RETURN("Option_MainMenu_Setup")
void sf64_ru_after_option_main_menu_setup(void) {
    s32 card_index;
    s32 texture_index;

    for (card_index = 0; card_index < 6; card_index++) {
        const Sf64RuMenuCard* replacement = &sf64_ru_menu_cards[card_index];
        u8* destination = sf64_ru_segmented_to_virtual(replacement->segmented_address);
        s32 byte_index;

        for (byte_index = 0; byte_index < replacement->byte_count; byte_index++) {
            destination[byte_index] = replacement->replacement[byte_index];
        }
    }
    sf64_ru_apply_main_menu_layout();

    for (texture_index = 0; texture_index < SF64_RU_ARRAY_COUNT(sf64_ru_option_textures); texture_index++) {
        const Sf64RuTexture* replacement = &sf64_ru_option_textures[texture_index];
        sf64_ru_copy_texture(replacement);
    }
}

RECOMP_HOOK("Option_InvoiceDraw")
void sf64_ru_before_option_invoice_draw(void) {
    if (gSegments[8] != 0) {
        sf64_ru_copy_texture_list(sf64_ru_invoice_textures, SF64_RU_ARRAY_COUNT(sf64_ru_invoice_textures));
    }
}

RECOMP_HOOK_RETURN("Option_MainMenu_Update")
void sf64_ru_after_option_main_menu_update(void) {
    if ((sMainMenuState == 1) || (sMainMenuState == 1000)) {
        sf64_ru_apply_main_menu_layout();
    }
}

RECOMP_HOOK_RETURN("Load_SceneSetup")
void sf64_ru_after_load_scene_setup(void) {
    sf64_ru_patch_venom_title_card_entries_safe();
    sf64_ru_copy_fortuna_title_card_direct();
    if ((gSceneId >= 3) && (gSegments[1] != 0)) {
        sf64_ru_copy_texture_list(sf64_ru_common_textures, SF64_RU_ARRAY_COUNT(sf64_ru_common_textures));
    }
    sf64_ru_patch_segment_d_actor_marks();
    if ((gSceneId >= 3) && (gSegments[5] != 0)) {
        sf64_ru_copy_texture_list(sf64_ru_text_status_textures, 11);
    }
    if ((gSceneId == 2) && (gSegments[6] != 0)) {
        sf64_ru_patch_briefing_title_tables_batch3_fix1();
        sf64_ru_copy_texture(&sf64_ru_map_textures[0]);
        sf64_ru_copy_texture(&sf64_ru_map_textures[1]);
        sf64_ru_copy_texture(&sf64_ru_map_textures[2]);
        sf64_ru_copy_texture_list(sf64_ru_map_course_textures, 4);
    }
    if ((gSceneId == 5) && (gCurrentLevel == 0) && (gSegments[6] != 0)) {
        sf64_ru_blank_native_mission_number();
        sf64_ru_copy_texture(&sf64_ru_mission_textures[0]);
    }
    if ((gSceneId == 0) && (gSegments[6] != 0)) {
        sf64_ru_copy_texture(&sf64_ru_title_textures[0]);
        sf64_ru_copy_texture(&sf64_ru_title_textures[1]);
        sf64_ru_copy_texture_list(sf64_ru_title_extra_textures, 1);
    }
    if ((gSceneId == 17) && (gSegments[6] != 0)) {
        sf64_ru_copy_texture_list(sf64_ru_training_textures, 1);
    }
}


RECOMP_HOOK("Display_ActorMarks")
void sf64_ru_before_display_actor_marks_patch_segment_d_marks(void) {
    sf64_ru_patch_segment_d_actor_marks();
}

RECOMP_HOOK("Map_Draw")
void sf64_ru_before_map_draw_table_patch_probe(void) {
    sf64_ru_patch_briefing_title_tables_batch3_fix1();
}

RECOMP_HOOK("HUD_VenomTitleCard_Draw")
void sf64_ru_before_hud_venom_title_card_draw_suppress_upper(void) {
    sf64_ru_suppress_venom_upper_title_begin();
}

RECOMP_HOOK_RETURN("HUD_VenomTitleCard_Draw")
void sf64_ru_after_hud_venom_title_card_draw_suppress_upper(void) {
    sf64_ru_suppress_venom_upper_title_end();
    sf64_ru_draw_venom_upper_title_ru();
}

RECOMP_HOOK("HUD_TitleCard_Draw")
void sf64_ru_before_hud_title_card_draw(f32 x, f32 y) {
    sf64_ru_patch_venom_title_card_entries_safe();
    sf64_ru_apply_meteo_title_card_pointer_probe();
    sf64_ru_apply_katina_title_card_pointer_probe();
    sf64_ru_apply_aquas_title_card_pointer_probe();
    sf64_ru_apply_sector_y_title_card_pointer_probe();
    sf64_ru_apply_sector_x_title_card_pointer_probe();
    sf64_ru_apply_solar_title_card_pointer_probe();
    sf64_ru_apply_zoness_title_card_pointer_probe();
    sf64_ru_apply_titania_title_card_pointer_probe();
    sf64_ru_apply_macbeth_title_card_pointer_probe();
    sf64_ru_apply_sector_z_title_card_pointer_probe();
    sf64_ru_apply_bolse_title_card_pointer_probe();
    sf64_ru_apply_area6_title_card_pointer_probe();
    sf64_ru_apply_venom1_title_card_pointer_probe();
    sf64_ru_apply_venom2_title_card_pointer_probe();
    sf64_ru_copy_fortuna_title_card_direct();
    sf64_ru_blank_native_mission_number();
    sf64_ru_last_title_card_x = x;
    sf64_ru_last_title_card_y = y;
    sf64_ru_title_card_overlay_pending = 1;
}

RECOMP_HOOK_RETURN("HUD_TitleCard_Draw")
void sf64_ru_after_hud_title_card_draw(void) {
    s32 level_index = HUD_GetLevelIndex();

    if (sf64_ru_title_card_overlay_pending == 0) {
        return;
    }

    sf64_ru_title_card_overlay_pending = 0;
    if ((level_index == 14) || (level_index == 15)) {
        return;
    }
    sf64_ru_draw_current_mission_no_full(sf64_ru_last_title_card_x - 16.0f, sf64_ru_last_title_card_y + 3.0f);
}


RECOMP_HOOK("HUD_PauseScreen_Update")
void sf64_ru_before_hud_pause_screen_update_patch_venom_title(void) {
    sf64_ru_patch_venom_title_card_entries_safe();
}

RECOMP_HOOK("HUD_DrawLevelClearScreen")
void sf64_ru_before_hud_draw_level_clear_screen(void) {
    sf64_ru_blank_native_mission_number();
}

RECOMP_HOOK_RETURN("HUD_DrawLevelClearScreen")
void sf64_ru_after_hud_draw_level_clear_screen(void) {
    if (sf64_ru_level_clear_mission_no_visible() == 0) {
        return;
    }

    sf64_ru_draw_current_mission_no_full(92.0f, 64.0f);
}

static void sf64_ru_set_prim_color(u32 rgba) {
    Gfx* command = gMasterDisp++;

    command->words[0] = 0xFA000000u;
    command->words[1] = rgba;
}

static void sf64_ru_prepare_mission_header_cover(void) {
    s32 byte_index;

    for (byte_index = 0; byte_index < (160 * 28); byte_index++) {
        sf64_ru_mission_header_cover_ia8[byte_index] = 0xFF;
    }
}

static void sf64_ru_draw_ia8_stripes(const u8* texture, u32 width, u32 height, f32 x_pos, f32 y_pos, u32 stripe_height) {
    u32 y_offset;

    for (y_offset = 0; y_offset < height; y_offset += stripe_height) {
        u32 rows = height - y_offset;

        if (rows > stripe_height) {
            rows = stripe_height;
        }
        Lib_TextureRect_IA8(&gMasterDisp, (u8*) (texture + (width * y_offset)), width, rows, x_pos, y_pos + y_offset,
                            1.0f, 1.0f);
    }
}

RECOMP_HOOK_RETURN("Map_801A9A8C")
void sf64_ru_after_map_mission_card_draw(void) {
    const u8* texture = sf64_ru_get_current_mission_no_full();

    if (texture == 0) {
        return;
    }

    sf64_ru_prepare_mission_header_cover();
    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_set_prim_color(0x000000FFu);
    Lib_TextureRect_IA8(&gMasterDisp, sf64_ru_mission_header_cover_ia8, 160, 28, 80.0f, 56.0f, 1.0f, 1.0f);
    sf64_ru_set_prim_color(0xFFFFFFFFu);
    Lib_TextureRect_IA8(&gMasterDisp, (u8*) texture, 136, 20, 92.0f, 60.0f, 1.0f, 1.0f);
}

RECOMP_HOOK_RETURN("Title_PressStart_Draw")
void sf64_ru_after_title_press_start_draw(void) {
    if (gControllerLock != 0) {
        return;
    }

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_set_prim_color(0xFF0000FFu | ((u32) sTitleTextPrimCol << 16) | ((u32) sTitleTextPrimCol << 8));
    if (gMainController >= 0) {
        sf64_ru_draw_ia8_stripes(sf64_ru_a_title_press_start_ia8, 184, 16, 68.0f, 187.0f, 4);
    }

    sf64_ru_set_prim_color(0xFFFFFFFFu);
    sf64_ru_draw_ia8_stripes(sf64_ru_a_title_copyright_block_ia8, 176, 30, 72.0f, 207.0f, 4);
}

RECOMP_HOOK_RETURN("Option_Sound_Draw")
void sf64_ru_after_option_sound_draw(void) {
    s32 color_gb = (s32) D_menu_801B9270[3];

    RCP_SetupDL(&gMasterDisp, 83);
    sf64_ru_set_prim_color(0xFF0000FFu | ((u32) color_gb << 16) | ((u32) color_gb << 8));
    Lib_TextureRect_IA8(&gMasterDisp, sf64_ru_segmented_to_virtual(0x080147F0u), 72, 13, 63.0f, 186.0f, 1.0f, 1.0f);
}
