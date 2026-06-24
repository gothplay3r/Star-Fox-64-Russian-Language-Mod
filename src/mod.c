
#include "modding.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int s32;
typedef float f32;
typedef struct Gfx { u32 w0; u32 w1; } Gfx;

extern u8 gTextCharTextures[][104];
extern u16 gTextCharPalettes[];
void Lib_TextureRect_CI4(Gfx** gfxPtr, u8* texture, u16* palette, s32 width, s32 height, f32 x, f32 y, f32 xScale, f32 yScale);

#define RU_BASE 0x0100
#define RU_LAST 0x0141
#define SF64_RU_BLOB_MAGIC_A 0x5255
#define SF64_RU_BLOB_MAGIC_B 0x3634
#define SF64_RU_BLOB_TAIL 0xCAFE
#define SF64_RU_BLOB_HEADER_WORDS 8
#define SF64_RU_RADIO_MAGIC_A 0x5252
#define SF64_RU_RADIO_MAGIC_B 0x3634
#define SF64_RU_RADIO_MAX_WORDS 64
#define MSGCHAR_END 0x0000
#define MSGCHAR_NWL 0x0001
#define MSGCHAR_NP2 0x0002
#define MSGCHAR_NP3 0x0003
#define MSGCHAR_NP4 0x0004
#define MSGCHAR_NP5 0x0005
#define MSGCHAR_NP6 0x0006
#define MSGCHAR_NP7 0x0007
#define MSGCHAR_PRI0 0x0008
#define MSGCHAR_PRI1 0x0009
#define MSGCHAR_PRI2 0x000A
#define MSGCHAR_PRI3 0x000B
#define MSGCHAR_SPC 0x000C
#define MSGCHAR_QSP 0x000D
#define MSGCHAR_HSP 0x000E
#define MSGCHAR_NXT 0x000F
#define MSGCHAR_CLF 0x0010
#define MSGCHAR_CUP 0x0011
#define MSGCHAR_CRT 0x0012
#define MSGCHAR_CDN 0x0013

/* Radio text horizontal shift.
   Negative value moves only radio Message_DisplayText output left.
   -4 px is the current safe value: enough margin gain without touching the portrait/frame. */
#define SF64_RU_RADIO_TEXT_X_SHIFT_PX (-4)


#include "ru_glyphs.inc"

