#include "reflood_editor_tile_help.h"
#include <stdio.h>
#include <string.h>

static void add_feature(char *features,size_t capacity,const char *feature){
    size_t used=strlen(features);
    if(used<capacity)snprintf(features+used,capacity-used,"%s%s",
        used?", ":"",feature);
}

void editor_tile_description(unsigned tile,uint8_t attributes,bool have_attributes,
    char *text,size_t capacity){
    const char *known=NULL;
    switch(tile){
    case 0:known="Empty space";break;
    case 1:known="Spawns Doctor Dusty";break;
    case 2:known="Spawns Snail crawler";break;
    case 3:known="Trigger switch - graphic hidden at level start";break;
    case 4:case 6:case 10:
        known="Cleared at level start - no object spawned";break;
    case 7:case 9:case 15:
        known="Cleared at level start - no object spawned";break;
    case 5:known="Spawns Beady Ball";break;
    case 8:known="Spawns Vacuous Gombo";break;
    case 11:known="Mechanism marker - tile 203 (unused in originals)";break;
    case 12:known="Spawns Lumpy Wanderer";break;
    case 13:known="Starts runtime explosion state";break;
    case 14:known="Spawns Plonkin Donkin";break;
    case 16:known="Horizontal mechanism - center tile 201";break;
    case 17:known="Vertical mechanism - center tile 205";break;
    case 18:known="Flood-limited mechanism - center tile 203";break;
    case 19:known="Spawns Mine - becomes tile 230";break;
    case 20:known="Spawns bolt launcher";break;
    case 21:known="Water source";break;
    case 22:known="Player start position";break;
    case 23:known="Camera bounds";break;
    case 24:known="Spawns Bulbous Headed Vong";break;
    case 25:known="Spawns Psycho Teddy";break;
    case 29:known="Animated tile - cycles through 029-032";break;
    case 30:case 31:known="Frame of 029-032 - static if placed";break;
    case 32:known="Reverse animation - cycles 032-029";break;
    case 36:case 37:case 38:case 39:case 148:
        known="Collectible trash; needed for exit";break;
    case 71:known=(attributes&64u)&&have_attributes?
        "Animated 071/072 - fatal contact in this bank":
        "Animated tile - alternates 071/072";break;
    case 72:known=(attributes&64u)&&have_attributes?
        "Frame of 071 - static and fatal in this bank":
        "Frame of 071 - static if placed";break;
    case 136:known="Level exit after all trash is collected";break;
    case 200:known="Horizontal mechanism growth segment";break;
    case 201:known="Horizontal mechanism center graphic";break;
    case 202:known="Vertical mechanism growth segment";break;
    case 203:known="Shared mechanism center graphic";break;
    case 204:known="Mechanism growth segment (states 11/18)";break;
    case 205:known="Vertical mechanism center graphic";break;
    case 219:known="Consumed on contact - no other effect";break;
    case 220:known="Starts controllable bounce effect";break;
    case 221:known="Grenade weapon pickup";break;
    case 222:known="Boomerang weapon pickup";break;
    case 223:known="Pause moving mechanisms";break;
    case 224:known="Resume moving mechanisms";break;
    case 225:known="Extra life pickup";break;
    case 226:known="Cocktail - temporary life and air protection";break;
    case 227:known="Orange Can - briefly suspends objects";break;
    case 228:known="Speed up water";break;
    case 229:known="Pause water temporarily";break;
    case 231:known="Parachute pickup";break;
    case 232:known="Balloon pickup";break;
    case 234:known="Radial flame weapon pickup";break;
    case 235:known="Shuriken weapon pickup";break;
    case 236:known="Dynamite weapon pickup";break;
    case 237:known="Flame beam weapon pickup";break;
    case 230:known="Mine graphic - marker 019 creates active Mine";break;
    case 238:known="Score tile (+20) and zap effect - stays";break;
    case 255:known="Trigger tile (requires trigger record)";break;
    default:break;
    }
    if(tile>=156u&&tile<=175u){
        snprintf(text,capacity,"Transport to matching tile %03u",
            tile<=165u?tile+10u:tile-10u);
        return;
    }
    if(known){snprintf(text,capacity,"%s",known);return;}
    if(!have_attributes){snprintf(text,capacity,"Terrain - bank attributes unavailable");return;}
    char features[120]={0},other[32];
    if(attributes&2u)add_feature(features,sizeof(features),"solid");
    if(attributes&1u)add_feature(features,sizeof(features),"blocks water");
    if(attributes&4u)add_feature(features,sizeof(features),"slope");
    if(attributes&0x30u)add_feature(features,sizeof(features),"slope correction");
    if(attributes&64u)add_feature(features,sizeof(features),"fatal contact");
    if(attributes&0x80u)add_feature(features,sizeof(features),"downward support check");
    if(attributes&0x08u){
        snprintf(other,sizeof(other),"other flag 0x%02X",attributes&0x08u);
        add_feature(features,sizeof(features),other);
    }
    snprintf(text,capacity,"Terrain - %s",features[0]?features:"no flagged physics");
}
