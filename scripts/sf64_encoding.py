#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""SF64 RU encoder/validator for confirmed 0x0100+ Cyrillic message route."""
from __future__ import annotations
RU_UPPER = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
RU_LOWER = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
MSG_END=0x0000; MSG_NWL=0x0001; MSG_PRI2=0x000A; MSG_SPC=0x000C; MSG_QSP=0x000D; MSG_HSP=0x000E; MSG_NXT=0x000F
RU_BASE=0x0100; RU_LAST=0x0141
CHAR_TO_CODE: dict[str,int] = {}
for i,ch in enumerate(RU_UPPER): CHAR_TO_CODE[ch]=RU_BASE+i
for i,ch in enumerate(RU_LOWER): CHAR_TO_CODE[ch]=0x0121+i
for i,ch in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ"): CHAR_TO_CODE[ch]=0x0018+i
for i,ch in enumerate("abcdefghijklmnopqrstuvwxyz"): CHAR_TO_CODE[ch]=0x0032+i
for i,ch in enumerate("0123456789"): CHAR_TO_CODE[ch]=0x0051+i
CHAR_TO_CODE.update({" ":MSG_SPC,"!":0x004C,"?":0x004D,"-":0x004E,",":0x004F,".":0x0050,"'":0x005B,"(":0x005C,")":0x005D,":":0x005E})
TOKEN_TO_CODE={"[NL]":MSG_NWL,"[NEWLINE]":MSG_NWL,"[PRI2]":MSG_PRI2,"[SPACE]":MSG_SPC,"[QSP]":MSG_QSP,"[HSP]":MSG_HSP,"[NEXT_BOX]":MSG_NXT,"[NXT]":MSG_NXT,"<NXT>":MSG_NXT}
NORMALIZE_CHARS={"’":"'","‘":"'","—":"-","–":"-","…":"...","«":"\"","»":"\""}

def encode_text(text: str, append_end: bool=True) -> tuple[list[int], list[str], list[str]]:
    codes=[]; errors=[]; warnings=[]; i=0
    while i < len(text):
        ch=text[i]
        if ch=='\r': i+=1; continue
        if ch=='\n': codes.append(MSG_NWL); i+=1; continue
        if ch in '[<':
            close=']' if ch=='[' else '>'
            j=text.find(close, i+1)
            if j!=-1:
                token=text[i:j+1]
                if token in TOKEN_TO_CODE:
                    codes.append(TOKEN_TO_CODE[token]); i=j+1; continue
        normalized=NORMALIZE_CHARS.get(ch,ch)
        if normalized != ch:
            if normalized == '"':
                errors.append(f"unsupported normalized char {normalized!r} from {ch!r} at pos {i}; use apostrophe or remove quotes")
                i+=1; continue
            for c in normalized:
                if c in CHAR_TO_CODE: codes.append(CHAR_TO_CODE[c])
                else: errors.append(f"unsupported normalized char {c!r} from {ch!r} at pos {i}")
            i+=1; continue
        if ch in CHAR_TO_CODE: codes.append(CHAR_TO_CODE[ch])
        else: errors.append(f"unsupported char {ch!r} U+{ord(ch):04X} at pos {i}")
        i+=1
    if append_end: codes.append(MSG_END)
    max_line=line_len=0
    for c in codes:
        if c in (MSG_END,MSG_NXT,MSG_NWL): max_line=max(max_line,line_len); line_len=0
        else: line_len+=1
    max_line=max(max_line,line_len)
    if max_line > 28: warnings.append(f"long visual line {max_line} chars; consider manual [NL] or [NXT]")
    return codes, errors, warnings

def codes_to_hex(codes:list[int])->str: return ' '.join(f'{c:04X}' for c in codes)

def analyse_text(text:str, slot_words:int|None=None)->dict[str,object]:
    codes, errors, warnings = encode_text(text)
    n=len(codes); overflow=0
    if errors: status='FAIL_UNSUPPORTED'; storage='none'
    elif slot_words is not None and n > slot_words:
        status='READY_EXTERNAL_CANDIDATE'; storage='external_ptr'; overflow=n-slot_words
        warnings.append('does not fit original slot; external pointer route requires route-specific testing')
    else: status='READY_SLOT'; storage='slot_copy'
    return {'status':status,'storage_mode':storage,'codes':codes,'encoded_u16_hex':codes_to_hex(codes),'encoded_word_count':n,'slot_capacity_words':slot_words,'overflow_words':overflow,'errors':errors,'warnings':warnings}

if __name__=='__main__':
    import argparse,json
    ap=argparse.ArgumentParser(); ap.add_argument('text'); ap.add_argument('--slot-words',type=int,default=None)
    args=ap.parse_args(); print(json.dumps(analyse_text(args.text,args.slot_words),ensure_ascii=False,indent=2))