static u16 sf64_ru_white_tlut[16] = { 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
static int sf64_ru_slot_applied = 0;


/* Radio-overflow NXT guard.
   Problem fixed: original Radio_Draw advances gRadioMsgCharIndex and checks
   MSGCHAR_NXT against the original gRadioMsg buffer. For radio_overflow rows,
   drawing is redirected to the external RU buffer, but the original buffer can
   still contain an earlier MSGCHAR_NXT or a shorter hard stop. That can cut the
   visible RU line even though the overflow blob is valid.

   This hook is deliberately narrow:
   - it does not replace gRadioMsg;
   - it does not repoint gMsgLookup;
   - it does not touch voice ID lookup;
   - it only corrects radio character-index progression after Radio_Draw when
     an enabled radio_overflow blob exists for the current original msg.
*/
extern s32 gRadioState;
extern s32 gRadioMsgCharIndex;
extern u16* gRadioMsg;
extern s32 Audio_GetCurrentVoiceStatus(void);

static s32 sf64_ru_radio_before_state = 0;
static s32 sf64_ru_radio_before_char_index = 0;
static u16* sf64_ru_radio_before_msg = (u16*)0;
static u16* sf64_ru_radio_before_ru_msg = (u16*)0;
static s32 sf64_ru_radio_before_active = 0;


static void sf64_ru_gfx_set_prim_color(Gfx** gfxPtr, u8 r, u8 g, u8 b, u8 a) {
    /* Manual gDPSetPrimColor. The original Message_DisplayText resets this
       before drawing text; our patched renderer must do the same or radio
       text inherits the blue/purple color left by the radio window draw. */
    (*gfxPtr)->w0 = 0xFA000000u;
    (*gfxPtr)->w1 = ((u32)r << 24) | ((u32)g << 16) | ((u32)b << 8) | (u32)a;
    (*gfxPtr)++;
}

typedef struct Sf64RuRuntimeSlotEntry { volatile u16* dst; volatile u16* blob; int max_words; } Sf64RuRuntimeSlotEntry;
typedef struct Sf64RuRadioExtEntry { u16 id; volatile u16* original; volatile u16* blob; int max_words; } Sf64RuRadioExtEntry;
extern u16 gMsg_ID_1[];
extern u16 gMsg_ID_10[];
extern u16 gMsg_ID_20[];
extern u16 gMsg_ID_30[];
extern u16 gMsg_ID_40[];
extern u16 gMsg_ID_50[];
extern u16 gMsg_ID_60[];
extern u16 gMsg_ID_1200[];
extern u16 gMsg_ID_1210[];
extern u16 gMsg_ID_1220[];
extern u16 gMsg_ID_1230[];
extern u16 gMsg_ID_1240[];
extern u16 gMsg_ID_1250[];
extern u16 gMsg_ID_1260[];
extern u16 gMsg_ID_1270[];
extern u16 gMsg_ID_1280[];
extern u16 gMsg_ID_1290[];
extern u16 gMsg_ID_1300[];
extern u16 gMsg_ID_1310[];
extern u16 gMsg_ID_1320[];
extern u16 gMsg_ID_1330[];
extern u16 gMsg_ID_1340[];
extern u16 gMsg_ID_1350[];
extern u16 gMsg_ID_1360[];
extern u16 gMsg_ID_1370[];
extern u16 gMsg_ID_1380[];
extern u16 gMsg_ID_1390[];
extern u16 gMsg_ID_1400[];
extern u16 gMsg_ID_1410[];
extern u16 gMsg_ID_1420[];
extern u16 gMsg_ID_1430[];
extern u16 gMsg_ID_1440[];
extern u16 gMsg_ID_1450[];
extern u16 gMsg_ID_1460[];
extern u16 gMsg_ID_1470[];
extern u16 gMsg_ID_2005[];
extern u16 gMsg_ID_2010[];
extern u16 gMsg_ID_2020[];
extern u16 gMsg_ID_2030[];
extern u16 gMsg_ID_2040[];
extern u16 gMsg_ID_2050[];
extern u16 gMsg_ID_2055[];
extern u16 gMsg_ID_2058[];
extern u16 gMsg_ID_2061[];
extern u16 gMsg_ID_2062[];
extern u16 gMsg_ID_2080[];
extern u16 gMsg_ID_2090[];
extern u16 gMsg_ID_2095[];
extern u16 gMsg_ID_2110[];
extern u16 gMsg_ID_2115[];
extern u16 gMsg_ID_2118[];
extern u16 gMsg_ID_2140[];
extern u16 gMsg_ID_2165[];
extern u16 gMsg_ID_2166[];
extern u16 gMsg_ID_2167[];
extern u16 gMsg_ID_2180[];
extern u16 gMsg_ID_2181[];
extern u16 gMsg_ID_2185[];
extern u16 gMsg_ID_2188[];
extern u16 gMsg_ID_2200[];
extern u16 gMsg_ID_2210[];
extern u16 gMsg_ID_2220[];
extern u16 gMsg_ID_2225[];
extern u16 gMsg_ID_2230[];
extern u16 gMsg_ID_2233[];
extern u16 gMsg_ID_2240[];
extern u16 gMsg_ID_2250[];
extern u16 gMsg_ID_2260[];
extern u16 gMsg_ID_2263[];
extern u16 gMsg_ID_2265[];
extern u16 gMsg_ID_2270[];
extern u16 gMsg_ID_2275[];
extern u16 gMsg_ID_2280[];
extern u16 gMsg_ID_2282[];
extern u16 gMsg_ID_2290[];
extern u16 gMsg_ID_2291[];
extern u16 gMsg_ID_2292[];
extern u16 gMsg_ID_2293[];
extern u16 gMsg_ID_2294[];
extern u16 gMsg_ID_2295[];
extern u16 gMsg_ID_2296[];
extern u16 gMsg_ID_2298[];
extern u16 gMsg_ID_2299[];
extern u16 gMsg_ID_2300[];
extern u16 gMsg_ID_2305[];
extern u16 gMsg_ID_2310[];
extern u16 gMsg_ID_2320[];
extern u16 gMsg_ID_2335[];
extern u16 gMsg_ID_2336[];
extern u16 gMsg_ID_2337[];
extern u16 gMsg_ID_3005[];
extern u16 gMsg_ID_3010[];
extern u16 gMsg_ID_3015[];
extern u16 gMsg_ID_3020[];
extern u16 gMsg_ID_3025[];
extern u16 gMsg_ID_3026[];
extern u16 gMsg_ID_3040[];
extern u16 gMsg_ID_3041[];
extern u16 gMsg_ID_3042[];
extern u16 gMsg_ID_3050[];
extern u16 gMsg_ID_3100[];
extern u16 gMsg_ID_3110[];
extern u16 gMsg_ID_3120[];
extern u16 gMsg_ID_3300[];
extern u16 gMsg_ID_3310[];
extern u16 gMsg_ID_3315[];
extern u16 gMsg_ID_3320[];
extern u16 gMsg_ID_3321[];
extern u16 gMsg_ID_3322[];
extern u16 gMsg_ID_3330[];
extern u16 gMsg_ID_3340[];
extern u16 gMsg_ID_3345[];
extern u16 gMsg_ID_3350[];
extern u16 gMsg_ID_3360[];
extern u16 gMsg_ID_3370[];
extern u16 gMsg_ID_3371[];
extern u16 gMsg_ID_4010[];
extern u16 gMsg_ID_4011[];
extern u16 gMsg_ID_4012[];
extern u16 gMsg_ID_4013[];
extern u16 gMsg_ID_4020[];
extern u16 gMsg_ID_4021[];
extern u16 gMsg_ID_4022[];
extern u16 gMsg_ID_4023[];
extern u16 gMsg_ID_4024[];
extern u16 gMsg_ID_4030[];
extern u16 gMsg_ID_4031[];
extern u16 gMsg_ID_4040[];
extern u16 gMsg_ID_4050[];
extern u16 gMsg_ID_4075[];
extern u16 gMsg_ID_4080[];
extern u16 gMsg_ID_4082[];
extern u16 gMsg_ID_4083[];
extern u16 gMsg_ID_4091[];
extern u16 gMsg_ID_4092[];
extern u16 gMsg_ID_4093[];
extern u16 gMsg_ID_4094[];
extern u16 gMsg_ID_4095[];
extern u16 gMsg_ID_4096[];
extern u16 gMsg_ID_4097[];
extern u16 gMsg_ID_4098[];
extern u16 gMsg_ID_4099[];
extern u16 gMsg_ID_4100[];
extern u16 gMsg_ID_4101[];
extern u16 gMsg_ID_4102[];
extern u16 gMsg_ID_4103[];
extern u16 gMsg_ID_4110[];
extern u16 gMsg_ID_4111[];
extern u16 gMsg_ID_4112[];
extern u16 gMsg_ID_4113[];
extern u16 gMsg_ID_5000[];
extern u16 gMsg_ID_5010[];
extern u16 gMsg_ID_5060[];
extern u16 gMsg_ID_5080[];
extern u16 gMsg_ID_5100[];
extern u16 gMsg_ID_5110[];
extern u16 gMsg_ID_5130[];
extern u16 gMsg_ID_5220[];
extern u16 gMsg_ID_5230[];
extern u16 gMsg_ID_5300[];
extern u16 gMsg_ID_5310[];
extern u16 gMsg_ID_5311[];
extern u16 gMsg_ID_5312[];
extern u16 gMsg_ID_5313[];
extern u16 gMsg_ID_5314[];
extern u16 gMsg_ID_5350[];
extern u16 gMsg_ID_5360[];
extern u16 gMsg_ID_5380[];
extern u16 gMsg_ID_5400[];
extern u16 gMsg_ID_5410[];
extern u16 gMsg_ID_5420[];
extern u16 gMsg_ID_5430[];
extern u16 gMsg_ID_5460[];
extern u16 gMsg_ID_5470[];
extern u16 gMsg_ID_5473[];
extern u16 gMsg_ID_5474[];
extern u16 gMsg_ID_5475[];
extern u16 gMsg_ID_5492[];
extern u16 gMsg_ID_5493[];
extern u16 gMsg_ID_5494[];
extern u16 gMsg_ID_5495[];
extern u16 gMsg_ID_5496[];
extern u16 gMsg_ID_5497[];
extern u16 gMsg_ID_5498[];
extern u16 gMsg_ID_5499[];
extern u16 gMsg_ID_5500[];
extern u16 gMsg_ID_5501[];
extern u16 gMsg_ID_5502[];
extern u16 gMsg_ID_5503[];
extern u16 gMsg_ID_5504[];
extern u16 gMsg_ID_5505[];
extern u16 gMsg_ID_5506[];
extern u16 gMsg_ID_6010[];
extern u16 gMsg_ID_6011[];
extern u16 gMsg_ID_6012[];
extern u16 gMsg_ID_6013[];
extern u16 gMsg_ID_6014[];
extern u16 gMsg_ID_6020[];
extern u16 gMsg_ID_6021[];
extern u16 gMsg_ID_6024[];
extern u16 gMsg_ID_6025[];
extern u16 gMsg_ID_6026[];
extern u16 gMsg_ID_6027[];
extern u16 gMsg_ID_6028[];
extern u16 gMsg_ID_6029[];
extern u16 gMsg_ID_6036[];
extern u16 gMsg_ID_6038[];
extern u16 gMsg_ID_6041[];
extern u16 gMsg_ID_6042[];
extern u16 gMsg_ID_6045[];
extern u16 gMsg_ID_6050[];
extern u16 gMsg_ID_6051[];
extern u16 gMsg_ID_6055[];
extern u16 gMsg_ID_6066[];
extern u16 gMsg_ID_6067[];
extern u16 gMsg_ID_6068[];
extern u16 gMsg_ID_6069[];
extern u16 gMsg_ID_6071[];
extern u16 gMsg_ID_6072[];
extern u16 gMsg_ID_6073[];
extern u16 gMsg_ID_6074[];
extern u16 gMsg_ID_6075[];
extern u16 gMsg_ID_6076[];
extern u16 gMsg_ID_6077[];
extern u16 gMsg_ID_6078[];
extern u16 gMsg_ID_6079[];
extern u16 gMsg_ID_6080[];
extern u16 gMsg_ID_6081[];
extern u16 gMsg_ID_6082[];
extern u16 gMsg_ID_6090[];
extern u16 gMsg_ID_6100[];
extern u16 gMsg_ID_6101[];
extern u16 gMsg_ID_7005[];
extern u16 gMsg_ID_7006[];
extern u16 gMsg_ID_7011[];
extern u16 gMsg_ID_7012[];
extern u16 gMsg_ID_7013[];
extern u16 gMsg_ID_7014[];
extern u16 gMsg_ID_7020[];
extern u16 gMsg_ID_7043[];
extern u16 gMsg_ID_7050[];
extern u16 gMsg_ID_7051[];
extern u16 gMsg_ID_7052[];
extern u16 gMsg_ID_7053[];
extern u16 gMsg_ID_7054[];
extern u16 gMsg_ID_7061[];
extern u16 gMsg_ID_7064[];
extern u16 gMsg_ID_7065[];
extern u16 gMsg_ID_7066[];
extern u16 gMsg_ID_7070[];
extern u16 gMsg_ID_7083[];
extern u16 gMsg_ID_7084[];
extern u16 gMsg_ID_7085[];
extern u16 gMsg_ID_7086[];
extern u16 gMsg_ID_7087[];
extern u16 gMsg_ID_7093[];
extern u16 gMsg_ID_7094[];
extern u16 gMsg_ID_7095[];
extern u16 gMsg_ID_7096[];
extern u16 gMsg_ID_7097[];
extern u16 gMsg_ID_7098[];
extern u16 gMsg_ID_7099[];
extern u16 gMsg_ID_7100[];
extern u16 gMsg_ID_8010[];
extern u16 gMsg_ID_8020[];
extern u16 gMsg_ID_8030[];
extern u16 gMsg_ID_8040[];
extern u16 gMsg_ID_8045[];
extern u16 gMsg_ID_8050[];
extern u16 gMsg_ID_8060[];
extern u16 gMsg_ID_8070[];
extern u16 gMsg_ID_8080[];
extern u16 gMsg_ID_8100[];
extern u16 gMsg_ID_8110[];
extern u16 gMsg_ID_8120[];
extern u16 gMsg_ID_8130[];
extern u16 gMsg_ID_8140[];
extern u16 gMsg_ID_8205[];
extern u16 gMsg_ID_8210[];
extern u16 gMsg_ID_8215[];
extern u16 gMsg_ID_8220[];
extern u16 gMsg_ID_8230[];
extern u16 gMsg_ID_8240[];
extern u16 gMsg_ID_8250[];
extern u16 gMsg_ID_8255[];
extern u16 gMsg_ID_8260[];
extern u16 gMsg_ID_8265[];
extern u16 gMsg_ID_8300[];
extern u16 gMsg_ID_8310[];
extern u16 gMsg_ID_8320[];
extern u16 gMsg_ID_9000[];
extern u16 gMsg_ID_9010[];
extern u16 gMsg_ID_9100[];
extern u16 gMsg_ID_9110[];
extern u16 gMsg_ID_9120[];
extern u16 gMsg_ID_9130[];
extern u16 gMsg_ID_9140[];
extern u16 gMsg_ID_9150[];
extern u16 gMsg_ID_9151[];
extern u16 gMsg_ID_9152[];
extern u16 gMsg_ID_9153[];
extern u16 gMsg_ID_9160[];
extern u16 gMsg_ID_9170[];
extern u16 gMsg_ID_9180[];
extern u16 gMsg_ID_9190[];
extern u16 gMsg_ID_9200[];
extern u16 gMsg_ID_9210[];
extern u16 gMsg_ID_9211[];
extern u16 gMsg_ID_9212[];
extern u16 gMsg_ID_9213[];
extern u16 gMsg_ID_9220[];
extern u16 gMsg_ID_9230[];
extern u16 gMsg_ID_9240[];
extern u16 gMsg_ID_9250[];
extern u16 gMsg_ID_9260[];
extern u16 gMsg_ID_9270[];
extern u16 gMsg_ID_9275[];
extern u16 gMsg_ID_9280[];
extern u16 gMsg_ID_9285[];
extern u16 gMsg_ID_9289[];
extern u16 gMsg_ID_9290[];
extern u16 gMsg_ID_9300[];
extern u16 gMsg_ID_9310[];
extern u16 gMsg_ID_9320[];
extern u16 gMsg_ID_9322[];
extern u16 gMsg_ID_9323[];
extern u16 gMsg_ID_9324[];
extern u16 gMsg_ID_9325[];
extern u16 gMsg_ID_9330[];
extern u16 gMsg_ID_9340[];
extern u16 gMsg_ID_9350[];
extern u16 gMsg_ID_9360[];
extern u16 gMsg_ID_9365[];
extern u16 gMsg_ID_9366[];
extern u16 gMsg_ID_9367[];
extern u16 gMsg_ID_9368[];
extern u16 gMsg_ID_9369[];
extern u16 gMsg_ID_9375[];
extern u16 gMsg_ID_9380[];
extern u16 gMsg_ID_9385[];
extern u16 gMsg_ID_9390[];
extern u16 gMsg_ID_9395[];
extern u16 gMsg_ID_9400[];
extern u16 gMsg_ID_9405[];
extern u16 gMsg_ID_9411[];
extern u16 gMsg_ID_9420[];
extern u16 gMsg_ID_9425[];
extern u16 gMsg_ID_9426[];
extern u16 gMsg_ID_9427[];
extern u16 gMsg_ID_9428[];
extern u16 gMsg_ID_9429[];
extern u16 gMsg_ID_9430[];
extern u16 gMsg_ID_9431[];
extern u16 gMsg_ID_9432[];
extern u16 gMsg_ID_9433[];
extern u16 gMsg_ID_9434[];
extern u16 gMsg_ID_9436[];
extern u16 gMsg_ID_9437[];
extern u16 gMsg_ID_9438[];
extern u16 gMsg_ID_10010[];
extern u16 gMsg_ID_10020[];
extern u16 gMsg_ID_10040[];
extern u16 gMsg_ID_10050[];
extern u16 gMsg_ID_10060[];
extern u16 gMsg_ID_10070[];
extern u16 gMsg_ID_10080[];
extern u16 gMsg_ID_10200[];
extern u16 gMsg_ID_10210[];
extern u16 gMsg_ID_10220[];
extern u16 gMsg_ID_10230[];
extern u16 gMsg_ID_10255[];
extern u16 gMsg_ID_10300[];
extern u16 gMsg_ID_10310[];
extern u16 gMsg_ID_10320[];
extern u16 gMsg_ID_10321[];
extern u16 gMsg_ID_10322[];
extern u16 gMsg_ID_10323[];
extern u16 gMsg_ID_10324[];
extern u16 gMsg_ID_11010[];
extern u16 gMsg_ID_11020[];
extern u16 gMsg_ID_11030[];
extern u16 gMsg_ID_11040[];
extern u16 gMsg_ID_11050[];
extern u16 gMsg_ID_11060[];
extern u16 gMsg_ID_11100[];
extern u16 gMsg_ID_11110[];
extern u16 gMsg_ID_11120[];
extern u16 gMsg_ID_11130[];
extern u16 gMsg_ID_11150[];
extern u16 gMsg_ID_11160[];
extern u16 gMsg_ID_11200[];
extern u16 gMsg_ID_11210[];
extern u16 gMsg_ID_11220[];
extern u16 gMsg_ID_11230[];
extern u16 gMsg_ID_11240[];
extern u16 gMsg_ID_11241[];
extern u16 gMsg_ID_14020[];
extern u16 gMsg_ID_14030[];
extern u16 gMsg_ID_14040[];
extern u16 gMsg_ID_14045[];
extern u16 gMsg_ID_14050[];
extern u16 gMsg_ID_14060[];
extern u16 gMsg_ID_14070[];
extern u16 gMsg_ID_14080[];
extern u16 gMsg_ID_14100[];
extern u16 gMsg_ID_14110[];
extern u16 gMsg_ID_14120[];
extern u16 gMsg_ID_14130[];
extern u16 gMsg_ID_14140[];
extern u16 gMsg_ID_14150[];
extern u16 gMsg_ID_14160[];
extern u16 gMsg_ID_14170[];
extern u16 gMsg_ID_14180[];
extern u16 gMsg_ID_14190[];
extern u16 gMsg_ID_14200[];
extern u16 gMsg_ID_14210[];
extern u16 gMsg_ID_14220[];
extern u16 gMsg_ID_14230[];
extern u16 gMsg_ID_14300[];
extern u16 gMsg_ID_14310[];
extern u16 gMsg_ID_14320[];
extern u16 gMsg_ID_14330[];
extern u16 gMsg_ID_14340[];
extern u16 gMsg_ID_14350[];
extern u16 gMsg_ID_14360[];
extern u16 gMsg_ID_14370[];
extern u16 gMsg_ID_15010[];
extern u16 gMsg_ID_15030[];
extern u16 gMsg_ID_15040[];
extern u16 gMsg_ID_15045[];
extern u16 gMsg_ID_15050[];
extern u16 gMsg_ID_15051[];
extern u16 gMsg_ID_15052[];
extern u16 gMsg_ID_15053[];
extern u16 gMsg_ID_15054[];
extern u16 gMsg_ID_15060[];
extern u16 gMsg_ID_15100[];
extern u16 gMsg_ID_15110[];
extern u16 gMsg_ID_15120[];
extern u16 gMsg_ID_15130[];
extern u16 gMsg_ID_15140[];
extern u16 gMsg_ID_15200[];
extern u16 gMsg_ID_15210[];
extern u16 gMsg_ID_15220[];
extern u16 gMsg_ID_15230[];
extern u16 gMsg_ID_15240[];
extern u16 gMsg_ID_15250[];
extern u16 gMsg_ID_15251[];
extern u16 gMsg_ID_15252[];
extern u16 gMsg_ID_15253[];
extern u16 gMsg_ID_15254[];
extern u16 gMsg_ID_16010[];
extern u16 gMsg_ID_16020[];
extern u16 gMsg_ID_16030[];
extern u16 gMsg_ID_16040[];
extern u16 gMsg_ID_16046[];
extern u16 gMsg_ID_16047[];
extern u16 gMsg_ID_16050[];
extern u16 gMsg_ID_16055[];
extern u16 gMsg_ID_16060[];
extern u16 gMsg_ID_16080[];
extern u16 gMsg_ID_16085[];
extern u16 gMsg_ID_16090[];
extern u16 gMsg_ID_16100[];
extern u16 gMsg_ID_16110[];
extern u16 gMsg_ID_16120[];
extern u16 gMsg_ID_16125[];
extern u16 gMsg_ID_16130[];
extern u16 gMsg_ID_16135[];
extern u16 gMsg_ID_16140[];
extern u16 gMsg_ID_16150[];
extern u16 gMsg_ID_16160[];
extern u16 gMsg_ID_16165[];
extern u16 gMsg_ID_16170[];
extern u16 gMsg_ID_16175[];
extern u16 gMsg_ID_16180[];
extern u16 gMsg_ID_16185[];
extern u16 gMsg_ID_16200[];
extern u16 gMsg_ID_16210[];
extern u16 gMsg_ID_16220[];
extern u16 gMsg_ID_16230[];
extern u16 gMsg_ID_16240[];
extern u16 gMsg_ID_16250[];
extern u16 gMsg_ID_16260[];
extern u16 gMsg_ID_16270[];
extern u16 gMsg_ID_16280[];
extern u16 gMsg_ID_17010[];
extern u16 gMsg_ID_17020[];
extern u16 gMsg_ID_17030[];
extern u16 gMsg_ID_17100[];
extern u16 gMsg_ID_17110[];
extern u16 gMsg_ID_17120[];
extern u16 gMsg_ID_17130[];
extern u16 gMsg_ID_17131[];
extern u16 gMsg_ID_17140[];
extern u16 gMsg_ID_17150[];
extern u16 gMsg_ID_17160[];
extern u16 gMsg_ID_17170[];
extern u16 gMsg_ID_17300[];
extern u16 gMsg_ID_17310[];
extern u16 gMsg_ID_17320[];
extern u16 gMsg_ID_17330[];
extern u16 gMsg_ID_17350[];
extern u16 gMsg_ID_17360[];
extern u16 gMsg_ID_17370[];
extern u16 gMsg_ID_17380[];
extern u16 gMsg_ID_17390[];
extern u16 gMsg_ID_17400[];
extern u16 gMsg_ID_17410[];
extern u16 gMsg_ID_17420[];
extern u16 gMsg_ID_17430[];
extern u16 gMsg_ID_17440[];
extern u16 gMsg_ID_17450[];
extern u16 gMsg_ID_17460[];
extern u16 gMsg_ID_17470[];
extern u16 gMsg_ID_17471[];
extern u16 gMsg_ID_17472[];
extern u16 gMsg_ID_17473[];
extern u16 gMsg_ID_17474[];
extern u16 gMsg_ID_17475[];
extern u16 gMsg_ID_17476[];
extern u16 gMsg_ID_18000[];
extern u16 gMsg_ID_18005[];
extern u16 gMsg_ID_18006[];
extern u16 gMsg_ID_18007[];
extern u16 gMsg_ID_18010[];
extern u16 gMsg_ID_18015[];
extern u16 gMsg_ID_18018[];
extern u16 gMsg_ID_18020[];
extern u16 gMsg_ID_18021[];
extern u16 gMsg_ID_18022[];
extern u16 gMsg_ID_18025[];
extern u16 gMsg_ID_18030[];
extern u16 gMsg_ID_18031[];
extern u16 gMsg_ID_18035[];
extern u16 gMsg_ID_18040[];
extern u16 gMsg_ID_18045[];
extern u16 gMsg_ID_18050[];
extern u16 gMsg_ID_18055[];
extern u16 gMsg_ID_18060[];
extern u16 gMsg_ID_18065[];
extern u16 gMsg_ID_18066[];
extern u16 gMsg_ID_18070[];
extern u16 gMsg_ID_18075[];
extern u16 gMsg_ID_18080[];
extern u16 gMsg_ID_18085[];
extern u16 gMsg_ID_18090[];
extern u16 gMsg_ID_18095[];
extern u16 gMsg_ID_18100[];
extern u16 gMsg_ID_18105[];
extern u16 gMsg_ID_18120[];
extern u16 gMsg_ID_18130[];
extern u16 gMsg_ID_18140[];
extern u16 gMsg_ID_18150[];
extern u16 gMsg_ID_19010[];
extern u16 gMsg_ID_19200[];
extern u16 gMsg_ID_19205[];
extern u16 gMsg_ID_19210[];
extern u16 gMsg_ID_19220[];
extern u16 gMsg_ID_19230[];
extern u16 gMsg_ID_19240[];
extern u16 gMsg_ID_19250[];
extern u16 gMsg_ID_19325[];
extern u16 gMsg_ID_19330[];
extern u16 gMsg_ID_19335[];
extern u16 gMsg_ID_19340[];
extern u16 gMsg_ID_19350[];
extern u16 gMsg_ID_19355[];
extern u16 gMsg_ID_19360[];
extern u16 gMsg_ID_19370[];
extern u16 gMsg_ID_19400[];
extern u16 gMsg_ID_19410[];
extern u16 gMsg_ID_19420[];
extern u16 gMsg_ID_19430[];
extern u16 gMsg_ID_19440[];
extern u16 gMsg_ID_19450[];
extern u16 gMsg_ID_19451[];
extern u16 gMsg_ID_19452[];
extern u16 gMsg_ID_19453[];
extern u16 gMsg_ID_19454[];
extern u16 gMsg_ID_19455[];
extern u16 gMsg_ID_19456[];
extern u16 gMsg_ID_19457[];
extern u16 gMsg_ID_19458[];
extern u16 gMsg_ID_19459[];
extern u16 gMsg_ID_19460[];
extern u16 gMsg_ID_19461[];
extern u16 gMsg_ID_19462[];
extern u16 gMsg_ID_19463[];
extern u16 gMsg_ID_19464[];
extern u16 gMsg_ID_19465[];
extern u16 gMsg_ID_19466[];
extern u16 gMsg_ID_19467[];
extern u16 gMsg_ID_19468[];
extern u16 gMsg_ID_20010[];
extern u16 gMsg_ID_20011[];
extern u16 gMsg_ID_20012[];
extern u16 gMsg_ID_20013[];
extern u16 gMsg_ID_20014[];
extern u16 gMsg_ID_20015[];
extern u16 gMsg_ID_20016[];
extern u16 gMsg_ID_20017[];
extern u16 gMsg_ID_20018[];
extern u16 gMsg_ID_20019[];
extern u16 gMsg_ID_20020[];
extern u16 gMsg_ID_20030[];
extern u16 gMsg_ID_20040[];
extern u16 gMsg_ID_20050[];
extern u16 gMsg_ID_20060[];
extern u16 gMsg_ID_20070[];
extern u16 gMsg_ID_20080[];
extern u16 gMsg_ID_20084[];
extern u16 gMsg_ID_20085[];
extern u16 gMsg_ID_20090[];
extern u16 gMsg_ID_20091[];
extern u16 gMsg_ID_20092[];
extern u16 gMsg_ID_20150[];
extern u16 gMsg_ID_20160[];
extern u16 gMsg_ID_20170[];
extern u16 gMsg_ID_20180[];
extern u16 gMsg_ID_20190[];
extern u16 gMsg_ID_20200[];
extern u16 gMsg_ID_20210[];
extern u16 gMsg_ID_20220[];
extern u16 gMsg_ID_20221[];
extern u16 gMsg_ID_20222[];
extern u16 gMsg_ID_20230[];
extern u16 gMsg_ID_20235[];
extern u16 gMsg_ID_20236[];
extern u16 gMsg_ID_20237[];
extern u16 gMsg_ID_20238[];
extern u16 gMsg_ID_20239[];
extern u16 gMsg_ID_20250[];
extern u16 gMsg_ID_20260[];
extern u16 gMsg_ID_20261[];
extern u16 gMsg_ID_20262[];
extern u16 gMsg_ID_20263[];
extern u16 gMsg_ID_20264[];
extern u16 gMsg_ID_20265[];
extern u16 gMsg_ID_20266[];
extern u16 gMsg_ID_20267[];
extern u16 gMsg_ID_20268[];
extern u16 gMsg_ID_20269[];
extern u16 gMsg_ID_20270[];
extern u16 gMsg_ID_20271[];
extern u16 gMsg_ID_20272[];
extern u16 gMsg_ID_20273[];
extern u16 gMsg_ID_20274[];
extern u16 gMsg_ID_20275[];
extern u16 gMsg_ID_20276[];
extern u16 gMsg_ID_20277[];
extern u16 gMsg_ID_20278[];
extern u16 gMsg_ID_20279[];
extern u16 gMsg_ID_20280[];
extern u16 gMsg_ID_20281[];
extern u16 gMsg_ID_20282[];
extern u16 gMsg_ID_20283[];
extern u16 gMsg_ID_20284[];
extern u16 gMsg_ID_20285[];
extern u16 gMsg_ID_20286[];
extern u16 gMsg_ID_20287[];
extern u16 gMsg_ID_20288[];
extern u16 gMsg_ID_20289[];
extern u16 gMsg_ID_20290[];
extern u16 gMsg_ID_20291[];
extern u16 gMsg_ID_20292[];
extern u16 gMsg_ID_20294[];
extern u16 gMsg_ID_20296[];
extern u16 gMsg_ID_20297[];
extern u16 gMsg_ID_20298[];
extern u16 gMsg_ID_20299[];
extern u16 gMsg_ID_20300[];
extern u16 gMsg_ID_20301[];
extern u16 gMsg_ID_20302[];
extern u16 gMsg_ID_20303[];
extern u16 gMsg_ID_20304[];
extern u16 gMsg_ID_20305[];
extern u16 gMsg_ID_20306[];
extern u16 gMsg_ID_20307[];
extern u16 gMsg_ID_20308[];
extern u16 gMsg_ID_20309[];
extern u16 gMsg_ID_20310[];
extern u16 gMsg_ID_20311[];
extern u16 gMsg_ID_20312[];
extern u16 gMsg_ID_20313[];
extern u16 gMsg_ID_20314[];
extern u16 gMsg_ID_20315[];
extern u16 gMsg_ID_20316[];
extern u16 gMsg_ID_20317[];
extern u16 gMsg_ID_20318[];
extern u16 gMsg_ID_20319[];
extern u16 gMsg_ID_20320[];
extern u16 gMsg_ID_20321[];
extern u16 gMsg_ID_20326[];
extern u16 gMsg_ID_20327[];
extern u16 gMsg_ID_20328[];
extern u16 gMsg_ID_20329[];
extern u16 gMsg_ID_20330[];
extern u16 gMsg_ID_20331[];
extern u16 gMsg_ID_20332[];
extern u16 gMsg_ID_20333[];
extern u16 gMsg_ID_20337[];
extern u16 gMsg_ID_20338[];
extern u16 gMsg_ID_20339[];
extern u16 gMsg_ID_20340[];
extern u16 gMsg_ID_20343[];
extern u16 gMsg_ID_20344[];
extern u16 gMsg_ID_20345[];
extern u16 gMsg_ID_21010[];
extern u16 gMsg_ID_21020[];
extern u16 gMsg_ID_21030[];
extern u16 gMsg_ID_21050[];
extern u16 gMsg_ID_21060[];
extern u16 gMsg_ID_21070[];
extern u16 gMsg_ID_21071[];
extern u16 gMsg_ID_21072[];
extern u16 gMsg_ID_21073[];
extern u16 gMsg_ID_21080[];
extern u16 gMsg_ID_21081[];
extern u16 gMsg_ID_21082[];
extern u16 gMsg_ID_21083[];
extern u16 gMsg_ID_21090[];
extern u16 gMsg_ID_21091[];
extern u16 gMsg_ID_21092[];
extern u16 gMsg_ID_21093[];
extern u16 gMsg_ID_22000[];
extern u16 gMsg_ID_22001[];
extern u16 gMsg_ID_22002[];
extern u16 gMsg_ID_22003[];
extern u16 gMsg_ID_22004[];
extern u16 gMsg_ID_22005[];
extern u16 gMsg_ID_22006[];
extern u16 gMsg_ID_22007[];
extern u16 gMsg_ID_22008[];
extern u16 gMsg_ID_22009[];
extern u16 gMsg_ID_22010[];
extern u16 gMsg_ID_22011[];
extern u16 gMsg_ID_22012[];
extern u16 gMsg_ID_22013[];
extern u16 gMsg_ID_22014[];
extern u16 gMsg_ID_22015[];
extern u16 gMsg_ID_22016[];
extern u16 gMsg_ID_22017[];
extern u16 gMsg_ID_22018[];
extern u16 gMsg_ID_22019[];
extern u16 gMsg_ID_22020[];
extern u16 gMsg_ID_23000[];
extern u16 gMsg_ID_23001[];
extern u16 gMsg_ID_23002[];
extern u16 gMsg_ID_23003[];
extern u16 gMsg_ID_23004[];
extern u16 gMsg_ID_23005[];
extern u16 gMsg_ID_23006[];
extern u16 gMsg_ID_23007[];
extern u16 gMsg_ID_23008[];
extern u16 gMsg_ID_23009[];
extern u16 gMsg_ID_23010[];
extern u16 gMsg_ID_23011[];
extern u16 gMsg_ID_23012[];
extern u16 gMsg_ID_23013[];
extern u16 gMsg_ID_23014[];
extern u16 gMsg_ID_23015[];
extern u16 gMsg_ID_23016[];
extern u16 gMsg_ID_23017[];
extern u16 gMsg_ID_23018[];
extern u16 gMsg_ID_23019[];
extern u16 gMsg_ID_23020[];
extern u16 gMsg_ID_23021[];
extern u16 gMsg_ID_23022[];
extern u16 gMsg_ID_23023[];
extern u16 gMsg_ID_23024[];
extern u16 gMsg_ID_23025[];
extern u16 gMsg_ID_23026[];
extern u16 gMsg_ID_23027[];
extern u16 gMsg_ID_23028[];
extern u16 gMsg_ID_23029[];
extern u16 gMsg_ID_23030[];
extern u16 gMsg_ID_23031[];
extern u16 gMsg_ID_23032[];

static volatile u16 sf64_ru_blob_1[SF64_RU_BLOB_HEADER_WORDS + 914] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0001, 0x0392, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x000A, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0014, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_30[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x001E, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_40[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0028, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_50[SF64_RU_BLOB_HEADER_WORDS + 51] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0032, 0x0033, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_60[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x003C, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1200[SF64_RU_BLOB_HEADER_WORDS + 73] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04B0, 0x0049, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1210[SF64_RU_BLOB_HEADER_WORDS + 54] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04BA, 0x0036, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1220[SF64_RU_BLOB_HEADER_WORDS + 75] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04C4, 0x004B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1230[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04CE, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1240[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04D8, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1250[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04E2, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1260[SF64_RU_BLOB_HEADER_WORDS + 65] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04EC, 0x0041, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1270[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x04F6, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1280[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0500, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1290[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x050A, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1300[SF64_RU_BLOB_HEADER_WORDS + 78] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0514, 0x004E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1310[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x051E, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1320[SF64_RU_BLOB_HEADER_WORDS + 68] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0528, 0x0044, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1330[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0532, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1340[SF64_RU_BLOB_HEADER_WORDS + 64] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x053C, 0x0040, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1350[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0546, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1360[SF64_RU_BLOB_HEADER_WORDS + 54] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0550, 0x0036, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1370[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x055A, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1380[SF64_RU_BLOB_HEADER_WORDS + 56] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0564, 0x0038, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1390[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x056E, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1400[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0578, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1410[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0582, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1420[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x058C, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1430[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0596, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1440[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x05A0, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1450[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x05AA, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1460[SF64_RU_BLOB_HEADER_WORDS + 73] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x05B4, 0x0049, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_1470[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x05BE, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2005[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x07D5, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2010[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x07DA, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2020[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x07E4, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2030[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x07EE, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2040[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x07F8, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2050[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0802, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2055[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0807, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2058[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x080A, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2061[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x080D, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2062[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x080E, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2080[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0820, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2090[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x082A, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2095[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x082F, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2110[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x083E, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2115[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0843, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2118[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0846, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2140[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x085C, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2165[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0875, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2166[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0876, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2167[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0877, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2180[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0884, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2181[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0885, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2185[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0889, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2188[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x088C, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2200[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0898, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2210[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08A2, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2220[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08AC, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2225[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08B1, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2230[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08B6, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2233[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08B9, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2240[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08C0, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2250[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08CA, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2260[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08D4, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2263[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08D7, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2265[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08D9, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2270[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08DE, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2275[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08E3, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2280[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08E8, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2282[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08EA, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2290[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F2, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2291[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F3, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2292[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F4, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2293[SF64_RU_BLOB_HEADER_WORDS + 7] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F5, 0x0007, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2294[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F6, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2295[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F7, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2296[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08F8, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2298[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08FA, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2299[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08FB, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2300[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x08FC, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2305[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0901, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2310[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0906, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2320[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0910, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2335[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x091F, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2336[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0920, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_2337[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0921, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3005[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BBD, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3010[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BC2, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3015[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BC7, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3020[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BCC, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3025[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BD1, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3026[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BD2, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3040[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BE0, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3041[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BE1, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3042[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BE2, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3050[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0BEA, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3100[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0C1C, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3110[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0C26, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3120[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0C30, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3300[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CE4, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3310[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CEE, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3315[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CF3, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3320[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CF8, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3321[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CF9, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3322[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0CFA, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3330[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D02, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3340[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D0C, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3345[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D11, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3350[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D16, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3360[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D20, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3370[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D2A, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_3371[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0D2B, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4010[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FAA, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4011[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FAB, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4012[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FAC, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4013[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FAD, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4020[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FB4, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4021[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FB5, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4022[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FB6, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4023[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FB7, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4024[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FB8, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4030[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FBE, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4031[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FBF, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4040[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FC8, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4050[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FD2, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4075[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FEB, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4080[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FF0, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4082[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FF2, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4083[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FF3, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4091[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FFB, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4092[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FFC, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4093[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FFD, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4094[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FFE, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4095[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x0FFF, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4096[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1000, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4097[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1001, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4098[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1002, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4099[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1003, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4100[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1004, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4101[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1005, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4102[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1006, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4103[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1007, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4110[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x100E, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4111[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x100F, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4112[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1010, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_4113[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1011, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5000[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1388, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5010[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1392, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5060[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x13C4, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5080[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x13D8, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5100[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x13EC, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5110[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x13F6, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5130[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x140A, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5220[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1464, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5230[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x146E, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5300[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14B4, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5310[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14BE, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5311[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14BF, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5312[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14C0, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5313[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14C1, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5314[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14C2, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5350[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14E6, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5360[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x14F0, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5380[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1504, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5400[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1518, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5410[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1522, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5420[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x152C, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5430[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1536, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5460[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1554, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5470[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x155E, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5473[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1561, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5474[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1562, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5475[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1563, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5492[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1574, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5493[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1575, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5494[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1576, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5495[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1577, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5496[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1578, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5497[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1579, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5498[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157A, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5499[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157B, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5500[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157C, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5501[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157D, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5502[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157E, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5503[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x157F, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5504[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1580, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5505[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1581, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_5506[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1582, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6010[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x177A, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6011[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x177B, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6012[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x177C, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6013[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x177D, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6014[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x177E, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6020[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1784, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6021[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1785, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6024[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1788, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6025[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1789, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6026[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x178A, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6027[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x178B, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6028[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x178C, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6029[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x178D, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6036[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1794, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6038[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1796, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6041[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1799, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6042[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x179A, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6045[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x179D, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6050[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17A2, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6051[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17A3, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6055[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17A7, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6066[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B2, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6067[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B3, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6068[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B4, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6069[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B5, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6071[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B7, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6072[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B8, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6073[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17B9, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6074[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BA, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6075[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BB, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6076[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BC, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6077[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BD, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6078[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BE, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6079[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17BF, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6080[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17C0, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6081[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17C1, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6082[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17C2, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6090[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17CA, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6100[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17D4, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_6101[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x17D5, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7005[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B5D, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7006[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B5E, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7011[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B63, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7012[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B64, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7013[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B65, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7014[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B66, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7020[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B6C, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7043[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B83, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7050[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B8A, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7051[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B8B, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7052[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B8C, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7053[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B8D, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7054[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B8E, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7061[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B95, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7064[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B98, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7065[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B99, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7066[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B9A, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7070[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1B9E, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7083[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BAB, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7084[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BAC, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7085[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BAD, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7086[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BAE, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7087[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BAF, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7093[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BB5, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7094[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BB6, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7095[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BB7, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7096[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BB8, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7097[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BB9, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7098[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BBA, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7099[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BBB, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_7100[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1BBC, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8010[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F4A, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8020[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F54, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8030[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F5E, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8040[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F68, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8045[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F6D, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8050[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F72, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8060[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F7C, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8070[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F86, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8080[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1F90, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8100[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1FA4, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8110[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1FAE, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8120[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1FB8, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8130[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1FC2, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8140[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x1FCC, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8205[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x200D, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8210[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2012, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8215[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2017, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8220[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x201C, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8230[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2026, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8240[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2030, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8250[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x203A, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8255[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x203F, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8260[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2044, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8265[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2049, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8300[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x206C, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8310[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2076, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_8320[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2080, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9000[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2328, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9010[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2332, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9100[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x238C, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9110[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2396, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9120[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23A0, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9130[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23AA, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9140[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23B4, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9150[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23BE, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9151[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23BF, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9152[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23C0, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9153[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23C1, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9160[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23C8, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9170[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23D2, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9180[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23DC, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9190[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23E6, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9200[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23F0, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9210[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23FA, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9211[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23FB, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9212[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23FC, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9213[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x23FD, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9220[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2404, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9230[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x240E, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9240[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2418, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9250[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2422, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9260[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x242C, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9270[SF64_RU_BLOB_HEADER_WORDS + 46] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2436, 0x002E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9275[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x243B, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9280[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2440, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9285[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2445, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9289[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2449, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9290[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x244A, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9300[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2454, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9310[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x245E, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9320[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2468, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9322[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x246A, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9323[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x246B, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9324[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x246C, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9325[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x246D, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9330[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2472, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9340[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x247C, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9350[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2486, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9360[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2490, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9365[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2495, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9366[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2496, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9367[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2497, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9368[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2498, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9369[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2499, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9375[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x249F, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9380[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24A4, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9385[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24A9, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9390[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24AE, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9395[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24B3, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9400[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24B8, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9405[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24BD, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9411[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24C3, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9420[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24CC, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9425[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D1, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9426[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D2, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9427[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D3, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9428[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D4, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9429[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D5, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9430[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D6, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9431[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D7, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9432[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D8, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9433[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24D9, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9434[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24DA, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9436[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24DC, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9437[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24DD, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_9438[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x24DE, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10010[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x271A, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10020[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2724, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10040[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2738, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10050[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2742, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10060[SF64_RU_BLOB_HEADER_WORDS + 54] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x274C, 0x0036, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10070[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2756, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10080[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2760, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10200[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x27D8, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10210[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x27E2, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10220[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x27EC, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10230[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x27F6, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10255[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x280F, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10300[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x283C, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10310[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2846, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10320[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2850, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10321[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2851, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10322[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2852, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10323[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2853, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_10324[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2854, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11010[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B02, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11020[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B0C, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11030[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B16, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11040[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B20, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11050[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B2A, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11060[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B34, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11100[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B5C, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11110[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B66, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11120[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B70, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11130[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B7A, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11150[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B8E, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11160[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2B98, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11200[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BC0, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11210[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BCA, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11220[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BD4, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11230[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BDE, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11240[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BE8, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_11241[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x2BE9, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14020[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36C4, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14030[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36CE, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14040[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36D8, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14045[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36DD, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14050[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36E2, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14060[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36EC, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14070[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x36F6, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14080[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3700, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14100[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3714, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14110[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x371E, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14120[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3728, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14130[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3732, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14140[SF64_RU_BLOB_HEADER_WORDS + 46] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x373C, 0x002E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14150[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3746, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14160[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3750, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14170[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x375A, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14180[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3764, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14190[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x376E, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14200[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3778, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14210[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3782, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14220[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x378C, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14230[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3796, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14300[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x37DC, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14310[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x37E6, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14320[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x37F0, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14330[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x37FA, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14340[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3804, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14350[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x380E, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14360[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3818, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_14370[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3822, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15010[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AA2, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15030[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AB6, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15040[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AC0, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15045[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AC5, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15050[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ACA, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15051[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ACB, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15052[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ACC, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15053[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ACD, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15054[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ACE, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15060[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AD4, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15100[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3AFC, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15110[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B06, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15120[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B10, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15130[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B1A, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15140[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B24, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15200[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B60, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15210[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B6A, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15220[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B74, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15230[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B7E, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15240[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B88, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15250[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B92, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15251[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B93, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15252[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B94, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15253[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B95, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_15254[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3B96, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16010[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3E8A, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16020[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3E94, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16030[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3E9E, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16040[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EA8, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16046[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EAE, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16047[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EAF, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16050[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EB2, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16055[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EB7, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16060[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EBC, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16080[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ED0, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16085[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3ED5, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16090[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EDA, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16100[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EE4, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16110[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EEE, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16120[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EF8, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16125[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3EFD, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16130[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F02, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16135[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F07, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16140[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F0C, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16150[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F16, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16160[SF64_RU_BLOB_HEADER_WORDS + 46] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F20, 0x002E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16165[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F25, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16170[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F2A, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16175[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F2F, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16180[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F34, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16185[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F39, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16200[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F48, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16210[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F52, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16220[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F5C, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16230[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F66, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16240[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F70, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16250[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F7A, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16260[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F84, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16270[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F8E, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_16280[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x3F98, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17010[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4272, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17020[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x427C, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17030[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4286, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17100[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42CC, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17110[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42D6, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17120[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42E0, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17130[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42EA, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17131[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42EB, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17140[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42F4, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17150[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x42FE, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17160[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4308, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17170[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4312, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17300[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4394, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17310[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x439E, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17320[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43A8, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17330[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43B2, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17350[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43C6, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17360[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43D0, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17370[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43DA, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17380[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43E4, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17390[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43EE, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17400[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x43F8, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17410[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4402, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17420[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x440C, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17430[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4416, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17440[SF64_RU_BLOB_HEADER_WORDS + 11] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4420, 0x000B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17450[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x442A, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17460[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4434, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17470[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x443E, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17471[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x443F, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17472[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4440, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17473[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4441, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17474[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4442, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17475[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4443, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_17476[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4444, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18000[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4650, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18005[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4655, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18006[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4656, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18007[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4657, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18010[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x465A, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18015[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x465F, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18018[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4662, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18020[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4664, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18021[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4665, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18022[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4666, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18025[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4669, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18030[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x466E, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18031[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x466F, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18035[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4673, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18040[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4678, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18045[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x467D, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18050[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4682, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18055[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4687, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18060[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x468C, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18065[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4691, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18066[SF64_RU_BLOB_HEADER_WORDS + 25] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4692, 0x0019, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18070[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4696, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18075[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x469B, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18080[SF64_RU_BLOB_HEADER_WORDS + 15] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46A0, 0x000F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18085[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46A5, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18090[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46AA, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18095[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46AF, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18100[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46B4, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18105[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46B9, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18120[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46C8, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18130[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46D2, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18140[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46DC, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_18150[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x46E6, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19010[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4A42, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19200[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B00, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19205[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B05, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19210[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B0A, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19220[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B14, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19230[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B1E, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19240[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B28, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19250[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B32, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19325[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B7D, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19330[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B82, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19335[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B87, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19340[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B8C, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19350[SF64_RU_BLOB_HEADER_WORDS + 11] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B96, 0x000B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19355[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4B9B, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19360[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BA0, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19370[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BAA, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19400[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BC8, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19410[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BD2, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19420[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BDC, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19430[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BE6, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19440[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BF0, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19450[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFA, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19451[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFB, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19452[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFC, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19453[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFD, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19454[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFE, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19455[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4BFF, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19456[SF64_RU_BLOB_HEADER_WORDS + 47] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C00, 0x002F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19457[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C01, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19458[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C02, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19459[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C03, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19460[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C04, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19461[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C05, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19462[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C06, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19463[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C07, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19464[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C08, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19465[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C09, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19466[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C0A, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19467[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C0B, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_19468[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4C0C, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20010[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2A, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20011[SF64_RU_BLOB_HEADER_WORDS + 22] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2B, 0x0016, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20012[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2C, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20013[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2D, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20014[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2E, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20015[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E2F, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20016[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E30, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20017[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E31, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20018[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E32, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20019[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E33, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20020[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E34, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20030[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E3E, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20040[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E48, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20050[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E52, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20060[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E5C, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20070[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E66, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20080[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E70, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20084[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E74, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20085[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E75, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20090[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E7A, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20091[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E7B, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20092[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4E7C, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20150[SF64_RU_BLOB_HEADER_WORDS + 20] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EB6, 0x0014, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20160[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EC0, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20170[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4ECA, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20180[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4ED4, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20190[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EDE, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20200[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EE8, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20210[SF64_RU_BLOB_HEADER_WORDS + 21] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EF2, 0x0015, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20220[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EFC, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20221[SF64_RU_BLOB_HEADER_WORDS + 46] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EFD, 0x002E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20222[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4EFE, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20230[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F06, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20235[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F0B, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20236[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F0C, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20237[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F0D, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20238[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F0E, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20239[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F0F, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20250[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F1A, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20260[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F24, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20261[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F25, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20262[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F26, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20263[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F27, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20264[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F28, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20265[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F29, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20266[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2A, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20267[SF64_RU_BLOB_HEADER_WORDS + 26] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2B, 0x001A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20268[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2C, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20269[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2D, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20270[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2E, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20271[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F2F, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20272[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F30, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20273[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F31, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20274[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F32, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20275[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F33, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20276[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F34, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20277[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F35, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20278[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F36, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20279[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F37, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20280[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F38, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20281[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F39, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20282[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3A, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20283[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3B, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20284[SF64_RU_BLOB_HEADER_WORDS + 34] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3C, 0x0022, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20285[SF64_RU_BLOB_HEADER_WORDS + 45] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3D, 0x002D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20286[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3E, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20287[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F3F, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20288[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F40, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20289[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F41, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20290[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F42, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20291[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F43, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20292[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F44, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20294[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F46, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20296[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F48, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20297[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F49, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20298[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4A, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20299[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4B, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20300[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4C, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20301[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4D, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20302[SF64_RU_BLOB_HEADER_WORDS + 23] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4E, 0x0017, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20303[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F4F, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20304[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F50, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20305[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F51, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20306[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F52, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20307[SF64_RU_BLOB_HEADER_WORDS + 17] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F53, 0x0011, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20308[SF64_RU_BLOB_HEADER_WORDS + 27] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F54, 0x001B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20309[SF64_RU_BLOB_HEADER_WORDS + 28] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F55, 0x001C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20310[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F56, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20311[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F57, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20312[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F58, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20313[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F59, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20314[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5A, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20315[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5B, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20316[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5C, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20317[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5D, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20318[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5E, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20319[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F5F, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20320[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F60, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20321[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F61, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20326[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F66, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20327[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F67, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20328[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F68, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20329[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F69, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20330[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F6A, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20331[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F6B, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20332[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F6C, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20333[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F6D, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20337[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F71, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20338[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F72, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20339[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F73, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20340[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F74, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20343[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F77, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20344[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F78, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_20345[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x4F79, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21010[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5212, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21020[SF64_RU_BLOB_HEADER_WORDS + 64] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x521C, 0x0040, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21030[SF64_RU_BLOB_HEADER_WORDS + 50] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5226, 0x0032, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21050[SF64_RU_BLOB_HEADER_WORDS + 29] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x523A, 0x001D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21060[SF64_RU_BLOB_HEADER_WORDS + 31] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5244, 0x001F, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21070[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x524E, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21071[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x524F, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21072[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5250, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21073[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5251, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21080[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5258, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21081[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5259, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21082[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x525A, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21083[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x525B, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21090[SF64_RU_BLOB_HEADER_WORDS + 19] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5262, 0x0013, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21091[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5263, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21092[SF64_RU_BLOB_HEADER_WORDS + 16] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5264, 0x0010, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_21093[SF64_RU_BLOB_HEADER_WORDS + 18] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5265, 0x0012, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22000[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F0, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22001[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F1, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22002[SF64_RU_BLOB_HEADER_WORDS + 10] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F2, 0x000A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22003[SF64_RU_BLOB_HEADER_WORDS + 6] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F3, 0x0006, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22004[SF64_RU_BLOB_HEADER_WORDS + 11] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F4, 0x000B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22005[SF64_RU_BLOB_HEADER_WORDS + 14] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F5, 0x000E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22006[SF64_RU_BLOB_HEADER_WORDS + 7] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F6, 0x0007, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22007[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F7, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22008[SF64_RU_BLOB_HEADER_WORDS + 9] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F8, 0x0009, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22009[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55F9, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22010[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FA, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22011[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FB, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22012[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FC, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22013[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FD, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22014[SF64_RU_BLOB_HEADER_WORDS + 12] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FE, 0x000C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22015[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x55FF, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22016[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5600, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22017[SF64_RU_BLOB_HEADER_WORDS + 8] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5601, 0x0008, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22018[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5602, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22019[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5603, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_22020[SF64_RU_BLOB_HEADER_WORDS + 13] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x5604, 0x000D, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23000[SF64_RU_BLOB_HEADER_WORDS + 33] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59D8, 0x0021, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23001[SF64_RU_BLOB_HEADER_WORDS + 51] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59D9, 0x0033, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23002[SF64_RU_BLOB_HEADER_WORDS + 46] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DA, 0x002E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23003[SF64_RU_BLOB_HEADER_WORDS + 59] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DB, 0x003B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23004[SF64_RU_BLOB_HEADER_WORDS + 44] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DC, 0x002C, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23005[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DD, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23006[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DE, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23007[SF64_RU_BLOB_HEADER_WORDS + 40] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59DF, 0x0028, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23008[SF64_RU_BLOB_HEADER_WORDS + 54] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E0, 0x0036, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23009[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E1, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23010[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E2, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23011[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E3, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23012[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E4, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23013[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E5, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23014[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E6, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23015[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E7, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23016[SF64_RU_BLOB_HEADER_WORDS + 24] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E8, 0x0018, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23017[SF64_RU_BLOB_HEADER_WORDS + 35] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59E9, 0x0023, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23018[SF64_RU_BLOB_HEADER_WORDS + 36] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59EA, 0x0024, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23019[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59EB, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23020[SF64_RU_BLOB_HEADER_WORDS + 37] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59EC, 0x0025, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23021[SF64_RU_BLOB_HEADER_WORDS + 42] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59ED, 0x002A, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23022[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59EE, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23023[SF64_RU_BLOB_HEADER_WORDS + 30] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59EF, 0x001E, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23024[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F0, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23025[SF64_RU_BLOB_HEADER_WORDS + 50] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F1, 0x0032, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23026[SF64_RU_BLOB_HEADER_WORDS + 39] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F2, 0x0027, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23027[SF64_RU_BLOB_HEADER_WORDS + 38] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F3, 0x0026, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23028[SF64_RU_BLOB_HEADER_WORDS + 32] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F4, 0x0020, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23029[SF64_RU_BLOB_HEADER_WORDS + 43] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F5, 0x002B, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23030[SF64_RU_BLOB_HEADER_WORDS + 48] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F6, 0x0030, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23031[SF64_RU_BLOB_HEADER_WORDS + 51] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F7, 0x0033, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_blob_23032[SF64_RU_BLOB_HEADER_WORDS + 41] __attribute__((used, section(".data.sf64_ru_blob"))) = { SF64_RU_BLOB_MAGIC_A, SF64_RU_BLOB_MAGIC_B, 0x0000, 0x59F8, 0x0029, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };


/* Russian Language Mod: patchable radio-only external buffers.
   These are not copied into original gMsg_ID_* slots; patched Message_* functions
   redirect radio display/width only when a buffer is enabled. */
static volatile u16 sf64_ru_radio_blob_1[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0001, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x000A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0014, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_30[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x001E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_40[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0028, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_50[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0032, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_60[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x003C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04B0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04BA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04C4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04CE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04E2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04EC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1270[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x04F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1280[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0500, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1290[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x050A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0514, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x051E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0528, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0532, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x053C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0546, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0550, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1370[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x055A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1380[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0564, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1390[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x056E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1400[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0578, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1410[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0582, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1420[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x058C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1430[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0596, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1440[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x05A0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1450[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x05AA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1460[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x05B4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_1470[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x05BE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23000[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23001[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59D9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23002[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23003[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23004[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23006[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23007[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59DF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23008[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23009[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23014[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23015[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23016[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23017[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59E9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23018[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59EA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23019[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59EB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59EC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23021[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59ED, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23022[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59EE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23023[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59EF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23024[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23025[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23026[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23027[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23028[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23029[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23031[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_23032[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x59F8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x07D5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x07DA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x07E4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x07EE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x07F8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0802, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2055[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0807, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2058[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x080A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2061[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x080D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2062[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x080E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0820, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x082A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2095[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x082F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x083E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2115[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0843, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2118[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0846, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x085C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2165[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0875, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2166[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0876, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2167[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0877, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2180[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0884, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2181[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0885, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2185[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0889, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2188[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x088C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0898, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08A2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08AC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2225[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08B1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08B6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2233[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08B9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08C0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08CA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08D4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2263[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08D7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2265[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08D9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2270[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08DE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2275[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08E3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2280[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08E8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2282[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08EA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2290[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2291[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2292[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2293[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2294[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2295[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2296[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08F8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2298[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08FA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2299[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08FB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x08FC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2305[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0901, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0906, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0910, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2335[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x091F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2336[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0920, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_2337[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0921, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BBD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BC2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3015[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BC7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BCC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3025[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BD1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3026[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BD2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BE0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3041[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BE1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3042[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BE2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0BEA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0C1C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0C26, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0C30, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CE4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CEE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3315[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CF3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CF8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3321[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CF9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3322[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0CFA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D02, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D0C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3345[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D11, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D16, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D20, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3370[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D2A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_3371[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0D2B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36C4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36CE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14045[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36DD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36E2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36EC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x36F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3700, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3714, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x371E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3728, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3732, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x373C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3746, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3750, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14170[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x375A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14180[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3764, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14190[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x376E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3778, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3782, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x378C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3796, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x37DC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x37E6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x37F0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x37FA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3804, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x380E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3818, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_14370[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3822, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9000[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2328, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2332, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9375[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x249F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9380[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24A4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9385[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24A9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9390[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24AE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9395[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24B3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9400[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24B8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9405[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24BD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9411[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24C3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9420[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24CC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9425[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9426[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9427[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9428[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9429[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9430[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9431[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9432[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9433[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24D9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9434[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24DA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9436[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24DC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9437[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24DD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9438[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x24DE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x238C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2396, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23A0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23AA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23B4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23BE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9151[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23BF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9152[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23C0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9153[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23C1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23C8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9170[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23D2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9180[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23DC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9190[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23E6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23F0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23FA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9211[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23FB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9212[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23FC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9213[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x23FD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2404, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x240E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2418, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2422, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x242C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9270[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2436, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9275[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x243B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9280[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2440, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9285[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2445, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9289[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2449, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9290[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x244A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2454, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x245E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2468, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9322[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x246A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9323[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x246B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9324[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x246C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9325[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x246D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2472, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x247C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2486, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2490, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9365[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2495, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9366[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2496, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9367[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2497, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9368[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2498, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_9369[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2499, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18000[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4650, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4655, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18006[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4656, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18007[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4657, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x465A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18015[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x465F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18018[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4662, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4664, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18021[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4665, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18022[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4666, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18025[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4669, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x466E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18031[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x466F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18035[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4673, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4678, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18045[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x467D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4682, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18055[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4687, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x468C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18065[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4691, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18066[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4692, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4696, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18075[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x469B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46A0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18085[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46A5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46AA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18095[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46AF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46B4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18105[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46B9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46C8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46D2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46DC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_18150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x46E6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AA2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AB6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AC0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15045[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AC5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ACA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15051[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ACB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15052[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ACC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15053[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ACD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15054[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ACE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AD4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3AFC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B06, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B10, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B1A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B24, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B60, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B6A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B74, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B7E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B88, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B92, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15251[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B93, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15252[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B94, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15253[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B95, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_15254[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3B96, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5000[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1388, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1392, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x13C4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x13D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x13EC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x13F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x140A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1464, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x146E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14B4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14BE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5311[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14BF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5312[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14C0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5313[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14C1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5314[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14C2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14E6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x14F0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5380[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1504, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5400[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1518, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5410[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1522, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5420[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x152C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5430[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1536, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5460[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1554, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5470[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x155E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5473[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1561, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5474[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1562, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5475[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1563, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5492[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1574, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5493[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1575, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5494[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1576, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5495[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1577, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5496[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1578, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5497[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1579, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5498[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5499[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5500[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5501[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5502[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5503[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x157F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5504[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1580, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5505[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1581, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_5506[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1582, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x271A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2724, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2738, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2742, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x274C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2756, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2760, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x27D8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x27E2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x27EC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x27F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10255[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x280F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x283C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2846, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2850, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10321[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2851, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10322[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2852, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10323[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2853, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_10324[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2854, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x177A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x177B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x177C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x177D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6014[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x177E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1784, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6021[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1785, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6024[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1788, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6025[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1789, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6026[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x178A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6027[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x178B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6028[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x178C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6029[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x178D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6036[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1794, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6038[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1796, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6041[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1799, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6042[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x179A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6045[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x179D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17A2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6051[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17A3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6055[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17A7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6066[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6067[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6068[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6069[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6071[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6072[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6073[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17B9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6074[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6075[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6076[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6077[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6078[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6079[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17BF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17C0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6081[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17C1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6082[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17C2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17CA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17D4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_6101[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x17D5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FAA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FAB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FAC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FAD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FB4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4021[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FB5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4022[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FB6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4023[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FB7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4024[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FB8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FBE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4031[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FBF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FC8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FD2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4075[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FEB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FF0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4082[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FF2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4083[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FF3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4091[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FFB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4092[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FFC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4093[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FFD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4094[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FFE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4095[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x0FFF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4096[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1000, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4097[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1001, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4098[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1002, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4099[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1003, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1004, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4101[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1005, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4102[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1006, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4103[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1007, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x100E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4111[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x100F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4112[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1010, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_4113[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1011, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4272, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x427C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4286, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42CC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42D6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42E0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42EA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17131[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42EB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42F4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x42FE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4308, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17170[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4312, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4394, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x439E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43A8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43B2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43C6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43D0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17370[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43DA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17380[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43E4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17390[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43EE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17400[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x43F8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17410[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4402, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17420[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x440C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17430[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4416, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17440[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4420, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17450[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x442A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17460[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4434, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17470[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x443E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17471[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x443F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17472[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4440, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17473[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4441, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17474[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4442, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17475[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4443, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_17476[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4444, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3E8A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3E94, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3E9E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EA8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16046[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EAE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16047[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EAF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EB2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16055[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EB7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EBC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ED0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16085[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3ED5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EDA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EE4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EEE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EF8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16125[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3EFD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F02, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16135[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F07, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F0C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F16, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F20, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16165[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F25, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16170[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F2A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16175[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F2F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16180[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F34, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16185[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F39, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F48, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F52, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F5C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F66, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F70, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F7A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F84, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16270[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F8E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_16280[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x3F98, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B02, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B0C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B16, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B20, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B2A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B34, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B5C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B66, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B70, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B7A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B8E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2B98, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BC0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BCA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BD4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BDE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BE8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_11241[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2BE9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B5D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7006[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B5E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B63, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B64, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B65, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7014[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B66, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B6C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7043[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B83, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B8A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7051[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B8B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7052[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B8C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7053[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B8D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7054[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B8E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7061[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B95, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7064[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B98, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7065[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B99, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7066[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B9A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1B9E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7083[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BAB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7084[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BAC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7085[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BAD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7086[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BAE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7087[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BAF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7093[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BB5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7094[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BB6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7095[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BB7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7096[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BB8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7097[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BB9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7098[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BBA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7099[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BBB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_7100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1BBC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F4A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F54, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F5E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F68, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8045[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F6D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F72, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F7C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F86, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1F90, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8100[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1FA4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8110[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1FAE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8120[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1FB8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8130[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1FC2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8140[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x1FCC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8205[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x200D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2012, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8215[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2017, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x201C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2026, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2030, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x203A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8255[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x203F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2044, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8265[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2049, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x206C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2076, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_8320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x2080, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4A42, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19325[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B7D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B82, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19335[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B87, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B8C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19350[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B96, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19355[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B9B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19360[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BA0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19370[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BAA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19466[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C0A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19467[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C0B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19468[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C0C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B00, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19205[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B05, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B0A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B14, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B1E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19240[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B28, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4B32, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19450[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19451[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19452[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19453[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19454[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19455[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BFF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19456[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C00, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19457[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C01, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19458[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C02, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19459[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C03, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19460[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C04, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19461[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C05, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19462[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C06, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19463[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C07, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19464[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C08, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19465[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4C09, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19400[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BC8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19410[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BD2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19420[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BDC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19430[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BE6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_19440[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4BF0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5212, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x521C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5226, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x523A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5244, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x524E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21071[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x524F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21072[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5250, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21073[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5251, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5258, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21081[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5259, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21082[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x525A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21083[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x525B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5262, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21091[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5263, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21092[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5264, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_21093[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5265, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20014[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20015[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E2F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20016[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E30, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20017[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E31, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20018[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E32, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20019[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E33, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E34, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20030[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E3E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20040[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E48, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20050[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E52, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20060[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E5C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20070[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E66, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20080[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E70, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20084[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E74, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20085[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E75, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20090[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E7A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20091[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E7B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20092[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4E7C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20150[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EB6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20160[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EC0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20170[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4ECA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20180[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4ED4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20190[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EDE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20200[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EE8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20210[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EF2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20220[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EFC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20221[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EFD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20222[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4EFE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20230[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F06, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20235[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F0B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20236[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F0C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20237[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F0D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20238[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F0E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20239[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F0F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20250[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F1A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20260[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F24, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20261[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F25, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20262[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F26, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20263[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F27, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20264[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F28, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20265[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F29, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20266[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20267[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20268[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20269[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20270[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20271[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F2F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20272[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F30, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20273[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F31, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20274[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F32, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20275[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F33, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20276[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F34, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20277[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F35, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20278[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F36, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20279[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F37, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20280[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F38, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20281[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F39, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20282[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20283[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20284[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20285[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20286[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20287[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F3F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20288[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F40, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20289[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F41, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20290[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F42, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20291[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F43, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20292[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F44, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20294[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F46, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20296[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F48, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20297[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F49, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20298[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20299[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20300[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20301[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20302[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20303[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F4F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20304[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F50, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20305[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F51, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20306[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F52, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20307[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F53, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20308[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F54, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20309[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F55, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20310[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F56, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20311[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F57, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20312[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F58, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20313[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F59, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20314[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20315[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20316[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20317[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20318[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5E, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20319[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F5F, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20320[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F60, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20321[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F61, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20326[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F66, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20327[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F67, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20328[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F68, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20329[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F69, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20330[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F6A, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20331[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F6B, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20332[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F6C, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20333[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F6D, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20337[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F71, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20338[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F72, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20339[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F73, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20340[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F74, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20343[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F77, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20344[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F78, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_20345[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x4F79, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22000[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F0, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22001[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F1, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22002[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F2, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22003[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F3, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22004[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F4, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22005[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F5, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22006[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F6, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22007[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F7, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22008[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F8, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22009[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55F9, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22010[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FA, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22011[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FB, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22012[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FC, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22013[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FD, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22014[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FE, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22015[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x55FF, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22016[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5600, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22017[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5601, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22018[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5602, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22019[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5603, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };
static volatile u16 sf64_ru_radio_blob_22020[SF64_RU_BLOB_HEADER_WORDS + SF64_RU_RADIO_MAX_WORDS] __attribute__((used, section(".data.sf64_ru_radio_blob"))) = { SF64_RU_RADIO_MAGIC_A, SF64_RU_RADIO_MAGIC_B, 0x0000, 0x5604, SF64_RU_RADIO_MAX_WORDS, 0x0000, 0x0000, SF64_RU_BLOB_TAIL };

static const Sf64RuRuntimeSlotEntry sf64_ru_runtime_table[] = {
    { gMsg_ID_1, sf64_ru_blob_1, 914 },
    { gMsg_ID_10, sf64_ru_blob_10, 32 },
    { gMsg_ID_20, sf64_ru_blob_20, 29 },
    { gMsg_ID_30, sf64_ru_blob_30, 33 },
    { gMsg_ID_40, sf64_ru_blob_40, 40 },
    { gMsg_ID_50, sf64_ru_blob_50, 51 },
    { gMsg_ID_60, sf64_ru_blob_60, 45 },
    { gMsg_ID_1200, sf64_ru_blob_1200, 73 },
    { gMsg_ID_1210, sf64_ru_blob_1210, 54 },
    { gMsg_ID_1220, sf64_ru_blob_1220, 75 },
    { gMsg_ID_1230, sf64_ru_blob_1230, 28 },
    { gMsg_ID_1240, sf64_ru_blob_1240, 47 },
    { gMsg_ID_1250, sf64_ru_blob_1250, 31 },
    { gMsg_ID_1260, sf64_ru_blob_1260, 65 },
    { gMsg_ID_1270, sf64_ru_blob_1270, 20 },
    { gMsg_ID_1280, sf64_ru_blob_1280, 39 },
    { gMsg_ID_1290, sf64_ru_blob_1290, 17 },
    { gMsg_ID_1300, sf64_ru_blob_1300, 78 },
    { gMsg_ID_1310, sf64_ru_blob_1310, 12 },
    { gMsg_ID_1320, sf64_ru_blob_1320, 68 },
    { gMsg_ID_1330, sf64_ru_blob_1330, 19 },
    { gMsg_ID_1340, sf64_ru_blob_1340, 64 },
    { gMsg_ID_1350, sf64_ru_blob_1350, 36 },
    { gMsg_ID_1360, sf64_ru_blob_1360, 54 },
    { gMsg_ID_1370, sf64_ru_blob_1370, 23 },
    { gMsg_ID_1380, sf64_ru_blob_1380, 56 },
    { gMsg_ID_1390, sf64_ru_blob_1390, 35 },
    { gMsg_ID_1400, sf64_ru_blob_1400, 31 },
    { gMsg_ID_1410, sf64_ru_blob_1410, 23 },
    { gMsg_ID_1420, sf64_ru_blob_1420, 48 },
    { gMsg_ID_1430, sf64_ru_blob_1430, 19 },
    { gMsg_ID_1440, sf64_ru_blob_1440, 45 },
    { gMsg_ID_1450, sf64_ru_blob_1450, 16 },
    { gMsg_ID_1460, sf64_ru_blob_1460, 73 },
    { gMsg_ID_1470, sf64_ru_blob_1470, 18 },
    { gMsg_ID_2005, sf64_ru_blob_2005, 18 },
    { gMsg_ID_2010, sf64_ru_blob_2010, 33 },
    { gMsg_ID_2020, sf64_ru_blob_2020, 25 },
    { gMsg_ID_2030, sf64_ru_blob_2030, 34 },
    { gMsg_ID_2040, sf64_ru_blob_2040, 24 },
    { gMsg_ID_2050, sf64_ru_blob_2050, 43 },
    { gMsg_ID_2055, sf64_ru_blob_2055, 36 },
    { gMsg_ID_2058, sf64_ru_blob_2058, 22 },
    { gMsg_ID_2061, sf64_ru_blob_2061, 42 },
    { gMsg_ID_2062, sf64_ru_blob_2062, 17 },
    { gMsg_ID_2080, sf64_ru_blob_2080, 35 },
    { gMsg_ID_2090, sf64_ru_blob_2090, 10 },
    { gMsg_ID_2095, sf64_ru_blob_2095, 17 },
    { gMsg_ID_2110, sf64_ru_blob_2110, 36 },
    { gMsg_ID_2115, sf64_ru_blob_2115, 37 },
    { gMsg_ID_2118, sf64_ru_blob_2118, 41 },
    { gMsg_ID_2140, sf64_ru_blob_2140, 39 },
    { gMsg_ID_2165, sf64_ru_blob_2165, 45 },
    { gMsg_ID_2166, sf64_ru_blob_2166, 36 },
    { gMsg_ID_2167, sf64_ru_blob_2167, 41 },
    { gMsg_ID_2180, sf64_ru_blob_2180, 30 },
    { gMsg_ID_2181, sf64_ru_blob_2181, 18 },
    { gMsg_ID_2185, sf64_ru_blob_2185, 26 },
    { gMsg_ID_2188, sf64_ru_blob_2188, 38 },
    { gMsg_ID_2200, sf64_ru_blob_2200, 26 },
    { gMsg_ID_2210, sf64_ru_blob_2210, 12 },
    { gMsg_ID_2220, sf64_ru_blob_2220, 30 },
    { gMsg_ID_2225, sf64_ru_blob_2225, 25 },
    { gMsg_ID_2230, sf64_ru_blob_2230, 20 },
    { gMsg_ID_2233, sf64_ru_blob_2233, 39 },
    { gMsg_ID_2240, sf64_ru_blob_2240, 27 },
    { gMsg_ID_2250, sf64_ru_blob_2250, 26 },
    { gMsg_ID_2260, sf64_ru_blob_2260, 36 },
    { gMsg_ID_2263, sf64_ru_blob_2263, 36 },
    { gMsg_ID_2265, sf64_ru_blob_2265, 33 },
    { gMsg_ID_2270, sf64_ru_blob_2270, 8 },
    { gMsg_ID_2275, sf64_ru_blob_2275, 39 },
    { gMsg_ID_2280, sf64_ru_blob_2280, 33 },
    { gMsg_ID_2282, sf64_ru_blob_2282, 38 },
    { gMsg_ID_2290, sf64_ru_blob_2290, 29 },
    { gMsg_ID_2291, sf64_ru_blob_2291, 30 },
    { gMsg_ID_2292, sf64_ru_blob_2292, 29 },
    { gMsg_ID_2293, sf64_ru_blob_2293, 7 },
    { gMsg_ID_2294, sf64_ru_blob_2294, 20 },
    { gMsg_ID_2295, sf64_ru_blob_2295, 18 },
    { gMsg_ID_2296, sf64_ru_blob_2296, 32 },
    { gMsg_ID_2298, sf64_ru_blob_2298, 48 },
    { gMsg_ID_2299, sf64_ru_blob_2299, 25 },
    { gMsg_ID_2300, sf64_ru_blob_2300, 43 },
    { gMsg_ID_2305, sf64_ru_blob_2305, 15 },
    { gMsg_ID_2310, sf64_ru_blob_2310, 41 },
    { gMsg_ID_2320, sf64_ru_blob_2320, 37 },
    { gMsg_ID_2335, sf64_ru_blob_2335, 43 },
    { gMsg_ID_2336, sf64_ru_blob_2336, 38 },
    { gMsg_ID_2337, sf64_ru_blob_2337, 33 },
    { gMsg_ID_3005, sf64_ru_blob_3005, 35 },
    { gMsg_ID_3010, sf64_ru_blob_3010, 29 },
    { gMsg_ID_3015, sf64_ru_blob_3015, 36 },
    { gMsg_ID_3020, sf64_ru_blob_3020, 24 },
    { gMsg_ID_3025, sf64_ru_blob_3025, 25 },
    { gMsg_ID_3026, sf64_ru_blob_3026, 27 },
    { gMsg_ID_3040, sf64_ru_blob_3040, 22 },
    { gMsg_ID_3041, sf64_ru_blob_3041, 29 },
    { gMsg_ID_3042, sf64_ru_blob_3042, 26 },
    { gMsg_ID_3050, sf64_ru_blob_3050, 34 },
    { gMsg_ID_3100, sf64_ru_blob_3100, 41 },
    { gMsg_ID_3110, sf64_ru_blob_3110, 27 },
    { gMsg_ID_3120, sf64_ru_blob_3120, 13 },
    { gMsg_ID_3300, sf64_ru_blob_3300, 40 },
    { gMsg_ID_3310, sf64_ru_blob_3310, 30 },
    { gMsg_ID_3315, sf64_ru_blob_3315, 39 },
    { gMsg_ID_3320, sf64_ru_blob_3320, 27 },
    { gMsg_ID_3321, sf64_ru_blob_3321, 18 },
    { gMsg_ID_3322, sf64_ru_blob_3322, 24 },
    { gMsg_ID_3330, sf64_ru_blob_3330, 18 },
    { gMsg_ID_3340, sf64_ru_blob_3340, 39 },
    { gMsg_ID_3345, sf64_ru_blob_3345, 45 },
    { gMsg_ID_3350, sf64_ru_blob_3350, 28 },
    { gMsg_ID_3360, sf64_ru_blob_3360, 36 },
    { gMsg_ID_3370, sf64_ru_blob_3370, 40 },
    { gMsg_ID_3371, sf64_ru_blob_3371, 38 },
    { gMsg_ID_4010, sf64_ru_blob_4010, 34 },
    { gMsg_ID_4011, sf64_ru_blob_4011, 37 },
    { gMsg_ID_4012, sf64_ru_blob_4012, 37 },
    { gMsg_ID_4013, sf64_ru_blob_4013, 39 },
    { gMsg_ID_4020, sf64_ru_blob_4020, 37 },
    { gMsg_ID_4021, sf64_ru_blob_4021, 37 },
    { gMsg_ID_4022, sf64_ru_blob_4022, 39 },
    { gMsg_ID_4023, sf64_ru_blob_4023, 18 },
    { gMsg_ID_4024, sf64_ru_blob_4024, 28 },
    { gMsg_ID_4030, sf64_ru_blob_4030, 26 },
    { gMsg_ID_4031, sf64_ru_blob_4031, 37 },
    { gMsg_ID_4040, sf64_ru_blob_4040, 14 },
    { gMsg_ID_4050, sf64_ru_blob_4050, 23 },
    { gMsg_ID_4075, sf64_ru_blob_4075, 12 },
    { gMsg_ID_4080, sf64_ru_blob_4080, 45 },
    { gMsg_ID_4082, sf64_ru_blob_4082, 23 },
    { gMsg_ID_4083, sf64_ru_blob_4083, 22 },
    { gMsg_ID_4091, sf64_ru_blob_4091, 20 },
    { gMsg_ID_4092, sf64_ru_blob_4092, 41 },
    { gMsg_ID_4093, sf64_ru_blob_4093, 16 },
    { gMsg_ID_4094, sf64_ru_blob_4094, 43 },
    { gMsg_ID_4095, sf64_ru_blob_4095, 27 },
    { gMsg_ID_4096, sf64_ru_blob_4096, 35 },
    { gMsg_ID_4097, sf64_ru_blob_4097, 40 },
    { gMsg_ID_4098, sf64_ru_blob_4098, 29 },
    { gMsg_ID_4099, sf64_ru_blob_4099, 31 },
    { gMsg_ID_4100, sf64_ru_blob_4100, 27 },
    { gMsg_ID_4101, sf64_ru_blob_4101, 39 },
    { gMsg_ID_4102, sf64_ru_blob_4102, 37 },
    { gMsg_ID_4103, sf64_ru_blob_4103, 41 },
    { gMsg_ID_4110, sf64_ru_blob_4110, 38 },
    { gMsg_ID_4111, sf64_ru_blob_4111, 10 },
    { gMsg_ID_4112, sf64_ru_blob_4112, 29 },
    { gMsg_ID_4113, sf64_ru_blob_4113, 25 },
    { gMsg_ID_5000, sf64_ru_blob_5000, 37 },
    { gMsg_ID_5010, sf64_ru_blob_5010, 28 },
    { gMsg_ID_5060, sf64_ru_blob_5060, 43 },
    { gMsg_ID_5080, sf64_ru_blob_5080, 25 },
    { gMsg_ID_5100, sf64_ru_blob_5100, 35 },
    { gMsg_ID_5110, sf64_ru_blob_5110, 40 },
    { gMsg_ID_5130, sf64_ru_blob_5130, 27 },
    { gMsg_ID_5220, sf64_ru_blob_5220, 44 },
    { gMsg_ID_5230, sf64_ru_blob_5230, 15 },
    { gMsg_ID_5300, sf64_ru_blob_5300, 43 },
    { gMsg_ID_5310, sf64_ru_blob_5310, 39 },
    { gMsg_ID_5311, sf64_ru_blob_5311, 39 },
    { gMsg_ID_5312, sf64_ru_blob_5312, 19 },
    { gMsg_ID_5313, sf64_ru_blob_5313, 32 },
    { gMsg_ID_5314, sf64_ru_blob_5314, 40 },
    { gMsg_ID_5350, sf64_ru_blob_5350, 30 },
    { gMsg_ID_5360, sf64_ru_blob_5360, 26 },
    { gMsg_ID_5380, sf64_ru_blob_5380, 28 },
    { gMsg_ID_5400, sf64_ru_blob_5400, 33 },
    { gMsg_ID_5410, sf64_ru_blob_5410, 33 },
    { gMsg_ID_5420, sf64_ru_blob_5420, 22 },
    { gMsg_ID_5430, sf64_ru_blob_5430, 31 },
    { gMsg_ID_5460, sf64_ru_blob_5460, 37 },
    { gMsg_ID_5470, sf64_ru_blob_5470, 39 },
    { gMsg_ID_5473, sf64_ru_blob_5473, 33 },
    { gMsg_ID_5474, sf64_ru_blob_5474, 28 },
    { gMsg_ID_5475, sf64_ru_blob_5475, 22 },
    { gMsg_ID_5492, sf64_ru_blob_5492, 20 },
    { gMsg_ID_5493, sf64_ru_blob_5493, 32 },
    { gMsg_ID_5494, sf64_ru_blob_5494, 20 },
    { gMsg_ID_5495, sf64_ru_blob_5495, 26 },
    { gMsg_ID_5496, sf64_ru_blob_5496, 20 },
    { gMsg_ID_5497, sf64_ru_blob_5497, 21 },
    { gMsg_ID_5498, sf64_ru_blob_5498, 22 },
    { gMsg_ID_5499, sf64_ru_blob_5499, 22 },
    { gMsg_ID_5500, sf64_ru_blob_5500, 43 },
    { gMsg_ID_5501, sf64_ru_blob_5501, 30 },
    { gMsg_ID_5502, sf64_ru_blob_5502, 33 },
    { gMsg_ID_5503, sf64_ru_blob_5503, 39 },
    { gMsg_ID_5504, sf64_ru_blob_5504, 24 },
    { gMsg_ID_5505, sf64_ru_blob_5505, 29 },
    { gMsg_ID_5506, sf64_ru_blob_5506, 25 },
    { gMsg_ID_6010, sf64_ru_blob_6010, 40 },
    { gMsg_ID_6011, sf64_ru_blob_6011, 18 },
    { gMsg_ID_6012, sf64_ru_blob_6012, 15 },
    { gMsg_ID_6013, sf64_ru_blob_6013, 34 },
    { gMsg_ID_6014, sf64_ru_blob_6014, 21 },
    { gMsg_ID_6020, sf64_ru_blob_6020, 34 },
    { gMsg_ID_6021, sf64_ru_blob_6021, 27 },
    { gMsg_ID_6024, sf64_ru_blob_6024, 31 },
    { gMsg_ID_6025, sf64_ru_blob_6025, 35 },
    { gMsg_ID_6026, sf64_ru_blob_6026, 34 },
    { gMsg_ID_6027, sf64_ru_blob_6027, 35 },
    { gMsg_ID_6028, sf64_ru_blob_6028, 13 },
    { gMsg_ID_6029, sf64_ru_blob_6029, 47 },
    { gMsg_ID_6036, sf64_ru_blob_6036, 16 },
    { gMsg_ID_6038, sf64_ru_blob_6038, 28 },
    { gMsg_ID_6041, sf64_ru_blob_6041, 32 },
    { gMsg_ID_6042, sf64_ru_blob_6042, 42 },
    { gMsg_ID_6045, sf64_ru_blob_6045, 42 },
    { gMsg_ID_6050, sf64_ru_blob_6050, 26 },
    { gMsg_ID_6051, sf64_ru_blob_6051, 9 },
    { gMsg_ID_6055, sf64_ru_blob_6055, 19 },
    { gMsg_ID_6066, sf64_ru_blob_6066, 34 },
    { gMsg_ID_6067, sf64_ru_blob_6067, 35 },
    { gMsg_ID_6068, sf64_ru_blob_6068, 19 },
    { gMsg_ID_6069, sf64_ru_blob_6069, 31 },
    { gMsg_ID_6071, sf64_ru_blob_6071, 30 },
    { gMsg_ID_6072, sf64_ru_blob_6072, 30 },
    { gMsg_ID_6073, sf64_ru_blob_6073, 22 },
    { gMsg_ID_6074, sf64_ru_blob_6074, 33 },
    { gMsg_ID_6075, sf64_ru_blob_6075, 36 },
    { gMsg_ID_6076, sf64_ru_blob_6076, 31 },
    { gMsg_ID_6077, sf64_ru_blob_6077, 40 },
    { gMsg_ID_6078, sf64_ru_blob_6078, 14 },
    { gMsg_ID_6079, sf64_ru_blob_6079, 15 },
    { gMsg_ID_6080, sf64_ru_blob_6080, 40 },
    { gMsg_ID_6081, sf64_ru_blob_6081, 33 },
    { gMsg_ID_6082, sf64_ru_blob_6082, 28 },
    { gMsg_ID_6090, sf64_ru_blob_6090, 13 },
    { gMsg_ID_6100, sf64_ru_blob_6100, 40 },
    { gMsg_ID_6101, sf64_ru_blob_6101, 25 },
    { gMsg_ID_7005, sf64_ru_blob_7005, 28 },
    { gMsg_ID_7006, sf64_ru_blob_7006, 36 },
    { gMsg_ID_7011, sf64_ru_blob_7011, 29 },
    { gMsg_ID_7012, sf64_ru_blob_7012, 35 },
    { gMsg_ID_7013, sf64_ru_blob_7013, 24 },
    { gMsg_ID_7014, sf64_ru_blob_7014, 21 },
    { gMsg_ID_7020, sf64_ru_blob_7020, 35 },
    { gMsg_ID_7043, sf64_ru_blob_7043, 40 },
    { gMsg_ID_7050, sf64_ru_blob_7050, 26 },
    { gMsg_ID_7051, sf64_ru_blob_7051, 38 },
    { gMsg_ID_7052, sf64_ru_blob_7052, 18 },
    { gMsg_ID_7053, sf64_ru_blob_7053, 16 },
    { gMsg_ID_7054, sf64_ru_blob_7054, 28 },
    { gMsg_ID_7061, sf64_ru_blob_7061, 37 },
    { gMsg_ID_7064, sf64_ru_blob_7064, 35 },
    { gMsg_ID_7065, sf64_ru_blob_7065, 39 },
    { gMsg_ID_7066, sf64_ru_blob_7066, 39 },
    { gMsg_ID_7070, sf64_ru_blob_7070, 23 },
    { gMsg_ID_7083, sf64_ru_blob_7083, 41 },
    { gMsg_ID_7084, sf64_ru_blob_7084, 34 },
    { gMsg_ID_7085, sf64_ru_blob_7085, 23 },
    { gMsg_ID_7086, sf64_ru_blob_7086, 14 },
    { gMsg_ID_7087, sf64_ru_blob_7087, 38 },
    { gMsg_ID_7093, sf64_ru_blob_7093, 43 },
    { gMsg_ID_7094, sf64_ru_blob_7094, 38 },
    { gMsg_ID_7095, sf64_ru_blob_7095, 42 },
    { gMsg_ID_7096, sf64_ru_blob_7096, 35 },
    { gMsg_ID_7097, sf64_ru_blob_7097, 24 },
    { gMsg_ID_7098, sf64_ru_blob_7098, 23 },
    { gMsg_ID_7099, sf64_ru_blob_7099, 35 },
    { gMsg_ID_7100, sf64_ru_blob_7100, 13 },
    { gMsg_ID_8010, sf64_ru_blob_8010, 35 },
    { gMsg_ID_8020, sf64_ru_blob_8020, 39 },
    { gMsg_ID_8030, sf64_ru_blob_8030, 42 },
    { gMsg_ID_8040, sf64_ru_blob_8040, 27 },
    { gMsg_ID_8045, sf64_ru_blob_8045, 31 },
    { gMsg_ID_8050, sf64_ru_blob_8050, 24 },
    { gMsg_ID_8060, sf64_ru_blob_8060, 22 },
    { gMsg_ID_8070, sf64_ru_blob_8070, 22 },
    { gMsg_ID_8080, sf64_ru_blob_8080, 28 },
    { gMsg_ID_8100, sf64_ru_blob_8100, 30 },
    { gMsg_ID_8110, sf64_ru_blob_8110, 35 },
    { gMsg_ID_8120, sf64_ru_blob_8120, 38 },
    { gMsg_ID_8130, sf64_ru_blob_8130, 14 },
    { gMsg_ID_8140, sf64_ru_blob_8140, 30 },
    { gMsg_ID_8205, sf64_ru_blob_8205, 32 },
    { gMsg_ID_8210, sf64_ru_blob_8210, 43 },
    { gMsg_ID_8215, sf64_ru_blob_8215, 30 },
    { gMsg_ID_8220, sf64_ru_blob_8220, 8 },
    { gMsg_ID_8230, sf64_ru_blob_8230, 8 },
    { gMsg_ID_8240, sf64_ru_blob_8240, 8 },
    { gMsg_ID_8250, sf64_ru_blob_8250, 39 },
    { gMsg_ID_8255, sf64_ru_blob_8255, 38 },
    { gMsg_ID_8260, sf64_ru_blob_8260, 36 },
    { gMsg_ID_8265, sf64_ru_blob_8265, 38 },
    { gMsg_ID_8300, sf64_ru_blob_8300, 21 },
    { gMsg_ID_8310, sf64_ru_blob_8310, 22 },
    { gMsg_ID_8320, sf64_ru_blob_8320, 40 },
    { gMsg_ID_9000, sf64_ru_blob_9000, 30 },
    { gMsg_ID_9010, sf64_ru_blob_9010, 40 },
    { gMsg_ID_9100, sf64_ru_blob_9100, 31 },
    { gMsg_ID_9110, sf64_ru_blob_9110, 28 },
    { gMsg_ID_9120, sf64_ru_blob_9120, 40 },
    { gMsg_ID_9130, sf64_ru_blob_9130, 27 },
    { gMsg_ID_9140, sf64_ru_blob_9140, 42 },
    { gMsg_ID_9150, sf64_ru_blob_9150, 34 },
    { gMsg_ID_9151, sf64_ru_blob_9151, 17 },
    { gMsg_ID_9152, sf64_ru_blob_9152, 18 },
    { gMsg_ID_9153, sf64_ru_blob_9153, 23 },
    { gMsg_ID_9160, sf64_ru_blob_9160, 43 },
    { gMsg_ID_9170, sf64_ru_blob_9170, 27 },
    { gMsg_ID_9180, sf64_ru_blob_9180, 40 },
    { gMsg_ID_9190, sf64_ru_blob_9190, 30 },
    { gMsg_ID_9200, sf64_ru_blob_9200, 37 },
    { gMsg_ID_9210, sf64_ru_blob_9210, 36 },
    { gMsg_ID_9211, sf64_ru_blob_9211, 25 },
    { gMsg_ID_9212, sf64_ru_blob_9212, 20 },
    { gMsg_ID_9213, sf64_ru_blob_9213, 15 },
    { gMsg_ID_9220, sf64_ru_blob_9220, 21 },
    { gMsg_ID_9230, sf64_ru_blob_9230, 13 },
    { gMsg_ID_9240, sf64_ru_blob_9240, 20 },
    { gMsg_ID_9250, sf64_ru_blob_9250, 36 },
    { gMsg_ID_9260, sf64_ru_blob_9260, 43 },
    { gMsg_ID_9270, sf64_ru_blob_9270, 46 },
    { gMsg_ID_9275, sf64_ru_blob_9275, 27 },
    { gMsg_ID_9280, sf64_ru_blob_9280, 30 },
    { gMsg_ID_9285, sf64_ru_blob_9285, 39 },
    { gMsg_ID_9289, sf64_ru_blob_9289, 40 },
    { gMsg_ID_9290, sf64_ru_blob_9290, 43 },
    { gMsg_ID_9300, sf64_ru_blob_9300, 36 },
    { gMsg_ID_9310, sf64_ru_blob_9310, 33 },
    { gMsg_ID_9320, sf64_ru_blob_9320, 32 },
    { gMsg_ID_9322, sf64_ru_blob_9322, 23 },
    { gMsg_ID_9323, sf64_ru_blob_9323, 39 },
    { gMsg_ID_9324, sf64_ru_blob_9324, 35 },
    { gMsg_ID_9325, sf64_ru_blob_9325, 30 },
    { gMsg_ID_9330, sf64_ru_blob_9330, 41 },
    { gMsg_ID_9340, sf64_ru_blob_9340, 28 },
    { gMsg_ID_9350, sf64_ru_blob_9350, 33 },
    { gMsg_ID_9360, sf64_ru_blob_9360, 26 },
    { gMsg_ID_9365, sf64_ru_blob_9365, 20 },
    { gMsg_ID_9366, sf64_ru_blob_9366, 27 },
    { gMsg_ID_9367, sf64_ru_blob_9367, 37 },
    { gMsg_ID_9368, sf64_ru_blob_9368, 17 },
    { gMsg_ID_9369, sf64_ru_blob_9369, 18 },
    { gMsg_ID_9375, sf64_ru_blob_9375, 39 },
    { gMsg_ID_9380, sf64_ru_blob_9380, 36 },
    { gMsg_ID_9385, sf64_ru_blob_9385, 39 },
    { gMsg_ID_9390, sf64_ru_blob_9390, 35 },
    { gMsg_ID_9395, sf64_ru_blob_9395, 27 },
    { gMsg_ID_9400, sf64_ru_blob_9400, 27 },
    { gMsg_ID_9405, sf64_ru_blob_9405, 30 },
    { gMsg_ID_9411, sf64_ru_blob_9411, 34 },
    { gMsg_ID_9420, sf64_ru_blob_9420, 42 },
    { gMsg_ID_9425, sf64_ru_blob_9425, 22 },
    { gMsg_ID_9426, sf64_ru_blob_9426, 38 },
    { gMsg_ID_9427, sf64_ru_blob_9427, 23 },
    { gMsg_ID_9428, sf64_ru_blob_9428, 19 },
    { gMsg_ID_9429, sf64_ru_blob_9429, 21 },
    { gMsg_ID_9430, sf64_ru_blob_9430, 30 },
    { gMsg_ID_9431, sf64_ru_blob_9431, 37 },
    { gMsg_ID_9432, sf64_ru_blob_9432, 30 },
    { gMsg_ID_9433, sf64_ru_blob_9433, 40 },
    { gMsg_ID_9434, sf64_ru_blob_9434, 26 },
    { gMsg_ID_9436, sf64_ru_blob_9436, 22 },
    { gMsg_ID_9437, sf64_ru_blob_9437, 22 },
    { gMsg_ID_9438, sf64_ru_blob_9438, 22 },
    { gMsg_ID_10010, sf64_ru_blob_10010, 37 },
    { gMsg_ID_10020, sf64_ru_blob_10020, 32 },
    { gMsg_ID_10040, sf64_ru_blob_10040, 20 },
    { gMsg_ID_10050, sf64_ru_blob_10050, 19 },
    { gMsg_ID_10060, sf64_ru_blob_10060, 54 },
    { gMsg_ID_10070, sf64_ru_blob_10070, 36 },
    { gMsg_ID_10080, sf64_ru_blob_10080, 28 },
    { gMsg_ID_10200, sf64_ru_blob_10200, 44 },
    { gMsg_ID_10210, sf64_ru_blob_10210, 9 },
    { gMsg_ID_10220, sf64_ru_blob_10220, 37 },
    { gMsg_ID_10230, sf64_ru_blob_10230, 25 },
    { gMsg_ID_10255, sf64_ru_blob_10255, 32 },
    { gMsg_ID_10300, sf64_ru_blob_10300, 35 },
    { gMsg_ID_10310, sf64_ru_blob_10310, 29 },
    { gMsg_ID_10320, sf64_ru_blob_10320, 38 },
    { gMsg_ID_10321, sf64_ru_blob_10321, 31 },
    { gMsg_ID_10322, sf64_ru_blob_10322, 38 },
    { gMsg_ID_10323, sf64_ru_blob_10323, 42 },
    { gMsg_ID_10324, sf64_ru_blob_10324, 47 },
    { gMsg_ID_11010, sf64_ru_blob_11010, 33 },
    { gMsg_ID_11020, sf64_ru_blob_11020, 33 },
    { gMsg_ID_11030, sf64_ru_blob_11030, 24 },
    { gMsg_ID_11040, sf64_ru_blob_11040, 24 },
    { gMsg_ID_11050, sf64_ru_blob_11050, 42 },
    { gMsg_ID_11060, sf64_ru_blob_11060, 32 },
    { gMsg_ID_11100, sf64_ru_blob_11100, 31 },
    { gMsg_ID_11110, sf64_ru_blob_11110, 41 },
    { gMsg_ID_11120, sf64_ru_blob_11120, 43 },
    { gMsg_ID_11130, sf64_ru_blob_11130, 34 },
    { gMsg_ID_11150, sf64_ru_blob_11150, 30 },
    { gMsg_ID_11160, sf64_ru_blob_11160, 32 },
    { gMsg_ID_11200, sf64_ru_blob_11200, 23 },
    { gMsg_ID_11210, sf64_ru_blob_11210, 33 },
    { gMsg_ID_11220, sf64_ru_blob_11220, 37 },
    { gMsg_ID_11230, sf64_ru_blob_11230, 31 },
    { gMsg_ID_11240, sf64_ru_blob_11240, 22 },
    { gMsg_ID_11241, sf64_ru_blob_11241, 29 },
    { gMsg_ID_14020, sf64_ru_blob_14020, 30 },
    { gMsg_ID_14030, sf64_ru_blob_14030, 37 },
    { gMsg_ID_14040, sf64_ru_blob_14040, 32 },
    { gMsg_ID_14045, sf64_ru_blob_14045, 44 },
    { gMsg_ID_14050, sf64_ru_blob_14050, 44 },
    { gMsg_ID_14060, sf64_ru_blob_14060, 42 },
    { gMsg_ID_14070, sf64_ru_blob_14070, 31 },
    { gMsg_ID_14080, sf64_ru_blob_14080, 31 },
    { gMsg_ID_14100, sf64_ru_blob_14100, 34 },
    { gMsg_ID_14110, sf64_ru_blob_14110, 37 },
    { gMsg_ID_14120, sf64_ru_blob_14120, 38 },
    { gMsg_ID_14130, sf64_ru_blob_14130, 29 },
    { gMsg_ID_14140, sf64_ru_blob_14140, 46 },
    { gMsg_ID_14150, sf64_ru_blob_14150, 30 },
    { gMsg_ID_14160, sf64_ru_blob_14160, 24 },
    { gMsg_ID_14170, sf64_ru_blob_14170, 32 },
    { gMsg_ID_14180, sf64_ru_blob_14180, 27 },
    { gMsg_ID_14190, sf64_ru_blob_14190, 41 },
    { gMsg_ID_14200, sf64_ru_blob_14200, 28 },
    { gMsg_ID_14210, sf64_ru_blob_14210, 34 },
    { gMsg_ID_14220, sf64_ru_blob_14220, 24 },
    { gMsg_ID_14230, sf64_ru_blob_14230, 35 },
    { gMsg_ID_14300, sf64_ru_blob_14300, 25 },
    { gMsg_ID_14310, sf64_ru_blob_14310, 35 },
    { gMsg_ID_14320, sf64_ru_blob_14320, 28 },
    { gMsg_ID_14330, sf64_ru_blob_14330, 21 },
    { gMsg_ID_14340, sf64_ru_blob_14340, 40 },
    { gMsg_ID_14350, sf64_ru_blob_14350, 23 },
    { gMsg_ID_14360, sf64_ru_blob_14360, 17 },
    { gMsg_ID_14370, sf64_ru_blob_14370, 38 },
    { gMsg_ID_15010, sf64_ru_blob_15010, 39 },
    { gMsg_ID_15030, sf64_ru_blob_15030, 37 },
    { gMsg_ID_15040, sf64_ru_blob_15040, 41 },
    { gMsg_ID_15045, sf64_ru_blob_15045, 16 },
    { gMsg_ID_15050, sf64_ru_blob_15050, 30 },
    { gMsg_ID_15051, sf64_ru_blob_15051, 40 },
    { gMsg_ID_15052, sf64_ru_blob_15052, 40 },
    { gMsg_ID_15053, sf64_ru_blob_15053, 47 },
    { gMsg_ID_15054, sf64_ru_blob_15054, 27 },
    { gMsg_ID_15060, sf64_ru_blob_15060, 17 },
    { gMsg_ID_15100, sf64_ru_blob_15100, 29 },
    { gMsg_ID_15110, sf64_ru_blob_15110, 27 },
    { gMsg_ID_15120, sf64_ru_blob_15120, 25 },
    { gMsg_ID_15130, sf64_ru_blob_15130, 18 },
    { gMsg_ID_15140, sf64_ru_blob_15140, 19 },
    { gMsg_ID_15200, sf64_ru_blob_15200, 27 },
    { gMsg_ID_15210, sf64_ru_blob_15210, 42 },
    { gMsg_ID_15220, sf64_ru_blob_15220, 23 },
    { gMsg_ID_15230, sf64_ru_blob_15230, 30 },
    { gMsg_ID_15240, sf64_ru_blob_15240, 27 },
    { gMsg_ID_15250, sf64_ru_blob_15250, 36 },
    { gMsg_ID_15251, sf64_ru_blob_15251, 37 },
    { gMsg_ID_15252, sf64_ru_blob_15252, 23 },
    { gMsg_ID_15253, sf64_ru_blob_15253, 42 },
    { gMsg_ID_15254, sf64_ru_blob_15254, 24 },
    { gMsg_ID_16010, sf64_ru_blob_16010, 35 },
    { gMsg_ID_16020, sf64_ru_blob_16020, 30 },
    { gMsg_ID_16030, sf64_ru_blob_16030, 16 },
    { gMsg_ID_16040, sf64_ru_blob_16040, 35 },
    { gMsg_ID_16046, sf64_ru_blob_16046, 29 },
    { gMsg_ID_16047, sf64_ru_blob_16047, 36 },
    { gMsg_ID_16050, sf64_ru_blob_16050, 37 },
    { gMsg_ID_16055, sf64_ru_blob_16055, 40 },
    { gMsg_ID_16060, sf64_ru_blob_16060, 17 },
    { gMsg_ID_16080, sf64_ru_blob_16080, 36 },
    { gMsg_ID_16085, sf64_ru_blob_16085, 36 },
    { gMsg_ID_16090, sf64_ru_blob_16090, 32 },
    { gMsg_ID_16100, sf64_ru_blob_16100, 32 },
    { gMsg_ID_16110, sf64_ru_blob_16110, 36 },
    { gMsg_ID_16120, sf64_ru_blob_16120, 33 },
    { gMsg_ID_16125, sf64_ru_blob_16125, 43 },
    { gMsg_ID_16130, sf64_ru_blob_16130, 23 },
    { gMsg_ID_16135, sf64_ru_blob_16135, 21 },
    { gMsg_ID_16140, sf64_ru_blob_16140, 41 },
    { gMsg_ID_16150, sf64_ru_blob_16150, 21 },
    { gMsg_ID_16160, sf64_ru_blob_16160, 46 },
    { gMsg_ID_16165, sf64_ru_blob_16165, 23 },
    { gMsg_ID_16170, sf64_ru_blob_16170, 20 },
    { gMsg_ID_16175, sf64_ru_blob_16175, 26 },
    { gMsg_ID_16180, sf64_ru_blob_16180, 23 },
    { gMsg_ID_16185, sf64_ru_blob_16185, 37 },
    { gMsg_ID_16200, sf64_ru_blob_16200, 25 },
    { gMsg_ID_16210, sf64_ru_blob_16210, 43 },
    { gMsg_ID_16220, sf64_ru_blob_16220, 29 },
    { gMsg_ID_16230, sf64_ru_blob_16230, 41 },
    { gMsg_ID_16240, sf64_ru_blob_16240, 14 },
    { gMsg_ID_16250, sf64_ru_blob_16250, 19 },
    { gMsg_ID_16260, sf64_ru_blob_16260, 26 },
    { gMsg_ID_16270, sf64_ru_blob_16270, 37 },
    { gMsg_ID_16280, sf64_ru_blob_16280, 30 },
    { gMsg_ID_17010, sf64_ru_blob_17010, 39 },
    { gMsg_ID_17020, sf64_ru_blob_17020, 39 },
    { gMsg_ID_17030, sf64_ru_blob_17030, 41 },
    { gMsg_ID_17100, sf64_ru_blob_17100, 45 },
    { gMsg_ID_17110, sf64_ru_blob_17110, 27 },
    { gMsg_ID_17120, sf64_ru_blob_17120, 42 },
    { gMsg_ID_17130, sf64_ru_blob_17130, 42 },
    { gMsg_ID_17131, sf64_ru_blob_17131, 25 },
    { gMsg_ID_17140, sf64_ru_blob_17140, 47 },
    { gMsg_ID_17150, sf64_ru_blob_17150, 30 },
    { gMsg_ID_17160, sf64_ru_blob_17160, 19 },
    { gMsg_ID_17170, sf64_ru_blob_17170, 45 },
    { gMsg_ID_17300, sf64_ru_blob_17300, 45 },
    { gMsg_ID_17310, sf64_ru_blob_17310, 34 },
    { gMsg_ID_17320, sf64_ru_blob_17320, 39 },
    { gMsg_ID_17330, sf64_ru_blob_17330, 45 },
    { gMsg_ID_17350, sf64_ru_blob_17350, 37 },
    { gMsg_ID_17360, sf64_ru_blob_17360, 37 },
    { gMsg_ID_17370, sf64_ru_blob_17370, 33 },
    { gMsg_ID_17380, sf64_ru_blob_17380, 38 },
    { gMsg_ID_17390, sf64_ru_blob_17390, 14 },
    { gMsg_ID_17400, sf64_ru_blob_17400, 35 },
    { gMsg_ID_17410, sf64_ru_blob_17410, 38 },
    { gMsg_ID_17420, sf64_ru_blob_17420, 39 },
    { gMsg_ID_17430, sf64_ru_blob_17430, 21 },
    { gMsg_ID_17440, sf64_ru_blob_17440, 11 },
    { gMsg_ID_17450, sf64_ru_blob_17450, 18 },
    { gMsg_ID_17460, sf64_ru_blob_17460, 31 },
    { gMsg_ID_17470, sf64_ru_blob_17470, 12 },
    { gMsg_ID_17471, sf64_ru_blob_17471, 25 },
    { gMsg_ID_17472, sf64_ru_blob_17472, 19 },
    { gMsg_ID_17473, sf64_ru_blob_17473, 19 },
    { gMsg_ID_17474, sf64_ru_blob_17474, 30 },
    { gMsg_ID_17475, sf64_ru_blob_17475, 37 },
    { gMsg_ID_17476, sf64_ru_blob_17476, 13 },
    { gMsg_ID_18000, sf64_ru_blob_18000, 37 },
    { gMsg_ID_18005, sf64_ru_blob_18005, 20 },
    { gMsg_ID_18006, sf64_ru_blob_18006, 42 },
    { gMsg_ID_18007, sf64_ru_blob_18007, 30 },
    { gMsg_ID_18010, sf64_ru_blob_18010, 44 },
    { gMsg_ID_18015, sf64_ru_blob_18015, 34 },
    { gMsg_ID_18018, sf64_ru_blob_18018, 28 },
    { gMsg_ID_18020, sf64_ru_blob_18020, 30 },
    { gMsg_ID_18021, sf64_ru_blob_18021, 48 },
    { gMsg_ID_18022, sf64_ru_blob_18022, 20 },
    { gMsg_ID_18025, sf64_ru_blob_18025, 44 },
    { gMsg_ID_18030, sf64_ru_blob_18030, 32 },
    { gMsg_ID_18031, sf64_ru_blob_18031, 32 },
    { gMsg_ID_18035, sf64_ru_blob_18035, 18 },
    { gMsg_ID_18040, sf64_ru_blob_18040, 43 },
    { gMsg_ID_18045, sf64_ru_blob_18045, 24 },
    { gMsg_ID_18050, sf64_ru_blob_18050, 40 },
    { gMsg_ID_18055, sf64_ru_blob_18055, 47 },
    { gMsg_ID_18060, sf64_ru_blob_18060, 36 },
    { gMsg_ID_18065, sf64_ru_blob_18065, 38 },
    { gMsg_ID_18066, sf64_ru_blob_18066, 25 },
    { gMsg_ID_18070, sf64_ru_blob_18070, 29 },
    { gMsg_ID_18075, sf64_ru_blob_18075, 31 },
    { gMsg_ID_18080, sf64_ru_blob_18080, 15 },
    { gMsg_ID_18085, sf64_ru_blob_18085, 27 },
    { gMsg_ID_18090, sf64_ru_blob_18090, 18 },
    { gMsg_ID_18095, sf64_ru_blob_18095, 20 },
    { gMsg_ID_18100, sf64_ru_blob_18100, 41 },
    { gMsg_ID_18105, sf64_ru_blob_18105, 18 },
    { gMsg_ID_18120, sf64_ru_blob_18120, 31 },
    { gMsg_ID_18130, sf64_ru_blob_18130, 28 },
    { gMsg_ID_18140, sf64_ru_blob_18140, 33 },
    { gMsg_ID_18150, sf64_ru_blob_18150, 45 },
    { gMsg_ID_19010, sf64_ru_blob_19010, 29 },
    { gMsg_ID_19200, sf64_ru_blob_19200, 34 },
    { gMsg_ID_19205, sf64_ru_blob_19205, 14 },
    { gMsg_ID_19210, sf64_ru_blob_19210, 43 },
    { gMsg_ID_19220, sf64_ru_blob_19220, 42 },
    { gMsg_ID_19230, sf64_ru_blob_19230, 44 },
    { gMsg_ID_19240, sf64_ru_blob_19240, 41 },
    { gMsg_ID_19250, sf64_ru_blob_19250, 26 },
    { gMsg_ID_19325, sf64_ru_blob_19325, 32 },
    { gMsg_ID_19330, sf64_ru_blob_19330, 42 },
    { gMsg_ID_19335, sf64_ru_blob_19335, 41 },
    { gMsg_ID_19340, sf64_ru_blob_19340, 18 },
    { gMsg_ID_19350, sf64_ru_blob_19350, 11 },
    { gMsg_ID_19355, sf64_ru_blob_19355, 17 },
    { gMsg_ID_19360, sf64_ru_blob_19360, 39 },
    { gMsg_ID_19370, sf64_ru_blob_19370, 32 },
    { gMsg_ID_19400, sf64_ru_blob_19400, 18 },
    { gMsg_ID_19410, sf64_ru_blob_19410, 18 },
    { gMsg_ID_19420, sf64_ru_blob_19420, 21 },
    { gMsg_ID_19430, sf64_ru_blob_19430, 21 },
    { gMsg_ID_19440, sf64_ru_blob_19440, 30 },
    { gMsg_ID_19450, sf64_ru_blob_19450, 31 },
    { gMsg_ID_19451, sf64_ru_blob_19451, 26 },
    { gMsg_ID_19452, sf64_ru_blob_19452, 36 },
    { gMsg_ID_19453, sf64_ru_blob_19453, 28 },
    { gMsg_ID_19454, sf64_ru_blob_19454, 32 },
    { gMsg_ID_19455, sf64_ru_blob_19455, 35 },
    { gMsg_ID_19456, sf64_ru_blob_19456, 47 },
    { gMsg_ID_19457, sf64_ru_blob_19457, 29 },
    { gMsg_ID_19458, sf64_ru_blob_19458, 31 },
    { gMsg_ID_19459, sf64_ru_blob_19459, 19 },
    { gMsg_ID_19460, sf64_ru_blob_19460, 27 },
    { gMsg_ID_19461, sf64_ru_blob_19461, 12 },
    { gMsg_ID_19462, sf64_ru_blob_19462, 31 },
    { gMsg_ID_19463, sf64_ru_blob_19463, 29 },
    { gMsg_ID_19464, sf64_ru_blob_19464, 22 },
    { gMsg_ID_19465, sf64_ru_blob_19465, 27 },
    { gMsg_ID_19466, sf64_ru_blob_19466, 41 },
    { gMsg_ID_19467, sf64_ru_blob_19467, 36 },
    { gMsg_ID_19468, sf64_ru_blob_19468, 31 },
    { gMsg_ID_20010, sf64_ru_blob_20010, 23 },
    { gMsg_ID_20011, sf64_ru_blob_20011, 22 },
    { gMsg_ID_20012, sf64_ru_blob_20012, 21 },
    { gMsg_ID_20013, sf64_ru_blob_20013, 32 },
    { gMsg_ID_20014, sf64_ru_blob_20014, 38 },
    { gMsg_ID_20015, sf64_ru_blob_20015, 27 },
    { gMsg_ID_20016, sf64_ru_blob_20016, 38 },
    { gMsg_ID_20017, sf64_ru_blob_20017, 34 },
    { gMsg_ID_20018, sf64_ru_blob_20018, 26 },
    { gMsg_ID_20019, sf64_ru_blob_20019, 34 },
    { gMsg_ID_20020, sf64_ru_blob_20020, 14 },
    { gMsg_ID_20030, sf64_ru_blob_20030, 29 },
    { gMsg_ID_20040, sf64_ru_blob_20040, 17 },
    { gMsg_ID_20050, sf64_ru_blob_20050, 16 },
    { gMsg_ID_20060, sf64_ru_blob_20060, 36 },
    { gMsg_ID_20070, sf64_ru_blob_20070, 21 },
    { gMsg_ID_20080, sf64_ru_blob_20080, 29 },
    { gMsg_ID_20084, sf64_ru_blob_20084, 42 },
    { gMsg_ID_20085, sf64_ru_blob_20085, 33 },
    { gMsg_ID_20090, sf64_ru_blob_20090, 9 },
    { gMsg_ID_20091, sf64_ru_blob_20091, 9 },
    { gMsg_ID_20092, sf64_ru_blob_20092, 9 },
    { gMsg_ID_20150, sf64_ru_blob_20150, 20 },
    { gMsg_ID_20160, sf64_ru_blob_20160, 29 },
    { gMsg_ID_20170, sf64_ru_blob_20170, 33 },
    { gMsg_ID_20180, sf64_ru_blob_20180, 18 },
    { gMsg_ID_20190, sf64_ru_blob_20190, 39 },
    { gMsg_ID_20200, sf64_ru_blob_20200, 30 },
    { gMsg_ID_20210, sf64_ru_blob_20210, 21 },
    { gMsg_ID_20220, sf64_ru_blob_20220, 39 },
    { gMsg_ID_20221, sf64_ru_blob_20221, 46 },
    { gMsg_ID_20222, sf64_ru_blob_20222, 36 },
    { gMsg_ID_20230, sf64_ru_blob_20230, 40 },
    { gMsg_ID_20235, sf64_ru_blob_20235, 33 },
    { gMsg_ID_20236, sf64_ru_blob_20236, 33 },
    { gMsg_ID_20237, sf64_ru_blob_20237, 29 },
    { gMsg_ID_20238, sf64_ru_blob_20238, 31 },
    { gMsg_ID_20239, sf64_ru_blob_20239, 36 },
    { gMsg_ID_20250, sf64_ru_blob_20250, 27 },
    { gMsg_ID_20260, sf64_ru_blob_20260, 19 },
    { gMsg_ID_20261, sf64_ru_blob_20261, 35 },
    { gMsg_ID_20262, sf64_ru_blob_20262, 37 },
    { gMsg_ID_20263, sf64_ru_blob_20263, 14 },
    { gMsg_ID_20264, sf64_ru_blob_20264, 9 },
    { gMsg_ID_20265, sf64_ru_blob_20265, 9 },
    { gMsg_ID_20266, sf64_ru_blob_20266, 10 },
    { gMsg_ID_20267, sf64_ru_blob_20267, 26 },
    { gMsg_ID_20268, sf64_ru_blob_20268, 29 },
    { gMsg_ID_20269, sf64_ru_blob_20269, 39 },
    { gMsg_ID_20270, sf64_ru_blob_20270, 12 },
    { gMsg_ID_20271, sf64_ru_blob_20271, 18 },
    { gMsg_ID_20272, sf64_ru_blob_20272, 12 },
    { gMsg_ID_20273, sf64_ru_blob_20273, 32 },
    { gMsg_ID_20274, sf64_ru_blob_20274, 32 },
    { gMsg_ID_20275, sf64_ru_blob_20275, 29 },
    { gMsg_ID_20276, sf64_ru_blob_20276, 38 },
    { gMsg_ID_20277, sf64_ru_blob_20277, 37 },
    { gMsg_ID_20278, sf64_ru_blob_20278, 18 },
    { gMsg_ID_20279, sf64_ru_blob_20279, 37 },
    { gMsg_ID_20280, sf64_ru_blob_20280, 30 },
    { gMsg_ID_20281, sf64_ru_blob_20281, 34 },
    { gMsg_ID_20282, sf64_ru_blob_20282, 32 },
    { gMsg_ID_20283, sf64_ru_blob_20283, 30 },
    { gMsg_ID_20284, sf64_ru_blob_20284, 34 },
    { gMsg_ID_20285, sf64_ru_blob_20285, 45 },
    { gMsg_ID_20286, sf64_ru_blob_20286, 23 },
    { gMsg_ID_20287, sf64_ru_blob_20287, 40 },
    { gMsg_ID_20288, sf64_ru_blob_20288, 35 },
    { gMsg_ID_20289, sf64_ru_blob_20289, 19 },
    { gMsg_ID_20290, sf64_ru_blob_20290, 39 },
    { gMsg_ID_20291, sf64_ru_blob_20291, 23 },
    { gMsg_ID_20292, sf64_ru_blob_20292, 28 },
    { gMsg_ID_20294, sf64_ru_blob_20294, 12 },
    { gMsg_ID_20296, sf64_ru_blob_20296, 31 },
    { gMsg_ID_20297, sf64_ru_blob_20297, 14 },
    { gMsg_ID_20298, sf64_ru_blob_20298, 40 },
    { gMsg_ID_20299, sf64_ru_blob_20299, 30 },
    { gMsg_ID_20300, sf64_ru_blob_20300, 30 },
    { gMsg_ID_20301, sf64_ru_blob_20301, 29 },
    { gMsg_ID_20302, sf64_ru_blob_20302, 23 },
    { gMsg_ID_20303, sf64_ru_blob_20303, 40 },
    { gMsg_ID_20304, sf64_ru_blob_20304, 40 },
    { gMsg_ID_20305, sf64_ru_blob_20305, 19 },
    { gMsg_ID_20306, sf64_ru_blob_20306, 17 },
    { gMsg_ID_20307, sf64_ru_blob_20307, 17 },
    { gMsg_ID_20308, sf64_ru_blob_20308, 27 },
    { gMsg_ID_20309, sf64_ru_blob_20309, 28 },
    { gMsg_ID_20310, sf64_ru_blob_20310, 8 },
    { gMsg_ID_20311, sf64_ru_blob_20311, 9 },
    { gMsg_ID_20312, sf64_ru_blob_20312, 14 },
    { gMsg_ID_20313, sf64_ru_blob_20313, 32 },
    { gMsg_ID_20314, sf64_ru_blob_20314, 44 },
    { gMsg_ID_20315, sf64_ru_blob_20315, 18 },
    { gMsg_ID_20316, sf64_ru_blob_20316, 18 },
    { gMsg_ID_20317, sf64_ru_blob_20317, 18 },
    { gMsg_ID_20318, sf64_ru_blob_20318, 12 },
    { gMsg_ID_20319, sf64_ru_blob_20319, 12 },
    { gMsg_ID_20320, sf64_ru_blob_20320, 12 },
    { gMsg_ID_20321, sf64_ru_blob_20321, 12 },
    { gMsg_ID_20326, sf64_ru_blob_20326, 29 },
    { gMsg_ID_20327, sf64_ru_blob_20327, 29 },
    { gMsg_ID_20328, sf64_ru_blob_20328, 12 },
    { gMsg_ID_20329, sf64_ru_blob_20329, 42 },
    { gMsg_ID_20330, sf64_ru_blob_20330, 43 },
    { gMsg_ID_20331, sf64_ru_blob_20331, 38 },
    { gMsg_ID_20332, sf64_ru_blob_20332, 35 },
    { gMsg_ID_20333, sf64_ru_blob_20333, 35 },
    { gMsg_ID_20337, sf64_ru_blob_20337, 38 },
    { gMsg_ID_20338, sf64_ru_blob_20338, 38 },
    { gMsg_ID_20339, sf64_ru_blob_20339, 39 },
    { gMsg_ID_20340, sf64_ru_blob_20340, 42 },
    { gMsg_ID_20343, sf64_ru_blob_20343, 37 },
    { gMsg_ID_20344, sf64_ru_blob_20344, 37 },
    { gMsg_ID_20345, sf64_ru_blob_20345, 38 },
    { gMsg_ID_21010, sf64_ru_blob_21010, 33 },
    { gMsg_ID_21020, sf64_ru_blob_21020, 64 },
    { gMsg_ID_21030, sf64_ru_blob_21030, 50 },
    { gMsg_ID_21050, sf64_ru_blob_21050, 29 },
    { gMsg_ID_21060, sf64_ru_blob_21060, 31 },
    { gMsg_ID_21070, sf64_ru_blob_21070, 19 },
    { gMsg_ID_21071, sf64_ru_blob_21071, 18 },
    { gMsg_ID_21072, sf64_ru_blob_21072, 16 },
    { gMsg_ID_21073, sf64_ru_blob_21073, 18 },
    { gMsg_ID_21080, sf64_ru_blob_21080, 19 },
    { gMsg_ID_21081, sf64_ru_blob_21081, 18 },
    { gMsg_ID_21082, sf64_ru_blob_21082, 16 },
    { gMsg_ID_21083, sf64_ru_blob_21083, 18 },
    { gMsg_ID_21090, sf64_ru_blob_21090, 19 },
    { gMsg_ID_21091, sf64_ru_blob_21091, 18 },
    { gMsg_ID_21092, sf64_ru_blob_21092, 16 },
    { gMsg_ID_21093, sf64_ru_blob_21093, 18 },
    { gMsg_ID_22000, sf64_ru_blob_22000, 10 },
    { gMsg_ID_22001, sf64_ru_blob_22001, 10 },
    { gMsg_ID_22002, sf64_ru_blob_22002, 10 },
    { gMsg_ID_22003, sf64_ru_blob_22003, 6 },
    { gMsg_ID_22004, sf64_ru_blob_22004, 11 },
    { gMsg_ID_22005, sf64_ru_blob_22005, 14 },
    { gMsg_ID_22006, sf64_ru_blob_22006, 7 },
    { gMsg_ID_22007, sf64_ru_blob_22007, 9 },
    { gMsg_ID_22008, sf64_ru_blob_22008, 9 },
    { gMsg_ID_22009, sf64_ru_blob_22009, 13 },
    { gMsg_ID_22010, sf64_ru_blob_22010, 8 },
    { gMsg_ID_22011, sf64_ru_blob_22011, 44 },
    { gMsg_ID_22012, sf64_ru_blob_22012, 12 },
    { gMsg_ID_22013, sf64_ru_blob_22013, 13 },
    { gMsg_ID_22014, sf64_ru_blob_22014, 12 },
    { gMsg_ID_22015, sf64_ru_blob_22015, 13 },
    { gMsg_ID_22016, sf64_ru_blob_22016, 13 },
    { gMsg_ID_22017, sf64_ru_blob_22017, 8 },
    { gMsg_ID_22018, sf64_ru_blob_22018, 13 },
    { gMsg_ID_22019, sf64_ru_blob_22019, 13 },
    { gMsg_ID_22020, sf64_ru_blob_22020, 13 },
    { gMsg_ID_23000, sf64_ru_blob_23000, 33 },
    { gMsg_ID_23001, sf64_ru_blob_23001, 51 },
    { gMsg_ID_23002, sf64_ru_blob_23002, 46 },
    { gMsg_ID_23003, sf64_ru_blob_23003, 59 },
    { gMsg_ID_23004, sf64_ru_blob_23004, 44 },
    { gMsg_ID_23005, sf64_ru_blob_23005, 43 },
    { gMsg_ID_23006, sf64_ru_blob_23006, 39 },
    { gMsg_ID_23007, sf64_ru_blob_23007, 40 },
    { gMsg_ID_23008, sf64_ru_blob_23008, 54 },
    { gMsg_ID_23009, sf64_ru_blob_23009, 43 },
    { gMsg_ID_23010, sf64_ru_blob_23010, 48 },
    { gMsg_ID_23011, sf64_ru_blob_23011, 43 },
    { gMsg_ID_23012, sf64_ru_blob_23012, 36 },
    { gMsg_ID_23013, sf64_ru_blob_23013, 38 },
    { gMsg_ID_23014, sf64_ru_blob_23014, 36 },
    { gMsg_ID_23015, sf64_ru_blob_23015, 24 },
    { gMsg_ID_23016, sf64_ru_blob_23016, 24 },
    { gMsg_ID_23017, sf64_ru_blob_23017, 35 },
    { gMsg_ID_23018, sf64_ru_blob_23018, 36 },
    { gMsg_ID_23019, sf64_ru_blob_23019, 30 },
    { gMsg_ID_23020, sf64_ru_blob_23020, 37 },
    { gMsg_ID_23021, sf64_ru_blob_23021, 42 },
    { gMsg_ID_23022, sf64_ru_blob_23022, 39 },
    { gMsg_ID_23023, sf64_ru_blob_23023, 30 },
    { gMsg_ID_23024, sf64_ru_blob_23024, 48 },
    { gMsg_ID_23025, sf64_ru_blob_23025, 50 },
    { gMsg_ID_23026, sf64_ru_blob_23026, 39 },
    { gMsg_ID_23027, sf64_ru_blob_23027, 38 },
    { gMsg_ID_23028, sf64_ru_blob_23028, 32 },
    { gMsg_ID_23029, sf64_ru_blob_23029, 43 },
    { gMsg_ID_23030, sf64_ru_blob_23030, 48 },
    { gMsg_ID_23031, sf64_ru_blob_23031, 51 },
    { gMsg_ID_23032, sf64_ru_blob_23032, 41 },
};
#define SF64_RU_RUNTIME_TABLE_COUNT 779

static const Sf64RuRadioExtEntry sf64_ru_radio_ext_table[] = {
    { 0x0001, gMsg_ID_1, sf64_ru_radio_blob_1, SF64_RU_RADIO_MAX_WORDS },
    { 0x000A, gMsg_ID_10, sf64_ru_radio_blob_10, SF64_RU_RADIO_MAX_WORDS },
    { 0x0014, gMsg_ID_20, sf64_ru_radio_blob_20, SF64_RU_RADIO_MAX_WORDS },
    { 0x001E, gMsg_ID_30, sf64_ru_radio_blob_30, SF64_RU_RADIO_MAX_WORDS },
    { 0x0028, gMsg_ID_40, sf64_ru_radio_blob_40, SF64_RU_RADIO_MAX_WORDS },
    { 0x0032, gMsg_ID_50, sf64_ru_radio_blob_50, SF64_RU_RADIO_MAX_WORDS },
    { 0x003C, gMsg_ID_60, sf64_ru_radio_blob_60, SF64_RU_RADIO_MAX_WORDS },
    { 0x04B0, gMsg_ID_1200, sf64_ru_radio_blob_1200, SF64_RU_RADIO_MAX_WORDS },
    { 0x04BA, gMsg_ID_1210, sf64_ru_radio_blob_1210, SF64_RU_RADIO_MAX_WORDS },
    { 0x04C4, gMsg_ID_1220, sf64_ru_radio_blob_1220, SF64_RU_RADIO_MAX_WORDS },
    { 0x04CE, gMsg_ID_1230, sf64_ru_radio_blob_1230, SF64_RU_RADIO_MAX_WORDS },
    { 0x04D8, gMsg_ID_1240, sf64_ru_radio_blob_1240, SF64_RU_RADIO_MAX_WORDS },
    { 0x04E2, gMsg_ID_1250, sf64_ru_radio_blob_1250, SF64_RU_RADIO_MAX_WORDS },
    { 0x04EC, gMsg_ID_1260, sf64_ru_radio_blob_1260, SF64_RU_RADIO_MAX_WORDS },
    { 0x04F6, gMsg_ID_1270, sf64_ru_radio_blob_1270, SF64_RU_RADIO_MAX_WORDS },
    { 0x0500, gMsg_ID_1280, sf64_ru_radio_blob_1280, SF64_RU_RADIO_MAX_WORDS },
    { 0x050A, gMsg_ID_1290, sf64_ru_radio_blob_1290, SF64_RU_RADIO_MAX_WORDS },
    { 0x0514, gMsg_ID_1300, sf64_ru_radio_blob_1300, SF64_RU_RADIO_MAX_WORDS },
    { 0x051E, gMsg_ID_1310, sf64_ru_radio_blob_1310, SF64_RU_RADIO_MAX_WORDS },
    { 0x0528, gMsg_ID_1320, sf64_ru_radio_blob_1320, SF64_RU_RADIO_MAX_WORDS },
    { 0x0532, gMsg_ID_1330, sf64_ru_radio_blob_1330, SF64_RU_RADIO_MAX_WORDS },
    { 0x053C, gMsg_ID_1340, sf64_ru_radio_blob_1340, SF64_RU_RADIO_MAX_WORDS },
    { 0x0546, gMsg_ID_1350, sf64_ru_radio_blob_1350, SF64_RU_RADIO_MAX_WORDS },
    { 0x0550, gMsg_ID_1360, sf64_ru_radio_blob_1360, SF64_RU_RADIO_MAX_WORDS },
    { 0x055A, gMsg_ID_1370, sf64_ru_radio_blob_1370, SF64_RU_RADIO_MAX_WORDS },
    { 0x0564, gMsg_ID_1380, sf64_ru_radio_blob_1380, SF64_RU_RADIO_MAX_WORDS },
    { 0x056E, gMsg_ID_1390, sf64_ru_radio_blob_1390, SF64_RU_RADIO_MAX_WORDS },
    { 0x0578, gMsg_ID_1400, sf64_ru_radio_blob_1400, SF64_RU_RADIO_MAX_WORDS },
    { 0x0582, gMsg_ID_1410, sf64_ru_radio_blob_1410, SF64_RU_RADIO_MAX_WORDS },
    { 0x058C, gMsg_ID_1420, sf64_ru_radio_blob_1420, SF64_RU_RADIO_MAX_WORDS },
    { 0x0596, gMsg_ID_1430, sf64_ru_radio_blob_1430, SF64_RU_RADIO_MAX_WORDS },
    { 0x05A0, gMsg_ID_1440, sf64_ru_radio_blob_1440, SF64_RU_RADIO_MAX_WORDS },
    { 0x05AA, gMsg_ID_1450, sf64_ru_radio_blob_1450, SF64_RU_RADIO_MAX_WORDS },
    { 0x05B4, gMsg_ID_1460, sf64_ru_radio_blob_1460, SF64_RU_RADIO_MAX_WORDS },
    { 0x05BE, gMsg_ID_1470, sf64_ru_radio_blob_1470, SF64_RU_RADIO_MAX_WORDS },
    { 0x59D8, gMsg_ID_23000, sf64_ru_radio_blob_23000, SF64_RU_RADIO_MAX_WORDS },
    { 0x59D9, gMsg_ID_23001, sf64_ru_radio_blob_23001, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DA, gMsg_ID_23002, sf64_ru_radio_blob_23002, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DB, gMsg_ID_23003, sf64_ru_radio_blob_23003, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DC, gMsg_ID_23004, sf64_ru_radio_blob_23004, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DD, gMsg_ID_23005, sf64_ru_radio_blob_23005, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DE, gMsg_ID_23006, sf64_ru_radio_blob_23006, SF64_RU_RADIO_MAX_WORDS },
    { 0x59DF, gMsg_ID_23007, sf64_ru_radio_blob_23007, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E0, gMsg_ID_23008, sf64_ru_radio_blob_23008, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E1, gMsg_ID_23009, sf64_ru_radio_blob_23009, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E2, gMsg_ID_23010, sf64_ru_radio_blob_23010, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E3, gMsg_ID_23011, sf64_ru_radio_blob_23011, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E4, gMsg_ID_23012, sf64_ru_radio_blob_23012, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E5, gMsg_ID_23013, sf64_ru_radio_blob_23013, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E6, gMsg_ID_23014, sf64_ru_radio_blob_23014, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E7, gMsg_ID_23015, sf64_ru_radio_blob_23015, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E8, gMsg_ID_23016, sf64_ru_radio_blob_23016, SF64_RU_RADIO_MAX_WORDS },
    { 0x59E9, gMsg_ID_23017, sf64_ru_radio_blob_23017, SF64_RU_RADIO_MAX_WORDS },
    { 0x59EA, gMsg_ID_23018, sf64_ru_radio_blob_23018, SF64_RU_RADIO_MAX_WORDS },
    { 0x59EB, gMsg_ID_23019, sf64_ru_radio_blob_23019, SF64_RU_RADIO_MAX_WORDS },
    { 0x59EC, gMsg_ID_23020, sf64_ru_radio_blob_23020, SF64_RU_RADIO_MAX_WORDS },
    { 0x59ED, gMsg_ID_23021, sf64_ru_radio_blob_23021, SF64_RU_RADIO_MAX_WORDS },
    { 0x59EE, gMsg_ID_23022, sf64_ru_radio_blob_23022, SF64_RU_RADIO_MAX_WORDS },
    { 0x59EF, gMsg_ID_23023, sf64_ru_radio_blob_23023, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F0, gMsg_ID_23024, sf64_ru_radio_blob_23024, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F1, gMsg_ID_23025, sf64_ru_radio_blob_23025, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F2, gMsg_ID_23026, sf64_ru_radio_blob_23026, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F3, gMsg_ID_23027, sf64_ru_radio_blob_23027, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F4, gMsg_ID_23028, sf64_ru_radio_blob_23028, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F5, gMsg_ID_23029, sf64_ru_radio_blob_23029, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F6, gMsg_ID_23030, sf64_ru_radio_blob_23030, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F7, gMsg_ID_23031, sf64_ru_radio_blob_23031, SF64_RU_RADIO_MAX_WORDS },
    { 0x59F8, gMsg_ID_23032, sf64_ru_radio_blob_23032, SF64_RU_RADIO_MAX_WORDS },
    { 0x07D5, gMsg_ID_2005, sf64_ru_radio_blob_2005, SF64_RU_RADIO_MAX_WORDS },
    { 0x07DA, gMsg_ID_2010, sf64_ru_radio_blob_2010, SF64_RU_RADIO_MAX_WORDS },
    { 0x07E4, gMsg_ID_2020, sf64_ru_radio_blob_2020, SF64_RU_RADIO_MAX_WORDS },
    { 0x07EE, gMsg_ID_2030, sf64_ru_radio_blob_2030, SF64_RU_RADIO_MAX_WORDS },
    { 0x07F8, gMsg_ID_2040, sf64_ru_radio_blob_2040, SF64_RU_RADIO_MAX_WORDS },
    { 0x0802, gMsg_ID_2050, sf64_ru_radio_blob_2050, SF64_RU_RADIO_MAX_WORDS },
    { 0x0807, gMsg_ID_2055, sf64_ru_radio_blob_2055, SF64_RU_RADIO_MAX_WORDS },
    { 0x080A, gMsg_ID_2058, sf64_ru_radio_blob_2058, SF64_RU_RADIO_MAX_WORDS },
    { 0x080D, gMsg_ID_2061, sf64_ru_radio_blob_2061, SF64_RU_RADIO_MAX_WORDS },
    { 0x080E, gMsg_ID_2062, sf64_ru_radio_blob_2062, SF64_RU_RADIO_MAX_WORDS },
    { 0x0820, gMsg_ID_2080, sf64_ru_radio_blob_2080, SF64_RU_RADIO_MAX_WORDS },
    { 0x082A, gMsg_ID_2090, sf64_ru_radio_blob_2090, SF64_RU_RADIO_MAX_WORDS },
    { 0x082F, gMsg_ID_2095, sf64_ru_radio_blob_2095, SF64_RU_RADIO_MAX_WORDS },
    { 0x083E, gMsg_ID_2110, sf64_ru_radio_blob_2110, SF64_RU_RADIO_MAX_WORDS },
    { 0x0843, gMsg_ID_2115, sf64_ru_radio_blob_2115, SF64_RU_RADIO_MAX_WORDS },
    { 0x0846, gMsg_ID_2118, sf64_ru_radio_blob_2118, SF64_RU_RADIO_MAX_WORDS },
    { 0x085C, gMsg_ID_2140, sf64_ru_radio_blob_2140, SF64_RU_RADIO_MAX_WORDS },
    { 0x0875, gMsg_ID_2165, sf64_ru_radio_blob_2165, SF64_RU_RADIO_MAX_WORDS },
    { 0x0876, gMsg_ID_2166, sf64_ru_radio_blob_2166, SF64_RU_RADIO_MAX_WORDS },
    { 0x0877, gMsg_ID_2167, sf64_ru_radio_blob_2167, SF64_RU_RADIO_MAX_WORDS },
    { 0x0884, gMsg_ID_2180, sf64_ru_radio_blob_2180, SF64_RU_RADIO_MAX_WORDS },
    { 0x0885, gMsg_ID_2181, sf64_ru_radio_blob_2181, SF64_RU_RADIO_MAX_WORDS },
    { 0x0889, gMsg_ID_2185, sf64_ru_radio_blob_2185, SF64_RU_RADIO_MAX_WORDS },
    { 0x088C, gMsg_ID_2188, sf64_ru_radio_blob_2188, SF64_RU_RADIO_MAX_WORDS },
    { 0x0898, gMsg_ID_2200, sf64_ru_radio_blob_2200, SF64_RU_RADIO_MAX_WORDS },
    { 0x08A2, gMsg_ID_2210, sf64_ru_radio_blob_2210, SF64_RU_RADIO_MAX_WORDS },
    { 0x08AC, gMsg_ID_2220, sf64_ru_radio_blob_2220, SF64_RU_RADIO_MAX_WORDS },
    { 0x08B1, gMsg_ID_2225, sf64_ru_radio_blob_2225, SF64_RU_RADIO_MAX_WORDS },
    { 0x08B6, gMsg_ID_2230, sf64_ru_radio_blob_2230, SF64_RU_RADIO_MAX_WORDS },
    { 0x08B9, gMsg_ID_2233, sf64_ru_radio_blob_2233, SF64_RU_RADIO_MAX_WORDS },
    { 0x08C0, gMsg_ID_2240, sf64_ru_radio_blob_2240, SF64_RU_RADIO_MAX_WORDS },
    { 0x08CA, gMsg_ID_2250, sf64_ru_radio_blob_2250, SF64_RU_RADIO_MAX_WORDS },
    { 0x08D4, gMsg_ID_2260, sf64_ru_radio_blob_2260, SF64_RU_RADIO_MAX_WORDS },
    { 0x08D7, gMsg_ID_2263, sf64_ru_radio_blob_2263, SF64_RU_RADIO_MAX_WORDS },
    { 0x08D9, gMsg_ID_2265, sf64_ru_radio_blob_2265, SF64_RU_RADIO_MAX_WORDS },
    { 0x08DE, gMsg_ID_2270, sf64_ru_radio_blob_2270, SF64_RU_RADIO_MAX_WORDS },
    { 0x08E3, gMsg_ID_2275, sf64_ru_radio_blob_2275, SF64_RU_RADIO_MAX_WORDS },
    { 0x08E8, gMsg_ID_2280, sf64_ru_radio_blob_2280, SF64_RU_RADIO_MAX_WORDS },
    { 0x08EA, gMsg_ID_2282, sf64_ru_radio_blob_2282, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F2, gMsg_ID_2290, sf64_ru_radio_blob_2290, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F3, gMsg_ID_2291, sf64_ru_radio_blob_2291, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F4, gMsg_ID_2292, sf64_ru_radio_blob_2292, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F5, gMsg_ID_2293, sf64_ru_radio_blob_2293, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F6, gMsg_ID_2294, sf64_ru_radio_blob_2294, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F7, gMsg_ID_2295, sf64_ru_radio_blob_2295, SF64_RU_RADIO_MAX_WORDS },
    { 0x08F8, gMsg_ID_2296, sf64_ru_radio_blob_2296, SF64_RU_RADIO_MAX_WORDS },
    { 0x08FA, gMsg_ID_2298, sf64_ru_radio_blob_2298, SF64_RU_RADIO_MAX_WORDS },
    { 0x08FB, gMsg_ID_2299, sf64_ru_radio_blob_2299, SF64_RU_RADIO_MAX_WORDS },
    { 0x08FC, gMsg_ID_2300, sf64_ru_radio_blob_2300, SF64_RU_RADIO_MAX_WORDS },
    { 0x0901, gMsg_ID_2305, sf64_ru_radio_blob_2305, SF64_RU_RADIO_MAX_WORDS },
    { 0x0906, gMsg_ID_2310, sf64_ru_radio_blob_2310, SF64_RU_RADIO_MAX_WORDS },
    { 0x0910, gMsg_ID_2320, sf64_ru_radio_blob_2320, SF64_RU_RADIO_MAX_WORDS },
    { 0x091F, gMsg_ID_2335, sf64_ru_radio_blob_2335, SF64_RU_RADIO_MAX_WORDS },
    { 0x0920, gMsg_ID_2336, sf64_ru_radio_blob_2336, SF64_RU_RADIO_MAX_WORDS },
    { 0x0921, gMsg_ID_2337, sf64_ru_radio_blob_2337, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BBD, gMsg_ID_3005, sf64_ru_radio_blob_3005, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BC2, gMsg_ID_3010, sf64_ru_radio_blob_3010, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BC7, gMsg_ID_3015, sf64_ru_radio_blob_3015, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BCC, gMsg_ID_3020, sf64_ru_radio_blob_3020, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BD1, gMsg_ID_3025, sf64_ru_radio_blob_3025, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BD2, gMsg_ID_3026, sf64_ru_radio_blob_3026, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BE0, gMsg_ID_3040, sf64_ru_radio_blob_3040, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BE1, gMsg_ID_3041, sf64_ru_radio_blob_3041, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BE2, gMsg_ID_3042, sf64_ru_radio_blob_3042, SF64_RU_RADIO_MAX_WORDS },
    { 0x0BEA, gMsg_ID_3050, sf64_ru_radio_blob_3050, SF64_RU_RADIO_MAX_WORDS },
    { 0x0C1C, gMsg_ID_3100, sf64_ru_radio_blob_3100, SF64_RU_RADIO_MAX_WORDS },
    { 0x0C26, gMsg_ID_3110, sf64_ru_radio_blob_3110, SF64_RU_RADIO_MAX_WORDS },
    { 0x0C30, gMsg_ID_3120, sf64_ru_radio_blob_3120, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CE4, gMsg_ID_3300, sf64_ru_radio_blob_3300, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CEE, gMsg_ID_3310, sf64_ru_radio_blob_3310, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CF3, gMsg_ID_3315, sf64_ru_radio_blob_3315, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CF8, gMsg_ID_3320, sf64_ru_radio_blob_3320, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CF9, gMsg_ID_3321, sf64_ru_radio_blob_3321, SF64_RU_RADIO_MAX_WORDS },
    { 0x0CFA, gMsg_ID_3322, sf64_ru_radio_blob_3322, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D02, gMsg_ID_3330, sf64_ru_radio_blob_3330, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D0C, gMsg_ID_3340, sf64_ru_radio_blob_3340, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D11, gMsg_ID_3345, sf64_ru_radio_blob_3345, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D16, gMsg_ID_3350, sf64_ru_radio_blob_3350, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D20, gMsg_ID_3360, sf64_ru_radio_blob_3360, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D2A, gMsg_ID_3370, sf64_ru_radio_blob_3370, SF64_RU_RADIO_MAX_WORDS },
    { 0x0D2B, gMsg_ID_3371, sf64_ru_radio_blob_3371, SF64_RU_RADIO_MAX_WORDS },
    { 0x36C4, gMsg_ID_14020, sf64_ru_radio_blob_14020, SF64_RU_RADIO_MAX_WORDS },
    { 0x36CE, gMsg_ID_14030, sf64_ru_radio_blob_14030, SF64_RU_RADIO_MAX_WORDS },
    { 0x36D8, gMsg_ID_14040, sf64_ru_radio_blob_14040, SF64_RU_RADIO_MAX_WORDS },
    { 0x36DD, gMsg_ID_14045, sf64_ru_radio_blob_14045, SF64_RU_RADIO_MAX_WORDS },
    { 0x36E2, gMsg_ID_14050, sf64_ru_radio_blob_14050, SF64_RU_RADIO_MAX_WORDS },
    { 0x36EC, gMsg_ID_14060, sf64_ru_radio_blob_14060, SF64_RU_RADIO_MAX_WORDS },
    { 0x36F6, gMsg_ID_14070, sf64_ru_radio_blob_14070, SF64_RU_RADIO_MAX_WORDS },
    { 0x3700, gMsg_ID_14080, sf64_ru_radio_blob_14080, SF64_RU_RADIO_MAX_WORDS },
    { 0x3714, gMsg_ID_14100, sf64_ru_radio_blob_14100, SF64_RU_RADIO_MAX_WORDS },
    { 0x371E, gMsg_ID_14110, sf64_ru_radio_blob_14110, SF64_RU_RADIO_MAX_WORDS },
    { 0x3728, gMsg_ID_14120, sf64_ru_radio_blob_14120, SF64_RU_RADIO_MAX_WORDS },
    { 0x3732, gMsg_ID_14130, sf64_ru_radio_blob_14130, SF64_RU_RADIO_MAX_WORDS },
    { 0x373C, gMsg_ID_14140, sf64_ru_radio_blob_14140, SF64_RU_RADIO_MAX_WORDS },
    { 0x3746, gMsg_ID_14150, sf64_ru_radio_blob_14150, SF64_RU_RADIO_MAX_WORDS },
    { 0x3750, gMsg_ID_14160, sf64_ru_radio_blob_14160, SF64_RU_RADIO_MAX_WORDS },
    { 0x375A, gMsg_ID_14170, sf64_ru_radio_blob_14170, SF64_RU_RADIO_MAX_WORDS },
    { 0x3764, gMsg_ID_14180, sf64_ru_radio_blob_14180, SF64_RU_RADIO_MAX_WORDS },
    { 0x376E, gMsg_ID_14190, sf64_ru_radio_blob_14190, SF64_RU_RADIO_MAX_WORDS },
    { 0x3778, gMsg_ID_14200, sf64_ru_radio_blob_14200, SF64_RU_RADIO_MAX_WORDS },
    { 0x3782, gMsg_ID_14210, sf64_ru_radio_blob_14210, SF64_RU_RADIO_MAX_WORDS },
    { 0x378C, gMsg_ID_14220, sf64_ru_radio_blob_14220, SF64_RU_RADIO_MAX_WORDS },
    { 0x3796, gMsg_ID_14230, sf64_ru_radio_blob_14230, SF64_RU_RADIO_MAX_WORDS },
    { 0x37DC, gMsg_ID_14300, sf64_ru_radio_blob_14300, SF64_RU_RADIO_MAX_WORDS },
    { 0x37E6, gMsg_ID_14310, sf64_ru_radio_blob_14310, SF64_RU_RADIO_MAX_WORDS },
    { 0x37F0, gMsg_ID_14320, sf64_ru_radio_blob_14320, SF64_RU_RADIO_MAX_WORDS },
    { 0x37FA, gMsg_ID_14330, sf64_ru_radio_blob_14330, SF64_RU_RADIO_MAX_WORDS },
    { 0x3804, gMsg_ID_14340, sf64_ru_radio_blob_14340, SF64_RU_RADIO_MAX_WORDS },
    { 0x380E, gMsg_ID_14350, sf64_ru_radio_blob_14350, SF64_RU_RADIO_MAX_WORDS },
    { 0x3818, gMsg_ID_14360, sf64_ru_radio_blob_14360, SF64_RU_RADIO_MAX_WORDS },
    { 0x3822, gMsg_ID_14370, sf64_ru_radio_blob_14370, SF64_RU_RADIO_MAX_WORDS },
    { 0x2328, gMsg_ID_9000, sf64_ru_radio_blob_9000, SF64_RU_RADIO_MAX_WORDS },
    { 0x2332, gMsg_ID_9010, sf64_ru_radio_blob_9010, SF64_RU_RADIO_MAX_WORDS },
    { 0x249F, gMsg_ID_9375, sf64_ru_radio_blob_9375, SF64_RU_RADIO_MAX_WORDS },
    { 0x24A4, gMsg_ID_9380, sf64_ru_radio_blob_9380, SF64_RU_RADIO_MAX_WORDS },
    { 0x24A9, gMsg_ID_9385, sf64_ru_radio_blob_9385, SF64_RU_RADIO_MAX_WORDS },
    { 0x24AE, gMsg_ID_9390, sf64_ru_radio_blob_9390, SF64_RU_RADIO_MAX_WORDS },
    { 0x24B3, gMsg_ID_9395, sf64_ru_radio_blob_9395, SF64_RU_RADIO_MAX_WORDS },
    { 0x24B8, gMsg_ID_9400, sf64_ru_radio_blob_9400, SF64_RU_RADIO_MAX_WORDS },
    { 0x24BD, gMsg_ID_9405, sf64_ru_radio_blob_9405, SF64_RU_RADIO_MAX_WORDS },
    { 0x24C3, gMsg_ID_9411, sf64_ru_radio_blob_9411, SF64_RU_RADIO_MAX_WORDS },
    { 0x24CC, gMsg_ID_9420, sf64_ru_radio_blob_9420, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D1, gMsg_ID_9425, sf64_ru_radio_blob_9425, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D2, gMsg_ID_9426, sf64_ru_radio_blob_9426, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D3, gMsg_ID_9427, sf64_ru_radio_blob_9427, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D4, gMsg_ID_9428, sf64_ru_radio_blob_9428, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D5, gMsg_ID_9429, sf64_ru_radio_blob_9429, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D6, gMsg_ID_9430, sf64_ru_radio_blob_9430, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D7, gMsg_ID_9431, sf64_ru_radio_blob_9431, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D8, gMsg_ID_9432, sf64_ru_radio_blob_9432, SF64_RU_RADIO_MAX_WORDS },
    { 0x24D9, gMsg_ID_9433, sf64_ru_radio_blob_9433, SF64_RU_RADIO_MAX_WORDS },
    { 0x24DA, gMsg_ID_9434, sf64_ru_radio_blob_9434, SF64_RU_RADIO_MAX_WORDS },
    { 0x24DC, gMsg_ID_9436, sf64_ru_radio_blob_9436, SF64_RU_RADIO_MAX_WORDS },
    { 0x24DD, gMsg_ID_9437, sf64_ru_radio_blob_9437, SF64_RU_RADIO_MAX_WORDS },
    { 0x24DE, gMsg_ID_9438, sf64_ru_radio_blob_9438, SF64_RU_RADIO_MAX_WORDS },
    { 0x238C, gMsg_ID_9100, sf64_ru_radio_blob_9100, SF64_RU_RADIO_MAX_WORDS },
    { 0x2396, gMsg_ID_9110, sf64_ru_radio_blob_9110, SF64_RU_RADIO_MAX_WORDS },
    { 0x23A0, gMsg_ID_9120, sf64_ru_radio_blob_9120, SF64_RU_RADIO_MAX_WORDS },
    { 0x23AA, gMsg_ID_9130, sf64_ru_radio_blob_9130, SF64_RU_RADIO_MAX_WORDS },
    { 0x23B4, gMsg_ID_9140, sf64_ru_radio_blob_9140, SF64_RU_RADIO_MAX_WORDS },
    { 0x23BE, gMsg_ID_9150, sf64_ru_radio_blob_9150, SF64_RU_RADIO_MAX_WORDS },
    { 0x23BF, gMsg_ID_9151, sf64_ru_radio_blob_9151, SF64_RU_RADIO_MAX_WORDS },
    { 0x23C0, gMsg_ID_9152, sf64_ru_radio_blob_9152, SF64_RU_RADIO_MAX_WORDS },
    { 0x23C1, gMsg_ID_9153, sf64_ru_radio_blob_9153, SF64_RU_RADIO_MAX_WORDS },
    { 0x23C8, gMsg_ID_9160, sf64_ru_radio_blob_9160, SF64_RU_RADIO_MAX_WORDS },
    { 0x23D2, gMsg_ID_9170, sf64_ru_radio_blob_9170, SF64_RU_RADIO_MAX_WORDS },
    { 0x23DC, gMsg_ID_9180, sf64_ru_radio_blob_9180, SF64_RU_RADIO_MAX_WORDS },
    { 0x23E6, gMsg_ID_9190, sf64_ru_radio_blob_9190, SF64_RU_RADIO_MAX_WORDS },
    { 0x23F0, gMsg_ID_9200, sf64_ru_radio_blob_9200, SF64_RU_RADIO_MAX_WORDS },
    { 0x23FA, gMsg_ID_9210, sf64_ru_radio_blob_9210, SF64_RU_RADIO_MAX_WORDS },
    { 0x23FB, gMsg_ID_9211, sf64_ru_radio_blob_9211, SF64_RU_RADIO_MAX_WORDS },
    { 0x23FC, gMsg_ID_9212, sf64_ru_radio_blob_9212, SF64_RU_RADIO_MAX_WORDS },
    { 0x23FD, gMsg_ID_9213, sf64_ru_radio_blob_9213, SF64_RU_RADIO_MAX_WORDS },
    { 0x2404, gMsg_ID_9220, sf64_ru_radio_blob_9220, SF64_RU_RADIO_MAX_WORDS },
    { 0x240E, gMsg_ID_9230, sf64_ru_radio_blob_9230, SF64_RU_RADIO_MAX_WORDS },
    { 0x2418, gMsg_ID_9240, sf64_ru_radio_blob_9240, SF64_RU_RADIO_MAX_WORDS },
    { 0x2422, gMsg_ID_9250, sf64_ru_radio_blob_9250, SF64_RU_RADIO_MAX_WORDS },
    { 0x242C, gMsg_ID_9260, sf64_ru_radio_blob_9260, SF64_RU_RADIO_MAX_WORDS },
    { 0x2436, gMsg_ID_9270, sf64_ru_radio_blob_9270, SF64_RU_RADIO_MAX_WORDS },
    { 0x243B, gMsg_ID_9275, sf64_ru_radio_blob_9275, SF64_RU_RADIO_MAX_WORDS },
    { 0x2440, gMsg_ID_9280, sf64_ru_radio_blob_9280, SF64_RU_RADIO_MAX_WORDS },
    { 0x2445, gMsg_ID_9285, sf64_ru_radio_blob_9285, SF64_RU_RADIO_MAX_WORDS },
    { 0x2449, gMsg_ID_9289, sf64_ru_radio_blob_9289, SF64_RU_RADIO_MAX_WORDS },
    { 0x244A, gMsg_ID_9290, sf64_ru_radio_blob_9290, SF64_RU_RADIO_MAX_WORDS },
    { 0x2454, gMsg_ID_9300, sf64_ru_radio_blob_9300, SF64_RU_RADIO_MAX_WORDS },
    { 0x245E, gMsg_ID_9310, sf64_ru_radio_blob_9310, SF64_RU_RADIO_MAX_WORDS },
    { 0x2468, gMsg_ID_9320, sf64_ru_radio_blob_9320, SF64_RU_RADIO_MAX_WORDS },
    { 0x246A, gMsg_ID_9322, sf64_ru_radio_blob_9322, SF64_RU_RADIO_MAX_WORDS },
    { 0x246B, gMsg_ID_9323, sf64_ru_radio_blob_9323, SF64_RU_RADIO_MAX_WORDS },
    { 0x246C, gMsg_ID_9324, sf64_ru_radio_blob_9324, SF64_RU_RADIO_MAX_WORDS },
    { 0x246D, gMsg_ID_9325, sf64_ru_radio_blob_9325, SF64_RU_RADIO_MAX_WORDS },
    { 0x2472, gMsg_ID_9330, sf64_ru_radio_blob_9330, SF64_RU_RADIO_MAX_WORDS },
    { 0x247C, gMsg_ID_9340, sf64_ru_radio_blob_9340, SF64_RU_RADIO_MAX_WORDS },
    { 0x2486, gMsg_ID_9350, sf64_ru_radio_blob_9350, SF64_RU_RADIO_MAX_WORDS },
    { 0x2490, gMsg_ID_9360, sf64_ru_radio_blob_9360, SF64_RU_RADIO_MAX_WORDS },
    { 0x2495, gMsg_ID_9365, sf64_ru_radio_blob_9365, SF64_RU_RADIO_MAX_WORDS },
    { 0x2496, gMsg_ID_9366, sf64_ru_radio_blob_9366, SF64_RU_RADIO_MAX_WORDS },
    { 0x2497, gMsg_ID_9367, sf64_ru_radio_blob_9367, SF64_RU_RADIO_MAX_WORDS },
    { 0x2498, gMsg_ID_9368, sf64_ru_radio_blob_9368, SF64_RU_RADIO_MAX_WORDS },
    { 0x2499, gMsg_ID_9369, sf64_ru_radio_blob_9369, SF64_RU_RADIO_MAX_WORDS },
    { 0x4650, gMsg_ID_18000, sf64_ru_radio_blob_18000, SF64_RU_RADIO_MAX_WORDS },
    { 0x4655, gMsg_ID_18005, sf64_ru_radio_blob_18005, SF64_RU_RADIO_MAX_WORDS },
    { 0x4656, gMsg_ID_18006, sf64_ru_radio_blob_18006, SF64_RU_RADIO_MAX_WORDS },
    { 0x4657, gMsg_ID_18007, sf64_ru_radio_blob_18007, SF64_RU_RADIO_MAX_WORDS },
    { 0x465A, gMsg_ID_18010, sf64_ru_radio_blob_18010, SF64_RU_RADIO_MAX_WORDS },
    { 0x465F, gMsg_ID_18015, sf64_ru_radio_blob_18015, SF64_RU_RADIO_MAX_WORDS },
    { 0x4662, gMsg_ID_18018, sf64_ru_radio_blob_18018, SF64_RU_RADIO_MAX_WORDS },
    { 0x4664, gMsg_ID_18020, sf64_ru_radio_blob_18020, SF64_RU_RADIO_MAX_WORDS },
    { 0x4665, gMsg_ID_18021, sf64_ru_radio_blob_18021, SF64_RU_RADIO_MAX_WORDS },
    { 0x4666, gMsg_ID_18022, sf64_ru_radio_blob_18022, SF64_RU_RADIO_MAX_WORDS },
    { 0x4669, gMsg_ID_18025, sf64_ru_radio_blob_18025, SF64_RU_RADIO_MAX_WORDS },
    { 0x466E, gMsg_ID_18030, sf64_ru_radio_blob_18030, SF64_RU_RADIO_MAX_WORDS },
    { 0x466F, gMsg_ID_18031, sf64_ru_radio_blob_18031, SF64_RU_RADIO_MAX_WORDS },
    { 0x4673, gMsg_ID_18035, sf64_ru_radio_blob_18035, SF64_RU_RADIO_MAX_WORDS },
    { 0x4678, gMsg_ID_18040, sf64_ru_radio_blob_18040, SF64_RU_RADIO_MAX_WORDS },
    { 0x467D, gMsg_ID_18045, sf64_ru_radio_blob_18045, SF64_RU_RADIO_MAX_WORDS },
    { 0x4682, gMsg_ID_18050, sf64_ru_radio_blob_18050, SF64_RU_RADIO_MAX_WORDS },
    { 0x4687, gMsg_ID_18055, sf64_ru_radio_blob_18055, SF64_RU_RADIO_MAX_WORDS },
    { 0x468C, gMsg_ID_18060, sf64_ru_radio_blob_18060, SF64_RU_RADIO_MAX_WORDS },
    { 0x4691, gMsg_ID_18065, sf64_ru_radio_blob_18065, SF64_RU_RADIO_MAX_WORDS },
    { 0x4692, gMsg_ID_18066, sf64_ru_radio_blob_18066, SF64_RU_RADIO_MAX_WORDS },
    { 0x4696, gMsg_ID_18070, sf64_ru_radio_blob_18070, SF64_RU_RADIO_MAX_WORDS },
    { 0x469B, gMsg_ID_18075, sf64_ru_radio_blob_18075, SF64_RU_RADIO_MAX_WORDS },
    { 0x46A0, gMsg_ID_18080, sf64_ru_radio_blob_18080, SF64_RU_RADIO_MAX_WORDS },
    { 0x46A5, gMsg_ID_18085, sf64_ru_radio_blob_18085, SF64_RU_RADIO_MAX_WORDS },
    { 0x46AA, gMsg_ID_18090, sf64_ru_radio_blob_18090, SF64_RU_RADIO_MAX_WORDS },
    { 0x46AF, gMsg_ID_18095, sf64_ru_radio_blob_18095, SF64_RU_RADIO_MAX_WORDS },
    { 0x46B4, gMsg_ID_18100, sf64_ru_radio_blob_18100, SF64_RU_RADIO_MAX_WORDS },
    { 0x46B9, gMsg_ID_18105, sf64_ru_radio_blob_18105, SF64_RU_RADIO_MAX_WORDS },
    { 0x46C8, gMsg_ID_18120, sf64_ru_radio_blob_18120, SF64_RU_RADIO_MAX_WORDS },
    { 0x46D2, gMsg_ID_18130, sf64_ru_radio_blob_18130, SF64_RU_RADIO_MAX_WORDS },
    { 0x46DC, gMsg_ID_18140, sf64_ru_radio_blob_18140, SF64_RU_RADIO_MAX_WORDS },
    { 0x46E6, gMsg_ID_18150, sf64_ru_radio_blob_18150, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AA2, gMsg_ID_15010, sf64_ru_radio_blob_15010, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AB6, gMsg_ID_15030, sf64_ru_radio_blob_15030, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AC0, gMsg_ID_15040, sf64_ru_radio_blob_15040, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AC5, gMsg_ID_15045, sf64_ru_radio_blob_15045, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ACA, gMsg_ID_15050, sf64_ru_radio_blob_15050, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ACB, gMsg_ID_15051, sf64_ru_radio_blob_15051, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ACC, gMsg_ID_15052, sf64_ru_radio_blob_15052, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ACD, gMsg_ID_15053, sf64_ru_radio_blob_15053, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ACE, gMsg_ID_15054, sf64_ru_radio_blob_15054, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AD4, gMsg_ID_15060, sf64_ru_radio_blob_15060, SF64_RU_RADIO_MAX_WORDS },
    { 0x3AFC, gMsg_ID_15100, sf64_ru_radio_blob_15100, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B06, gMsg_ID_15110, sf64_ru_radio_blob_15110, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B10, gMsg_ID_15120, sf64_ru_radio_blob_15120, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B1A, gMsg_ID_15130, sf64_ru_radio_blob_15130, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B24, gMsg_ID_15140, sf64_ru_radio_blob_15140, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B60, gMsg_ID_15200, sf64_ru_radio_blob_15200, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B6A, gMsg_ID_15210, sf64_ru_radio_blob_15210, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B74, gMsg_ID_15220, sf64_ru_radio_blob_15220, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B7E, gMsg_ID_15230, sf64_ru_radio_blob_15230, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B88, gMsg_ID_15240, sf64_ru_radio_blob_15240, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B92, gMsg_ID_15250, sf64_ru_radio_blob_15250, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B93, gMsg_ID_15251, sf64_ru_radio_blob_15251, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B94, gMsg_ID_15252, sf64_ru_radio_blob_15252, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B95, gMsg_ID_15253, sf64_ru_radio_blob_15253, SF64_RU_RADIO_MAX_WORDS },
    { 0x3B96, gMsg_ID_15254, sf64_ru_radio_blob_15254, SF64_RU_RADIO_MAX_WORDS },
    { 0x1388, gMsg_ID_5000, sf64_ru_radio_blob_5000, SF64_RU_RADIO_MAX_WORDS },
    { 0x1392, gMsg_ID_5010, sf64_ru_radio_blob_5010, SF64_RU_RADIO_MAX_WORDS },
    { 0x13C4, gMsg_ID_5060, sf64_ru_radio_blob_5060, SF64_RU_RADIO_MAX_WORDS },
    { 0x13D8, gMsg_ID_5080, sf64_ru_radio_blob_5080, SF64_RU_RADIO_MAX_WORDS },
    { 0x13EC, gMsg_ID_5100, sf64_ru_radio_blob_5100, SF64_RU_RADIO_MAX_WORDS },
    { 0x13F6, gMsg_ID_5110, sf64_ru_radio_blob_5110, SF64_RU_RADIO_MAX_WORDS },
    { 0x140A, gMsg_ID_5130, sf64_ru_radio_blob_5130, SF64_RU_RADIO_MAX_WORDS },
    { 0x1464, gMsg_ID_5220, sf64_ru_radio_blob_5220, SF64_RU_RADIO_MAX_WORDS },
    { 0x146E, gMsg_ID_5230, sf64_ru_radio_blob_5230, SF64_RU_RADIO_MAX_WORDS },
    { 0x14B4, gMsg_ID_5300, sf64_ru_radio_blob_5300, SF64_RU_RADIO_MAX_WORDS },
    { 0x14BE, gMsg_ID_5310, sf64_ru_radio_blob_5310, SF64_RU_RADIO_MAX_WORDS },
    { 0x14BF, gMsg_ID_5311, sf64_ru_radio_blob_5311, SF64_RU_RADIO_MAX_WORDS },
    { 0x14C0, gMsg_ID_5312, sf64_ru_radio_blob_5312, SF64_RU_RADIO_MAX_WORDS },
    { 0x14C1, gMsg_ID_5313, sf64_ru_radio_blob_5313, SF64_RU_RADIO_MAX_WORDS },
    { 0x14C2, gMsg_ID_5314, sf64_ru_radio_blob_5314, SF64_RU_RADIO_MAX_WORDS },
    { 0x14E6, gMsg_ID_5350, sf64_ru_radio_blob_5350, SF64_RU_RADIO_MAX_WORDS },
    { 0x14F0, gMsg_ID_5360, sf64_ru_radio_blob_5360, SF64_RU_RADIO_MAX_WORDS },
    { 0x1504, gMsg_ID_5380, sf64_ru_radio_blob_5380, SF64_RU_RADIO_MAX_WORDS },
    { 0x1518, gMsg_ID_5400, sf64_ru_radio_blob_5400, SF64_RU_RADIO_MAX_WORDS },
    { 0x1522, gMsg_ID_5410, sf64_ru_radio_blob_5410, SF64_RU_RADIO_MAX_WORDS },
    { 0x152C, gMsg_ID_5420, sf64_ru_radio_blob_5420, SF64_RU_RADIO_MAX_WORDS },
    { 0x1536, gMsg_ID_5430, sf64_ru_radio_blob_5430, SF64_RU_RADIO_MAX_WORDS },
    { 0x1554, gMsg_ID_5460, sf64_ru_radio_blob_5460, SF64_RU_RADIO_MAX_WORDS },
    { 0x155E, gMsg_ID_5470, sf64_ru_radio_blob_5470, SF64_RU_RADIO_MAX_WORDS },
    { 0x1561, gMsg_ID_5473, sf64_ru_radio_blob_5473, SF64_RU_RADIO_MAX_WORDS },
    { 0x1562, gMsg_ID_5474, sf64_ru_radio_blob_5474, SF64_RU_RADIO_MAX_WORDS },
    { 0x1563, gMsg_ID_5475, sf64_ru_radio_blob_5475, SF64_RU_RADIO_MAX_WORDS },
    { 0x1574, gMsg_ID_5492, sf64_ru_radio_blob_5492, SF64_RU_RADIO_MAX_WORDS },
    { 0x1575, gMsg_ID_5493, sf64_ru_radio_blob_5493, SF64_RU_RADIO_MAX_WORDS },
    { 0x1576, gMsg_ID_5494, sf64_ru_radio_blob_5494, SF64_RU_RADIO_MAX_WORDS },
    { 0x1577, gMsg_ID_5495, sf64_ru_radio_blob_5495, SF64_RU_RADIO_MAX_WORDS },
    { 0x1578, gMsg_ID_5496, sf64_ru_radio_blob_5496, SF64_RU_RADIO_MAX_WORDS },
    { 0x1579, gMsg_ID_5497, sf64_ru_radio_blob_5497, SF64_RU_RADIO_MAX_WORDS },
    { 0x157A, gMsg_ID_5498, sf64_ru_radio_blob_5498, SF64_RU_RADIO_MAX_WORDS },
    { 0x157B, gMsg_ID_5499, sf64_ru_radio_blob_5499, SF64_RU_RADIO_MAX_WORDS },
    { 0x157C, gMsg_ID_5500, sf64_ru_radio_blob_5500, SF64_RU_RADIO_MAX_WORDS },
    { 0x157D, gMsg_ID_5501, sf64_ru_radio_blob_5501, SF64_RU_RADIO_MAX_WORDS },
    { 0x157E, gMsg_ID_5502, sf64_ru_radio_blob_5502, SF64_RU_RADIO_MAX_WORDS },
    { 0x157F, gMsg_ID_5503, sf64_ru_radio_blob_5503, SF64_RU_RADIO_MAX_WORDS },
    { 0x1580, gMsg_ID_5504, sf64_ru_radio_blob_5504, SF64_RU_RADIO_MAX_WORDS },
    { 0x1581, gMsg_ID_5505, sf64_ru_radio_blob_5505, SF64_RU_RADIO_MAX_WORDS },
    { 0x1582, gMsg_ID_5506, sf64_ru_radio_blob_5506, SF64_RU_RADIO_MAX_WORDS },
    { 0x271A, gMsg_ID_10010, sf64_ru_radio_blob_10010, SF64_RU_RADIO_MAX_WORDS },
    { 0x2724, gMsg_ID_10020, sf64_ru_radio_blob_10020, SF64_RU_RADIO_MAX_WORDS },
    { 0x2738, gMsg_ID_10040, sf64_ru_radio_blob_10040, SF64_RU_RADIO_MAX_WORDS },
    { 0x2742, gMsg_ID_10050, sf64_ru_radio_blob_10050, SF64_RU_RADIO_MAX_WORDS },
    { 0x274C, gMsg_ID_10060, sf64_ru_radio_blob_10060, SF64_RU_RADIO_MAX_WORDS },
    { 0x2756, gMsg_ID_10070, sf64_ru_radio_blob_10070, SF64_RU_RADIO_MAX_WORDS },
    { 0x2760, gMsg_ID_10080, sf64_ru_radio_blob_10080, SF64_RU_RADIO_MAX_WORDS },
    { 0x27D8, gMsg_ID_10200, sf64_ru_radio_blob_10200, SF64_RU_RADIO_MAX_WORDS },
    { 0x27E2, gMsg_ID_10210, sf64_ru_radio_blob_10210, SF64_RU_RADIO_MAX_WORDS },
    { 0x27EC, gMsg_ID_10220, sf64_ru_radio_blob_10220, SF64_RU_RADIO_MAX_WORDS },
    { 0x27F6, gMsg_ID_10230, sf64_ru_radio_blob_10230, SF64_RU_RADIO_MAX_WORDS },
    { 0x280F, gMsg_ID_10255, sf64_ru_radio_blob_10255, SF64_RU_RADIO_MAX_WORDS },
    { 0x283C, gMsg_ID_10300, sf64_ru_radio_blob_10300, SF64_RU_RADIO_MAX_WORDS },
    { 0x2846, gMsg_ID_10310, sf64_ru_radio_blob_10310, SF64_RU_RADIO_MAX_WORDS },
    { 0x2850, gMsg_ID_10320, sf64_ru_radio_blob_10320, SF64_RU_RADIO_MAX_WORDS },
    { 0x2851, gMsg_ID_10321, sf64_ru_radio_blob_10321, SF64_RU_RADIO_MAX_WORDS },
    { 0x2852, gMsg_ID_10322, sf64_ru_radio_blob_10322, SF64_RU_RADIO_MAX_WORDS },
    { 0x2853, gMsg_ID_10323, sf64_ru_radio_blob_10323, SF64_RU_RADIO_MAX_WORDS },
    { 0x2854, gMsg_ID_10324, sf64_ru_radio_blob_10324, SF64_RU_RADIO_MAX_WORDS },
    { 0x177A, gMsg_ID_6010, sf64_ru_radio_blob_6010, SF64_RU_RADIO_MAX_WORDS },
    { 0x177B, gMsg_ID_6011, sf64_ru_radio_blob_6011, SF64_RU_RADIO_MAX_WORDS },
    { 0x177C, gMsg_ID_6012, sf64_ru_radio_blob_6012, SF64_RU_RADIO_MAX_WORDS },
    { 0x177D, gMsg_ID_6013, sf64_ru_radio_blob_6013, SF64_RU_RADIO_MAX_WORDS },
    { 0x177E, gMsg_ID_6014, sf64_ru_radio_blob_6014, SF64_RU_RADIO_MAX_WORDS },
    { 0x1784, gMsg_ID_6020, sf64_ru_radio_blob_6020, SF64_RU_RADIO_MAX_WORDS },
    { 0x1785, gMsg_ID_6021, sf64_ru_radio_blob_6021, SF64_RU_RADIO_MAX_WORDS },
    { 0x1788, gMsg_ID_6024, sf64_ru_radio_blob_6024, SF64_RU_RADIO_MAX_WORDS },
    { 0x1789, gMsg_ID_6025, sf64_ru_radio_blob_6025, SF64_RU_RADIO_MAX_WORDS },
    { 0x178A, gMsg_ID_6026, sf64_ru_radio_blob_6026, SF64_RU_RADIO_MAX_WORDS },
    { 0x178B, gMsg_ID_6027, sf64_ru_radio_blob_6027, SF64_RU_RADIO_MAX_WORDS },
    { 0x178C, gMsg_ID_6028, sf64_ru_radio_blob_6028, SF64_RU_RADIO_MAX_WORDS },
    { 0x178D, gMsg_ID_6029, sf64_ru_radio_blob_6029, SF64_RU_RADIO_MAX_WORDS },
    { 0x1794, gMsg_ID_6036, sf64_ru_radio_blob_6036, SF64_RU_RADIO_MAX_WORDS },
    { 0x1796, gMsg_ID_6038, sf64_ru_radio_blob_6038, SF64_RU_RADIO_MAX_WORDS },
    { 0x1799, gMsg_ID_6041, sf64_ru_radio_blob_6041, SF64_RU_RADIO_MAX_WORDS },
    { 0x179A, gMsg_ID_6042, sf64_ru_radio_blob_6042, SF64_RU_RADIO_MAX_WORDS },
    { 0x179D, gMsg_ID_6045, sf64_ru_radio_blob_6045, SF64_RU_RADIO_MAX_WORDS },
    { 0x17A2, gMsg_ID_6050, sf64_ru_radio_blob_6050, SF64_RU_RADIO_MAX_WORDS },
    { 0x17A3, gMsg_ID_6051, sf64_ru_radio_blob_6051, SF64_RU_RADIO_MAX_WORDS },
    { 0x17A7, gMsg_ID_6055, sf64_ru_radio_blob_6055, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B2, gMsg_ID_6066, sf64_ru_radio_blob_6066, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B3, gMsg_ID_6067, sf64_ru_radio_blob_6067, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B4, gMsg_ID_6068, sf64_ru_radio_blob_6068, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B5, gMsg_ID_6069, sf64_ru_radio_blob_6069, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B7, gMsg_ID_6071, sf64_ru_radio_blob_6071, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B8, gMsg_ID_6072, sf64_ru_radio_blob_6072, SF64_RU_RADIO_MAX_WORDS },
    { 0x17B9, gMsg_ID_6073, sf64_ru_radio_blob_6073, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BA, gMsg_ID_6074, sf64_ru_radio_blob_6074, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BB, gMsg_ID_6075, sf64_ru_radio_blob_6075, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BC, gMsg_ID_6076, sf64_ru_radio_blob_6076, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BD, gMsg_ID_6077, sf64_ru_radio_blob_6077, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BE, gMsg_ID_6078, sf64_ru_radio_blob_6078, SF64_RU_RADIO_MAX_WORDS },
    { 0x17BF, gMsg_ID_6079, sf64_ru_radio_blob_6079, SF64_RU_RADIO_MAX_WORDS },
    { 0x17C0, gMsg_ID_6080, sf64_ru_radio_blob_6080, SF64_RU_RADIO_MAX_WORDS },
    { 0x17C1, gMsg_ID_6081, sf64_ru_radio_blob_6081, SF64_RU_RADIO_MAX_WORDS },
    { 0x17C2, gMsg_ID_6082, sf64_ru_radio_blob_6082, SF64_RU_RADIO_MAX_WORDS },
    { 0x17CA, gMsg_ID_6090, sf64_ru_radio_blob_6090, SF64_RU_RADIO_MAX_WORDS },
    { 0x17D4, gMsg_ID_6100, sf64_ru_radio_blob_6100, SF64_RU_RADIO_MAX_WORDS },
    { 0x17D5, gMsg_ID_6101, sf64_ru_radio_blob_6101, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FAA, gMsg_ID_4010, sf64_ru_radio_blob_4010, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FAB, gMsg_ID_4011, sf64_ru_radio_blob_4011, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FAC, gMsg_ID_4012, sf64_ru_radio_blob_4012, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FAD, gMsg_ID_4013, sf64_ru_radio_blob_4013, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FB4, gMsg_ID_4020, sf64_ru_radio_blob_4020, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FB5, gMsg_ID_4021, sf64_ru_radio_blob_4021, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FB6, gMsg_ID_4022, sf64_ru_radio_blob_4022, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FB7, gMsg_ID_4023, sf64_ru_radio_blob_4023, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FB8, gMsg_ID_4024, sf64_ru_radio_blob_4024, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FBE, gMsg_ID_4030, sf64_ru_radio_blob_4030, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FBF, gMsg_ID_4031, sf64_ru_radio_blob_4031, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FC8, gMsg_ID_4040, sf64_ru_radio_blob_4040, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FD2, gMsg_ID_4050, sf64_ru_radio_blob_4050, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FEB, gMsg_ID_4075, sf64_ru_radio_blob_4075, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FF0, gMsg_ID_4080, sf64_ru_radio_blob_4080, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FF2, gMsg_ID_4082, sf64_ru_radio_blob_4082, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FF3, gMsg_ID_4083, sf64_ru_radio_blob_4083, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FFB, gMsg_ID_4091, sf64_ru_radio_blob_4091, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FFC, gMsg_ID_4092, sf64_ru_radio_blob_4092, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FFD, gMsg_ID_4093, sf64_ru_radio_blob_4093, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FFE, gMsg_ID_4094, sf64_ru_radio_blob_4094, SF64_RU_RADIO_MAX_WORDS },
    { 0x0FFF, gMsg_ID_4095, sf64_ru_radio_blob_4095, SF64_RU_RADIO_MAX_WORDS },
    { 0x1000, gMsg_ID_4096, sf64_ru_radio_blob_4096, SF64_RU_RADIO_MAX_WORDS },
    { 0x1001, gMsg_ID_4097, sf64_ru_radio_blob_4097, SF64_RU_RADIO_MAX_WORDS },
    { 0x1002, gMsg_ID_4098, sf64_ru_radio_blob_4098, SF64_RU_RADIO_MAX_WORDS },
    { 0x1003, gMsg_ID_4099, sf64_ru_radio_blob_4099, SF64_RU_RADIO_MAX_WORDS },
    { 0x1004, gMsg_ID_4100, sf64_ru_radio_blob_4100, SF64_RU_RADIO_MAX_WORDS },
    { 0x1005, gMsg_ID_4101, sf64_ru_radio_blob_4101, SF64_RU_RADIO_MAX_WORDS },
    { 0x1006, gMsg_ID_4102, sf64_ru_radio_blob_4102, SF64_RU_RADIO_MAX_WORDS },
    { 0x1007, gMsg_ID_4103, sf64_ru_radio_blob_4103, SF64_RU_RADIO_MAX_WORDS },
    { 0x100E, gMsg_ID_4110, sf64_ru_radio_blob_4110, SF64_RU_RADIO_MAX_WORDS },
    { 0x100F, gMsg_ID_4111, sf64_ru_radio_blob_4111, SF64_RU_RADIO_MAX_WORDS },
    { 0x1010, gMsg_ID_4112, sf64_ru_radio_blob_4112, SF64_RU_RADIO_MAX_WORDS },
    { 0x1011, gMsg_ID_4113, sf64_ru_radio_blob_4113, SF64_RU_RADIO_MAX_WORDS },
    { 0x4272, gMsg_ID_17010, sf64_ru_radio_blob_17010, SF64_RU_RADIO_MAX_WORDS },
    { 0x427C, gMsg_ID_17020, sf64_ru_radio_blob_17020, SF64_RU_RADIO_MAX_WORDS },
    { 0x4286, gMsg_ID_17030, sf64_ru_radio_blob_17030, SF64_RU_RADIO_MAX_WORDS },
    { 0x42CC, gMsg_ID_17100, sf64_ru_radio_blob_17100, SF64_RU_RADIO_MAX_WORDS },
    { 0x42D6, gMsg_ID_17110, sf64_ru_radio_blob_17110, SF64_RU_RADIO_MAX_WORDS },
    { 0x42E0, gMsg_ID_17120, sf64_ru_radio_blob_17120, SF64_RU_RADIO_MAX_WORDS },
    { 0x42EA, gMsg_ID_17130, sf64_ru_radio_blob_17130, SF64_RU_RADIO_MAX_WORDS },
    { 0x42EB, gMsg_ID_17131, sf64_ru_radio_blob_17131, SF64_RU_RADIO_MAX_WORDS },
    { 0x42F4, gMsg_ID_17140, sf64_ru_radio_blob_17140, SF64_RU_RADIO_MAX_WORDS },
    { 0x42FE, gMsg_ID_17150, sf64_ru_radio_blob_17150, SF64_RU_RADIO_MAX_WORDS },
    { 0x4308, gMsg_ID_17160, sf64_ru_radio_blob_17160, SF64_RU_RADIO_MAX_WORDS },
    { 0x4312, gMsg_ID_17170, sf64_ru_radio_blob_17170, SF64_RU_RADIO_MAX_WORDS },
    { 0x4394, gMsg_ID_17300, sf64_ru_radio_blob_17300, SF64_RU_RADIO_MAX_WORDS },
    { 0x439E, gMsg_ID_17310, sf64_ru_radio_blob_17310, SF64_RU_RADIO_MAX_WORDS },
    { 0x43A8, gMsg_ID_17320, sf64_ru_radio_blob_17320, SF64_RU_RADIO_MAX_WORDS },
    { 0x43B2, gMsg_ID_17330, sf64_ru_radio_blob_17330, SF64_RU_RADIO_MAX_WORDS },
    { 0x43C6, gMsg_ID_17350, sf64_ru_radio_blob_17350, SF64_RU_RADIO_MAX_WORDS },
    { 0x43D0, gMsg_ID_17360, sf64_ru_radio_blob_17360, SF64_RU_RADIO_MAX_WORDS },
    { 0x43DA, gMsg_ID_17370, sf64_ru_radio_blob_17370, SF64_RU_RADIO_MAX_WORDS },
    { 0x43E4, gMsg_ID_17380, sf64_ru_radio_blob_17380, SF64_RU_RADIO_MAX_WORDS },
    { 0x43EE, gMsg_ID_17390, sf64_ru_radio_blob_17390, SF64_RU_RADIO_MAX_WORDS },
    { 0x43F8, gMsg_ID_17400, sf64_ru_radio_blob_17400, SF64_RU_RADIO_MAX_WORDS },
    { 0x4402, gMsg_ID_17410, sf64_ru_radio_blob_17410, SF64_RU_RADIO_MAX_WORDS },
    { 0x440C, gMsg_ID_17420, sf64_ru_radio_blob_17420, SF64_RU_RADIO_MAX_WORDS },
    { 0x4416, gMsg_ID_17430, sf64_ru_radio_blob_17430, SF64_RU_RADIO_MAX_WORDS },
    { 0x4420, gMsg_ID_17440, sf64_ru_radio_blob_17440, SF64_RU_RADIO_MAX_WORDS },
    { 0x442A, gMsg_ID_17450, sf64_ru_radio_blob_17450, SF64_RU_RADIO_MAX_WORDS },
    { 0x4434, gMsg_ID_17460, sf64_ru_radio_blob_17460, SF64_RU_RADIO_MAX_WORDS },
    { 0x443E, gMsg_ID_17470, sf64_ru_radio_blob_17470, SF64_RU_RADIO_MAX_WORDS },
    { 0x443F, gMsg_ID_17471, sf64_ru_radio_blob_17471, SF64_RU_RADIO_MAX_WORDS },
    { 0x4440, gMsg_ID_17472, sf64_ru_radio_blob_17472, SF64_RU_RADIO_MAX_WORDS },
    { 0x4441, gMsg_ID_17473, sf64_ru_radio_blob_17473, SF64_RU_RADIO_MAX_WORDS },
    { 0x4442, gMsg_ID_17474, sf64_ru_radio_blob_17474, SF64_RU_RADIO_MAX_WORDS },
    { 0x4443, gMsg_ID_17475, sf64_ru_radio_blob_17475, SF64_RU_RADIO_MAX_WORDS },
    { 0x4444, gMsg_ID_17476, sf64_ru_radio_blob_17476, SF64_RU_RADIO_MAX_WORDS },
    { 0x3E8A, gMsg_ID_16010, sf64_ru_radio_blob_16010, SF64_RU_RADIO_MAX_WORDS },
    { 0x3E94, gMsg_ID_16020, sf64_ru_radio_blob_16020, SF64_RU_RADIO_MAX_WORDS },
    { 0x3E9E, gMsg_ID_16030, sf64_ru_radio_blob_16030, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EA8, gMsg_ID_16040, sf64_ru_radio_blob_16040, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EAE, gMsg_ID_16046, sf64_ru_radio_blob_16046, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EAF, gMsg_ID_16047, sf64_ru_radio_blob_16047, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EB2, gMsg_ID_16050, sf64_ru_radio_blob_16050, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EB7, gMsg_ID_16055, sf64_ru_radio_blob_16055, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EBC, gMsg_ID_16060, sf64_ru_radio_blob_16060, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ED0, gMsg_ID_16080, sf64_ru_radio_blob_16080, SF64_RU_RADIO_MAX_WORDS },
    { 0x3ED5, gMsg_ID_16085, sf64_ru_radio_blob_16085, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EDA, gMsg_ID_16090, sf64_ru_radio_blob_16090, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EE4, gMsg_ID_16100, sf64_ru_radio_blob_16100, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EEE, gMsg_ID_16110, sf64_ru_radio_blob_16110, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EF8, gMsg_ID_16120, sf64_ru_radio_blob_16120, SF64_RU_RADIO_MAX_WORDS },
    { 0x3EFD, gMsg_ID_16125, sf64_ru_radio_blob_16125, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F02, gMsg_ID_16130, sf64_ru_radio_blob_16130, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F07, gMsg_ID_16135, sf64_ru_radio_blob_16135, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F0C, gMsg_ID_16140, sf64_ru_radio_blob_16140, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F16, gMsg_ID_16150, sf64_ru_radio_blob_16150, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F20, gMsg_ID_16160, sf64_ru_radio_blob_16160, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F25, gMsg_ID_16165, sf64_ru_radio_blob_16165, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F2A, gMsg_ID_16170, sf64_ru_radio_blob_16170, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F2F, gMsg_ID_16175, sf64_ru_radio_blob_16175, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F34, gMsg_ID_16180, sf64_ru_radio_blob_16180, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F39, gMsg_ID_16185, sf64_ru_radio_blob_16185, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F48, gMsg_ID_16200, sf64_ru_radio_blob_16200, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F52, gMsg_ID_16210, sf64_ru_radio_blob_16210, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F5C, gMsg_ID_16220, sf64_ru_radio_blob_16220, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F66, gMsg_ID_16230, sf64_ru_radio_blob_16230, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F70, gMsg_ID_16240, sf64_ru_radio_blob_16240, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F7A, gMsg_ID_16250, sf64_ru_radio_blob_16250, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F84, gMsg_ID_16260, sf64_ru_radio_blob_16260, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F8E, gMsg_ID_16270, sf64_ru_radio_blob_16270, SF64_RU_RADIO_MAX_WORDS },
    { 0x3F98, gMsg_ID_16280, sf64_ru_radio_blob_16280, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B02, gMsg_ID_11010, sf64_ru_radio_blob_11010, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B0C, gMsg_ID_11020, sf64_ru_radio_blob_11020, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B16, gMsg_ID_11030, sf64_ru_radio_blob_11030, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B20, gMsg_ID_11040, sf64_ru_radio_blob_11040, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B2A, gMsg_ID_11050, sf64_ru_radio_blob_11050, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B34, gMsg_ID_11060, sf64_ru_radio_blob_11060, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B5C, gMsg_ID_11100, sf64_ru_radio_blob_11100, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B66, gMsg_ID_11110, sf64_ru_radio_blob_11110, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B70, gMsg_ID_11120, sf64_ru_radio_blob_11120, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B7A, gMsg_ID_11130, sf64_ru_radio_blob_11130, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B8E, gMsg_ID_11150, sf64_ru_radio_blob_11150, SF64_RU_RADIO_MAX_WORDS },
    { 0x2B98, gMsg_ID_11160, sf64_ru_radio_blob_11160, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BC0, gMsg_ID_11200, sf64_ru_radio_blob_11200, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BCA, gMsg_ID_11210, sf64_ru_radio_blob_11210, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BD4, gMsg_ID_11220, sf64_ru_radio_blob_11220, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BDE, gMsg_ID_11230, sf64_ru_radio_blob_11230, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BE8, gMsg_ID_11240, sf64_ru_radio_blob_11240, SF64_RU_RADIO_MAX_WORDS },
    { 0x2BE9, gMsg_ID_11241, sf64_ru_radio_blob_11241, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B5D, gMsg_ID_7005, sf64_ru_radio_blob_7005, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B5E, gMsg_ID_7006, sf64_ru_radio_blob_7006, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B63, gMsg_ID_7011, sf64_ru_radio_blob_7011, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B64, gMsg_ID_7012, sf64_ru_radio_blob_7012, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B65, gMsg_ID_7013, sf64_ru_radio_blob_7013, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B66, gMsg_ID_7014, sf64_ru_radio_blob_7014, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B6C, gMsg_ID_7020, sf64_ru_radio_blob_7020, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B83, gMsg_ID_7043, sf64_ru_radio_blob_7043, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B8A, gMsg_ID_7050, sf64_ru_radio_blob_7050, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B8B, gMsg_ID_7051, sf64_ru_radio_blob_7051, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B8C, gMsg_ID_7052, sf64_ru_radio_blob_7052, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B8D, gMsg_ID_7053, sf64_ru_radio_blob_7053, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B8E, gMsg_ID_7054, sf64_ru_radio_blob_7054, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B95, gMsg_ID_7061, sf64_ru_radio_blob_7061, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B98, gMsg_ID_7064, sf64_ru_radio_blob_7064, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B99, gMsg_ID_7065, sf64_ru_radio_blob_7065, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B9A, gMsg_ID_7066, sf64_ru_radio_blob_7066, SF64_RU_RADIO_MAX_WORDS },
    { 0x1B9E, gMsg_ID_7070, sf64_ru_radio_blob_7070, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BAB, gMsg_ID_7083, sf64_ru_radio_blob_7083, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BAC, gMsg_ID_7084, sf64_ru_radio_blob_7084, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BAD, gMsg_ID_7085, sf64_ru_radio_blob_7085, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BAE, gMsg_ID_7086, sf64_ru_radio_blob_7086, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BAF, gMsg_ID_7087, sf64_ru_radio_blob_7087, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BB5, gMsg_ID_7093, sf64_ru_radio_blob_7093, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BB6, gMsg_ID_7094, sf64_ru_radio_blob_7094, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BB7, gMsg_ID_7095, sf64_ru_radio_blob_7095, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BB8, gMsg_ID_7096, sf64_ru_radio_blob_7096, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BB9, gMsg_ID_7097, sf64_ru_radio_blob_7097, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BBA, gMsg_ID_7098, sf64_ru_radio_blob_7098, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BBB, gMsg_ID_7099, sf64_ru_radio_blob_7099, SF64_RU_RADIO_MAX_WORDS },
    { 0x1BBC, gMsg_ID_7100, sf64_ru_radio_blob_7100, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F4A, gMsg_ID_8010, sf64_ru_radio_blob_8010, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F54, gMsg_ID_8020, sf64_ru_radio_blob_8020, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F5E, gMsg_ID_8030, sf64_ru_radio_blob_8030, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F68, gMsg_ID_8040, sf64_ru_radio_blob_8040, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F6D, gMsg_ID_8045, sf64_ru_radio_blob_8045, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F72, gMsg_ID_8050, sf64_ru_radio_blob_8050, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F7C, gMsg_ID_8060, sf64_ru_radio_blob_8060, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F86, gMsg_ID_8070, sf64_ru_radio_blob_8070, SF64_RU_RADIO_MAX_WORDS },
    { 0x1F90, gMsg_ID_8080, sf64_ru_radio_blob_8080, SF64_RU_RADIO_MAX_WORDS },
    { 0x1FA4, gMsg_ID_8100, sf64_ru_radio_blob_8100, SF64_RU_RADIO_MAX_WORDS },
    { 0x1FAE, gMsg_ID_8110, sf64_ru_radio_blob_8110, SF64_RU_RADIO_MAX_WORDS },
    { 0x1FB8, gMsg_ID_8120, sf64_ru_radio_blob_8120, SF64_RU_RADIO_MAX_WORDS },
    { 0x1FC2, gMsg_ID_8130, sf64_ru_radio_blob_8130, SF64_RU_RADIO_MAX_WORDS },
    { 0x1FCC, gMsg_ID_8140, sf64_ru_radio_blob_8140, SF64_RU_RADIO_MAX_WORDS },
    { 0x200D, gMsg_ID_8205, sf64_ru_radio_blob_8205, SF64_RU_RADIO_MAX_WORDS },
    { 0x2012, gMsg_ID_8210, sf64_ru_radio_blob_8210, SF64_RU_RADIO_MAX_WORDS },
    { 0x2017, gMsg_ID_8215, sf64_ru_radio_blob_8215, SF64_RU_RADIO_MAX_WORDS },
    { 0x201C, gMsg_ID_8220, sf64_ru_radio_blob_8220, SF64_RU_RADIO_MAX_WORDS },
    { 0x2026, gMsg_ID_8230, sf64_ru_radio_blob_8230, SF64_RU_RADIO_MAX_WORDS },
    { 0x2030, gMsg_ID_8240, sf64_ru_radio_blob_8240, SF64_RU_RADIO_MAX_WORDS },
    { 0x203A, gMsg_ID_8250, sf64_ru_radio_blob_8250, SF64_RU_RADIO_MAX_WORDS },
    { 0x203F, gMsg_ID_8255, sf64_ru_radio_blob_8255, SF64_RU_RADIO_MAX_WORDS },
    { 0x2044, gMsg_ID_8260, sf64_ru_radio_blob_8260, SF64_RU_RADIO_MAX_WORDS },
    { 0x2049, gMsg_ID_8265, sf64_ru_radio_blob_8265, SF64_RU_RADIO_MAX_WORDS },
    { 0x206C, gMsg_ID_8300, sf64_ru_radio_blob_8300, SF64_RU_RADIO_MAX_WORDS },
    { 0x2076, gMsg_ID_8310, sf64_ru_radio_blob_8310, SF64_RU_RADIO_MAX_WORDS },
    { 0x2080, gMsg_ID_8320, sf64_ru_radio_blob_8320, SF64_RU_RADIO_MAX_WORDS },
    { 0x4A42, gMsg_ID_19010, sf64_ru_radio_blob_19010, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B7D, gMsg_ID_19325, sf64_ru_radio_blob_19325, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B82, gMsg_ID_19330, sf64_ru_radio_blob_19330, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B87, gMsg_ID_19335, sf64_ru_radio_blob_19335, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B8C, gMsg_ID_19340, sf64_ru_radio_blob_19340, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B96, gMsg_ID_19350, sf64_ru_radio_blob_19350, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B9B, gMsg_ID_19355, sf64_ru_radio_blob_19355, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BA0, gMsg_ID_19360, sf64_ru_radio_blob_19360, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BAA, gMsg_ID_19370, sf64_ru_radio_blob_19370, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C0A, gMsg_ID_19466, sf64_ru_radio_blob_19466, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C0B, gMsg_ID_19467, sf64_ru_radio_blob_19467, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C0C, gMsg_ID_19468, sf64_ru_radio_blob_19468, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B00, gMsg_ID_19200, sf64_ru_radio_blob_19200, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B05, gMsg_ID_19205, sf64_ru_radio_blob_19205, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B0A, gMsg_ID_19210, sf64_ru_radio_blob_19210, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B14, gMsg_ID_19220, sf64_ru_radio_blob_19220, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B1E, gMsg_ID_19230, sf64_ru_radio_blob_19230, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B28, gMsg_ID_19240, sf64_ru_radio_blob_19240, SF64_RU_RADIO_MAX_WORDS },
    { 0x4B32, gMsg_ID_19250, sf64_ru_radio_blob_19250, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFA, gMsg_ID_19450, sf64_ru_radio_blob_19450, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFB, gMsg_ID_19451, sf64_ru_radio_blob_19451, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFC, gMsg_ID_19452, sf64_ru_radio_blob_19452, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFD, gMsg_ID_19453, sf64_ru_radio_blob_19453, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFE, gMsg_ID_19454, sf64_ru_radio_blob_19454, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BFF, gMsg_ID_19455, sf64_ru_radio_blob_19455, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C00, gMsg_ID_19456, sf64_ru_radio_blob_19456, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C01, gMsg_ID_19457, sf64_ru_radio_blob_19457, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C02, gMsg_ID_19458, sf64_ru_radio_blob_19458, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C03, gMsg_ID_19459, sf64_ru_radio_blob_19459, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C04, gMsg_ID_19460, sf64_ru_radio_blob_19460, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C05, gMsg_ID_19461, sf64_ru_radio_blob_19461, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C06, gMsg_ID_19462, sf64_ru_radio_blob_19462, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C07, gMsg_ID_19463, sf64_ru_radio_blob_19463, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C08, gMsg_ID_19464, sf64_ru_radio_blob_19464, SF64_RU_RADIO_MAX_WORDS },
    { 0x4C09, gMsg_ID_19465, sf64_ru_radio_blob_19465, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BC8, gMsg_ID_19400, sf64_ru_radio_blob_19400, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BD2, gMsg_ID_19410, sf64_ru_radio_blob_19410, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BDC, gMsg_ID_19420, sf64_ru_radio_blob_19420, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BE6, gMsg_ID_19430, sf64_ru_radio_blob_19430, SF64_RU_RADIO_MAX_WORDS },
    { 0x4BF0, gMsg_ID_19440, sf64_ru_radio_blob_19440, SF64_RU_RADIO_MAX_WORDS },
    { 0x5212, gMsg_ID_21010, sf64_ru_radio_blob_21010, SF64_RU_RADIO_MAX_WORDS },
    { 0x521C, gMsg_ID_21020, sf64_ru_radio_blob_21020, SF64_RU_RADIO_MAX_WORDS },
    { 0x5226, gMsg_ID_21030, sf64_ru_radio_blob_21030, SF64_RU_RADIO_MAX_WORDS },
    { 0x523A, gMsg_ID_21050, sf64_ru_radio_blob_21050, SF64_RU_RADIO_MAX_WORDS },
    { 0x5244, gMsg_ID_21060, sf64_ru_radio_blob_21060, SF64_RU_RADIO_MAX_WORDS },
    { 0x524E, gMsg_ID_21070, sf64_ru_radio_blob_21070, SF64_RU_RADIO_MAX_WORDS },
    { 0x524F, gMsg_ID_21071, sf64_ru_radio_blob_21071, SF64_RU_RADIO_MAX_WORDS },
    { 0x5250, gMsg_ID_21072, sf64_ru_radio_blob_21072, SF64_RU_RADIO_MAX_WORDS },
    { 0x5251, gMsg_ID_21073, sf64_ru_radio_blob_21073, SF64_RU_RADIO_MAX_WORDS },
    { 0x5258, gMsg_ID_21080, sf64_ru_radio_blob_21080, SF64_RU_RADIO_MAX_WORDS },
    { 0x5259, gMsg_ID_21081, sf64_ru_radio_blob_21081, SF64_RU_RADIO_MAX_WORDS },
    { 0x525A, gMsg_ID_21082, sf64_ru_radio_blob_21082, SF64_RU_RADIO_MAX_WORDS },
    { 0x525B, gMsg_ID_21083, sf64_ru_radio_blob_21083, SF64_RU_RADIO_MAX_WORDS },
    { 0x5262, gMsg_ID_21090, sf64_ru_radio_blob_21090, SF64_RU_RADIO_MAX_WORDS },
    { 0x5263, gMsg_ID_21091, sf64_ru_radio_blob_21091, SF64_RU_RADIO_MAX_WORDS },
    { 0x5264, gMsg_ID_21092, sf64_ru_radio_blob_21092, SF64_RU_RADIO_MAX_WORDS },
    { 0x5265, gMsg_ID_21093, sf64_ru_radio_blob_21093, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2A, gMsg_ID_20010, sf64_ru_radio_blob_20010, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2B, gMsg_ID_20011, sf64_ru_radio_blob_20011, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2C, gMsg_ID_20012, sf64_ru_radio_blob_20012, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2D, gMsg_ID_20013, sf64_ru_radio_blob_20013, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2E, gMsg_ID_20014, sf64_ru_radio_blob_20014, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E2F, gMsg_ID_20015, sf64_ru_radio_blob_20015, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E30, gMsg_ID_20016, sf64_ru_radio_blob_20016, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E31, gMsg_ID_20017, sf64_ru_radio_blob_20017, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E32, gMsg_ID_20018, sf64_ru_radio_blob_20018, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E33, gMsg_ID_20019, sf64_ru_radio_blob_20019, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E34, gMsg_ID_20020, sf64_ru_radio_blob_20020, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E3E, gMsg_ID_20030, sf64_ru_radio_blob_20030, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E48, gMsg_ID_20040, sf64_ru_radio_blob_20040, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E52, gMsg_ID_20050, sf64_ru_radio_blob_20050, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E5C, gMsg_ID_20060, sf64_ru_radio_blob_20060, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E66, gMsg_ID_20070, sf64_ru_radio_blob_20070, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E70, gMsg_ID_20080, sf64_ru_radio_blob_20080, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E74, gMsg_ID_20084, sf64_ru_radio_blob_20084, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E75, gMsg_ID_20085, sf64_ru_radio_blob_20085, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E7A, gMsg_ID_20090, sf64_ru_radio_blob_20090, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E7B, gMsg_ID_20091, sf64_ru_radio_blob_20091, SF64_RU_RADIO_MAX_WORDS },
    { 0x4E7C, gMsg_ID_20092, sf64_ru_radio_blob_20092, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EB6, gMsg_ID_20150, sf64_ru_radio_blob_20150, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EC0, gMsg_ID_20160, sf64_ru_radio_blob_20160, SF64_RU_RADIO_MAX_WORDS },
    { 0x4ECA, gMsg_ID_20170, sf64_ru_radio_blob_20170, SF64_RU_RADIO_MAX_WORDS },
    { 0x4ED4, gMsg_ID_20180, sf64_ru_radio_blob_20180, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EDE, gMsg_ID_20190, sf64_ru_radio_blob_20190, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EE8, gMsg_ID_20200, sf64_ru_radio_blob_20200, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EF2, gMsg_ID_20210, sf64_ru_radio_blob_20210, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EFC, gMsg_ID_20220, sf64_ru_radio_blob_20220, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EFD, gMsg_ID_20221, sf64_ru_radio_blob_20221, SF64_RU_RADIO_MAX_WORDS },
    { 0x4EFE, gMsg_ID_20222, sf64_ru_radio_blob_20222, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F06, gMsg_ID_20230, sf64_ru_radio_blob_20230, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F0B, gMsg_ID_20235, sf64_ru_radio_blob_20235, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F0C, gMsg_ID_20236, sf64_ru_radio_blob_20236, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F0D, gMsg_ID_20237, sf64_ru_radio_blob_20237, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F0E, gMsg_ID_20238, sf64_ru_radio_blob_20238, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F0F, gMsg_ID_20239, sf64_ru_radio_blob_20239, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F1A, gMsg_ID_20250, sf64_ru_radio_blob_20250, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F24, gMsg_ID_20260, sf64_ru_radio_blob_20260, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F25, gMsg_ID_20261, sf64_ru_radio_blob_20261, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F26, gMsg_ID_20262, sf64_ru_radio_blob_20262, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F27, gMsg_ID_20263, sf64_ru_radio_blob_20263, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F28, gMsg_ID_20264, sf64_ru_radio_blob_20264, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F29, gMsg_ID_20265, sf64_ru_radio_blob_20265, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2A, gMsg_ID_20266, sf64_ru_radio_blob_20266, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2B, gMsg_ID_20267, sf64_ru_radio_blob_20267, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2C, gMsg_ID_20268, sf64_ru_radio_blob_20268, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2D, gMsg_ID_20269, sf64_ru_radio_blob_20269, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2E, gMsg_ID_20270, sf64_ru_radio_blob_20270, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F2F, gMsg_ID_20271, sf64_ru_radio_blob_20271, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F30, gMsg_ID_20272, sf64_ru_radio_blob_20272, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F31, gMsg_ID_20273, sf64_ru_radio_blob_20273, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F32, gMsg_ID_20274, sf64_ru_radio_blob_20274, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F33, gMsg_ID_20275, sf64_ru_radio_blob_20275, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F34, gMsg_ID_20276, sf64_ru_radio_blob_20276, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F35, gMsg_ID_20277, sf64_ru_radio_blob_20277, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F36, gMsg_ID_20278, sf64_ru_radio_blob_20278, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F37, gMsg_ID_20279, sf64_ru_radio_blob_20279, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F38, gMsg_ID_20280, sf64_ru_radio_blob_20280, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F39, gMsg_ID_20281, sf64_ru_radio_blob_20281, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3A, gMsg_ID_20282, sf64_ru_radio_blob_20282, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3B, gMsg_ID_20283, sf64_ru_radio_blob_20283, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3C, gMsg_ID_20284, sf64_ru_radio_blob_20284, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3D, gMsg_ID_20285, sf64_ru_radio_blob_20285, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3E, gMsg_ID_20286, sf64_ru_radio_blob_20286, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F3F, gMsg_ID_20287, sf64_ru_radio_blob_20287, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F40, gMsg_ID_20288, sf64_ru_radio_blob_20288, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F41, gMsg_ID_20289, sf64_ru_radio_blob_20289, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F42, gMsg_ID_20290, sf64_ru_radio_blob_20290, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F43, gMsg_ID_20291, sf64_ru_radio_blob_20291, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F44, gMsg_ID_20292, sf64_ru_radio_blob_20292, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F46, gMsg_ID_20294, sf64_ru_radio_blob_20294, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F48, gMsg_ID_20296, sf64_ru_radio_blob_20296, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F49, gMsg_ID_20297, sf64_ru_radio_blob_20297, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4A, gMsg_ID_20298, sf64_ru_radio_blob_20298, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4B, gMsg_ID_20299, sf64_ru_radio_blob_20299, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4C, gMsg_ID_20300, sf64_ru_radio_blob_20300, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4D, gMsg_ID_20301, sf64_ru_radio_blob_20301, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4E, gMsg_ID_20302, sf64_ru_radio_blob_20302, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F4F, gMsg_ID_20303, sf64_ru_radio_blob_20303, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F50, gMsg_ID_20304, sf64_ru_radio_blob_20304, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F51, gMsg_ID_20305, sf64_ru_radio_blob_20305, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F52, gMsg_ID_20306, sf64_ru_radio_blob_20306, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F53, gMsg_ID_20307, sf64_ru_radio_blob_20307, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F54, gMsg_ID_20308, sf64_ru_radio_blob_20308, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F55, gMsg_ID_20309, sf64_ru_radio_blob_20309, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F56, gMsg_ID_20310, sf64_ru_radio_blob_20310, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F57, gMsg_ID_20311, sf64_ru_radio_blob_20311, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F58, gMsg_ID_20312, sf64_ru_radio_blob_20312, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F59, gMsg_ID_20313, sf64_ru_radio_blob_20313, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5A, gMsg_ID_20314, sf64_ru_radio_blob_20314, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5B, gMsg_ID_20315, sf64_ru_radio_blob_20315, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5C, gMsg_ID_20316, sf64_ru_radio_blob_20316, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5D, gMsg_ID_20317, sf64_ru_radio_blob_20317, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5E, gMsg_ID_20318, sf64_ru_radio_blob_20318, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F5F, gMsg_ID_20319, sf64_ru_radio_blob_20319, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F60, gMsg_ID_20320, sf64_ru_radio_blob_20320, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F61, gMsg_ID_20321, sf64_ru_radio_blob_20321, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F66, gMsg_ID_20326, sf64_ru_radio_blob_20326, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F67, gMsg_ID_20327, sf64_ru_radio_blob_20327, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F68, gMsg_ID_20328, sf64_ru_radio_blob_20328, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F69, gMsg_ID_20329, sf64_ru_radio_blob_20329, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F6A, gMsg_ID_20330, sf64_ru_radio_blob_20330, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F6B, gMsg_ID_20331, sf64_ru_radio_blob_20331, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F6C, gMsg_ID_20332, sf64_ru_radio_blob_20332, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F6D, gMsg_ID_20333, sf64_ru_radio_blob_20333, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F71, gMsg_ID_20337, sf64_ru_radio_blob_20337, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F72, gMsg_ID_20338, sf64_ru_radio_blob_20338, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F73, gMsg_ID_20339, sf64_ru_radio_blob_20339, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F74, gMsg_ID_20340, sf64_ru_radio_blob_20340, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F77, gMsg_ID_20343, sf64_ru_radio_blob_20343, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F78, gMsg_ID_20344, sf64_ru_radio_blob_20344, SF64_RU_RADIO_MAX_WORDS },
    { 0x4F79, gMsg_ID_20345, sf64_ru_radio_blob_20345, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F0, gMsg_ID_22000, sf64_ru_radio_blob_22000, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F1, gMsg_ID_22001, sf64_ru_radio_blob_22001, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F2, gMsg_ID_22002, sf64_ru_radio_blob_22002, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F3, gMsg_ID_22003, sf64_ru_radio_blob_22003, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F4, gMsg_ID_22004, sf64_ru_radio_blob_22004, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F5, gMsg_ID_22005, sf64_ru_radio_blob_22005, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F6, gMsg_ID_22006, sf64_ru_radio_blob_22006, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F7, gMsg_ID_22007, sf64_ru_radio_blob_22007, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F8, gMsg_ID_22008, sf64_ru_radio_blob_22008, SF64_RU_RADIO_MAX_WORDS },
    { 0x55F9, gMsg_ID_22009, sf64_ru_radio_blob_22009, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FA, gMsg_ID_22010, sf64_ru_radio_blob_22010, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FB, gMsg_ID_22011, sf64_ru_radio_blob_22011, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FC, gMsg_ID_22012, sf64_ru_radio_blob_22012, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FD, gMsg_ID_22013, sf64_ru_radio_blob_22013, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FE, gMsg_ID_22014, sf64_ru_radio_blob_22014, SF64_RU_RADIO_MAX_WORDS },
    { 0x55FF, gMsg_ID_22015, sf64_ru_radio_blob_22015, SF64_RU_RADIO_MAX_WORDS },
    { 0x5600, gMsg_ID_22016, sf64_ru_radio_blob_22016, SF64_RU_RADIO_MAX_WORDS },
    { 0x5601, gMsg_ID_22017, sf64_ru_radio_blob_22017, SF64_RU_RADIO_MAX_WORDS },
    { 0x5602, gMsg_ID_22018, sf64_ru_radio_blob_22018, SF64_RU_RADIO_MAX_WORDS },
    { 0x5603, gMsg_ID_22019, sf64_ru_radio_blob_22019, SF64_RU_RADIO_MAX_WORDS },
    { 0x5604, gMsg_ID_22020, sf64_ru_radio_blob_22020, SF64_RU_RADIO_MAX_WORDS },
};
#define SF64_RU_RADIO_EXT_TABLE_COUNT 779


static void sf64_ru_copy_from_blob(volatile u16* dst, volatile u16* blob, int max_words) {
    int i;
    int len;
    if (blob[0] != SF64_RU_BLOB_MAGIC_A) return;
    if (blob[1] != SF64_RU_BLOB_MAGIC_B) return;
    if (blob[7] != SF64_RU_BLOB_TAIL) return;
    if (blob[5] != 1) return;
    len = (int)blob[6];
    if (len <= 0) return;
    if (len > max_words) len = max_words;
    for (i = 0; i < len; i++) {
        dst[i] = blob[SF64_RU_BLOB_HEADER_WORDS + i];
        if (dst[i] == 0) return;
    }
    dst[max_words - 1] = 0;
}

static void sf64_ru_apply_slot_translations_once(void) {
    int i;
    if (sf64_ru_slot_applied) return;
    for (i = 0; i < SF64_RU_RUNTIME_TABLE_COUNT; i++) {
        sf64_ru_copy_from_blob(sf64_ru_runtime_table[i].dst, sf64_ru_runtime_table[i].blob, sf64_ru_runtime_table[i].max_words);
    }
    sf64_ru_slot_applied = 1;
}


static int sf64_ru_blob_enabled(volatile u16* blob) {
    if (blob[0] != SF64_RU_RADIO_MAGIC_A) return 0;
    if (blob[1] != SF64_RU_RADIO_MAGIC_B) return 0;
    if (blob[7] != SF64_RU_BLOB_TAIL) return 0;
    if (blob[5] != 1) return 0;
    if (blob[6] <= 0) return 0;
    if (blob[6] > SF64_RU_RADIO_MAX_WORDS) return 0;
    return 1;
}

static u16* sf64_ru_radio_text_for_msg(u16* msgPtr) {
    int i;
    for (i = 0; i < SF64_RU_RADIO_EXT_TABLE_COUNT; i++) {
        if ((u16*)sf64_ru_radio_ext_table[i].original == msgPtr) {
            volatile u16* blob = sf64_ru_radio_ext_table[i].blob;
            if (sf64_ru_blob_enabled(blob)) {
                return (u16*)&blob[SF64_RU_BLOB_HEADER_WORDS];
            }
            return msgPtr;
        }
    }
    return msgPtr;
}

static int sf64_ru_should_shift_radio_text(u16* msgPtr) {
    if (msgPtr == (u16*)0) return 0;
    /* Shift the active radio message itself, including slot-copy rows. */
    if (gRadioMsg == msgPtr) return 1;
    /* Shift Russian Language Mod radio_overflow rows when Message_DisplayText receives the
       original pointer and redirects to an external RU radio blob. */
    if (sf64_ru_radio_text_for_msg(msgPtr) != msgPtr) return 1;
    return 0;
}

static s32 sf64_ru_msg_token_at_is_visible_tail(u16* msgPtr, s32 idx) {
    u16 tok;
    if (idx < 0) return 0;
    tok = msgPtr[idx];
    if (tok == MSGCHAR_END) return 0;
    if (tok == MSGCHAR_NXT) return 0;
    return 1;
}

static s32 sf64_ru_original_msg_has_nxt(u16* msgPtr) {
    s32 i;
    if (msgPtr == (u16*)0) return 0;
    /* Keep this bounded. Original radio slots are small, and we only need to
       know whether this message is a multi-box radio entry. */
    for (i = 0; i < 64; i++) {
        if (msgPtr[i] == MSGCHAR_NXT) return 1;
        if (msgPtr[i] == MSGCHAR_END) return 0;
    }
    return 0;
}

RECOMP_HOOK("Radio_Draw") void sf64_ru_radio_draw_before_guard(void) {
    sf64_ru_radio_before_state = gRadioState;
    sf64_ru_radio_before_char_index = gRadioMsgCharIndex;
    sf64_ru_radio_before_msg = gRadioMsg;
    sf64_ru_radio_before_ru_msg = sf64_ru_radio_text_for_msg(gRadioMsg);
    sf64_ru_radio_before_active = (sf64_ru_radio_before_ru_msg != gRadioMsg);
}

RECOMP_HOOK_RETURN("Radio_Draw") void sf64_ru_radio_draw_after_guard(void) {
    s32 before;
    s32 after;
    u16 next_ru;

    if (!sf64_ru_radio_before_active) return;
    if (sf64_ru_radio_before_msg != gRadioMsg) return;
    if (sf64_ru_radio_before_ru_msg != sf64_ru_radio_text_for_msg(gRadioMsg)) return;
    if (sf64_ru_radio_before_state != 4) return;

    before = sf64_ru_radio_before_char_index;
    after = gRadioMsgCharIndex;
    if (before < 0) return;

    next_ru = sf64_ru_radio_before_ru_msg[before + 1];

    /* If the RU overflow text itself wants a next box at this position, keep
       the original radio state machine behavior and do not force the tail. */
    if (next_ru == MSGCHAR_NXT) {
        if (Audio_GetCurrentVoiceStatus() == 0) {
            gRadioState = 31;
        }
        gRadioMsgCharIndex = before;
        return;
    }

    if (next_ru == MSGCHAR_END) {
        /* If the original US message was a multi-box radio entry, the original
           Radio_Draw would normally see MSGCHAR_NXT and advance gRadioMsgList.
           With overflow RU text we intentionally delay that until the RU blob
           has fully printed. Without this, long first boxes can display fine
           but the following original radio boxes never start. */
        if (sf64_ru_original_msg_has_nxt(sf64_ru_radio_before_msg) && (Audio_GetCurrentVoiceStatus() == 0)) {
            gRadioState = 31;
        }
        return;
    }

    /* Correct the two clipping cases:
       1) the original buffer saw MSGCHAR_NXT early and changed state to 31;
       2) the original US Radio_Draw hard cap stopped at index 60 while the
          Russian Language Mod radio blob still has a few valid words left. */
    if (after <= before) {
        if (sf64_ru_msg_token_at_is_visible_tail(sf64_ru_radio_before_ru_msg, before + 1)) {
            gRadioState = 4;
            gRadioMsgCharIndex = before + 1;
        }
        return;
    }

    if ((after >= 60) && sf64_ru_msg_token_at_is_visible_tail(sf64_ru_radio_before_ru_msg, after + 1)) {
        gRadioState = 4;
        gRadioMsgCharIndex = after + 1;
    }
}

RECOMP_PATCH void Message_DisplayChar(Gfx** gfxPtr, u16 msgChar, s32 xpos, s32 ypos) {
    if ((msgChar >= RU_BASE) && (msgChar <= RU_LAST)) {
        s32 idx = (s32)(msgChar - RU_BASE);
        Lib_TextureRect_CI4(gfxPtr, (u8*)sf64_ru_glyph_ci4[idx], sf64_ru_white_tlut, 16, 13, (f32)xpos, (f32)ypos, 1.0f, 1.0f);
        return;
    }
    if (msgChar <= 95) {
        u16* pal = &gTextCharPalettes[(msgChar & 3) * 16];
        Lib_TextureRect_CI4(gfxPtr, gTextCharTextures[msgChar >> 2], pal, 16, 13, (f32)xpos, (f32)ypos, 1.0f, 1.0f);
    }
}

RECOMP_PATCH s32 Message_GetWidth(u16* msgPtr) {
    s32 width = 0;
    u16* msgChar = sf64_ru_radio_text_for_msg(msgPtr);
    while (*msgChar != MSGCHAR_END) {
        if ((*msgChar >= MSGCHAR_CLF) || (*msgChar == MSGCHAR_SPC)) {
            width++;
        }
        msgChar++;
    }
    return width;
}

RECOMP_PATCH s32 Message_GetCharCount(u16* msgPtr) {
    s32 count = 0;
    u16* msgChar = sf64_ru_radio_text_for_msg(msgPtr);
    while (*msgChar != MSGCHAR_END) {
        count++;
        msgChar++;
    }
    return count;
}

RECOMP_PATCH s32 Message_DisplayText(Gfx** gfxPtr, u16* msgPtr, s32 xPos, s32 yPos, s32 len) {
    s32 xChar;
    s32 yChar;
    s32 i;
    s32 print = 0;
    u16* originalMsgPtr = msgPtr;
    if (sf64_ru_should_shift_radio_text(originalMsgPtr)) {
        xPos += SF64_RU_RADIO_TEXT_X_SHIFT_PX;
    }
    xChar = xPos;
    yChar = yPos;
    sf64_ru_gfx_set_prim_color(gfxPtr, 255, 255, 255, 255);
    msgPtr = sf64_ru_radio_text_for_msg(msgPtr);
    for (i = 0; msgPtr[i] != MSGCHAR_END && i < len; i++) {
        print = 0;
        switch (msgPtr[i]) {
            case MSGCHAR_NWL:
                xChar = xPos;
                yChar += 13;
                break;
            case MSGCHAR_CLF:
            case MSGCHAR_CUP:
            case MSGCHAR_CRT:
            case MSGCHAR_CDN:
                sf64_ru_gfx_set_prim_color(gfxPtr, 255, 255, 0, 255);
                Message_DisplayChar(gfxPtr, msgPtr[i], xChar, yChar);
                xChar += 14;
                print = 1;
                sf64_ru_gfx_set_prim_color(gfxPtr, 255, 255, 255, 255);
                break;
            case MSGCHAR_NP2:
            case MSGCHAR_NP3:
            case MSGCHAR_NP4:
            case MSGCHAR_NP5:
            case MSGCHAR_NP6:
            case MSGCHAR_NP7:
            case MSGCHAR_PRI0:
            case MSGCHAR_PRI1:
            case MSGCHAR_PRI2:
            case MSGCHAR_PRI3:
            case MSGCHAR_QSP:
            case MSGCHAR_HSP:
            case MSGCHAR_NXT:
                break;
            case MSGCHAR_SPC:
            default:
                Message_DisplayChar(gfxPtr, msgPtr[i], xChar, yChar);
                xChar += 7;
                print = 1;
                break;
        }
    }
    return print;
}

RECOMP_PATCH s32 Message_IsPrintingChar(u16* msgPtr, s32 charPos) {
    s32 i;
    s32 print = 0;
    msgPtr = sf64_ru_radio_text_for_msg(msgPtr);
    for (i = 0; msgPtr[i] != MSGCHAR_END && i < charPos; i++) {
        print = 0;
        switch (msgPtr[i]) {
            case MSGCHAR_NWL:
            case MSGCHAR_NP2:
            case MSGCHAR_NP3:
            case MSGCHAR_NP4:
            case MSGCHAR_NP5:
            case MSGCHAR_NP6:
            case MSGCHAR_NP7:
            case MSGCHAR_PRI0:
            case MSGCHAR_PRI1:
            case MSGCHAR_PRI2:
            case MSGCHAR_PRI3:
            case MSGCHAR_SPC:
            case MSGCHAR_QSP:
            case MSGCHAR_HSP:
            case MSGCHAR_NXT:
                break;
            default:
                print = 1;
                break;
        }
    }
    return print;
}

RECOMP_HOOK_RETURN("Display_Update") void sf64_ru_after_display_update(void) { sf64_ru_apply_slot_translations_once(); }
RECOMP_HOOK("Message_DisplayText") void sf64_ru_before_message_display_text(Gfx** gfxPtr, u16* msgPtr, s32 xPos, s32 yPos, s32 len) { sf64_ru_apply_slot_translations_once(); }
RECOMP_HOOK("Message_DisplayScrollingText") void sf64_ru_before_message_display_scrolling_text(Gfx** gfxPtr, u16* msgPtr, s32 xPos, s32 yPos, s32 yRangeHi, s32 yRangeLo, s32 len) { sf64_ru_apply_slot_translations_once(); }
RECOMP_HOOK("Radio_PlayMessage") void sf64_ru_before_radio_play_message(u16* msg, s32 radioCharacterId) { sf64_ru_apply_slot_translations_once(); }
