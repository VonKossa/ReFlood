#include "flood_game.h"
#include <stdio.h>
#include <string.h>

#define PAL_AUDIO_CLOCK 3546895u
#define PAL_CIA_E_CLOCK 709379u
#define CIA_AUDIO_PERIOD 0x1a00u

static const uint16_t route_control[64]={
0x0000,0x0081,0x0081,0x0081,0x0081,0x0081,0x0082,0x0082,
0x0082,0x00a4,0x00a8,0x0082,0x0082,0x0084,0x0088,0x00a4,
0x0081,0x0082,0x0082,0x0082,0x0082,0x0084,0x0084,0x0082,
0x0082,0x0082,0x0084,0x0088,0x0082,0x0081,0x0082,0x0082,
0x0082,0x0084,0x0084,0x0082,0x0082,0x0082,0x0082,0x0084,
0x0088,0x0084,0x0088,0x0081,0x0081,0x0081,0x0081,0x0084,
0x0088,0x0084,0x0084,0x0081,0x0081,0x0082,0x0081,0x0084,
0x0081,0x0082,0x0082,0x0088,0x0081,0x0082,0x0084,0x0088};
static const uint8_t route_next[64]={
0,0,0,0,0,0,0,0,0,10,0,0,0,14,0,0,0,0,0,0,0,0,0,0,0,0,27,0,0,30,0,0,
0,0,0,0,0,0,0,40,0,42,0,0,0,0,0,48,0,0,0,0,53,0,0,0,57,0,59,0,61,62,63,0};
static const uint8_t delay_table[12]={2,3,4,6,8,12,16,24,32,48,64,96};
static const uint16_t period_table[104]={
0x6acc,0x64cc,0x5f25,0x59ce,0x54c3,0x5003,0x4b86,0x4747,0x4346,0x3f8b,0x3bf3,0x3892,
0x3568,0x3269,0x2f93,0x2cea,0x2a66,0x2801,0x2566,0x23a5,0x21af,0x1fc4,0x1dfe,0x1c4e,
0x1abc,0x1936,0x17cc,0x1676,0x1533,0x1401,0x12e4,0x11d5,0x10d4,0x0fe3,0x0efe,0x0e26,
0x0d5b,0x0c9b,0x0be5,0x0b3b,0x0a9b,0x0a02,0x0972,0x08e9,0x0869,0x07f1,0x077f,0x0713,
0x06ad,0x064d,0x05f2,0x059d,0x054d,0x0500,0x04b8,0x0475,0x0435,0x03f8,0x03bf,0x038a,
0x0356,0x0326,0x02f9,0x02cf,0x02a6,0x0280,0x025c,0x023a,0x021a,0x01fc,0x01e0,0x01c5,
0x01ab,0x0193,0x017d,0x0167,0x0153,0x0140,0x012e,0x011d,0x010d,0x00fe,0x00f0,0x00e2,
0x00d6,0x00ca,0x00be,0x00b4,0x00aa,0x00a0,0x0097,0x008f,0x0087,0x007f,0x0078,0x0070,
0x0060,0x0050,0x0040,0x0030,0x0020,0x0010,0,0};

static uint16_t be16(const uint8_t *p){return (uint16_t)((p[0]<<8)|p[1]);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static int read_file(const char *path,uint8_t *out,size_t size){
    FILE *f=fopen(path,"rb");size_t got;if(!f)return 0;
    got=fread(out,1,size,f);fclose(f);return got==size;
}

static void envelope_start(FloodAudioEnvelope *e,const uint8_t *data,unsigned mode){
    memset(e,0,sizeof(*e));e->flags=1u;
    if(mode==0u){e->flags|=(uint8_t)(data[0]&0x80u);e->loop_adjust=(uint8_t)(data[0]&0x7fu);}
    else if(mode==2u)e->flags|=0x80u;
    e->data=data+1;
}

static void envelope_restart(FloodAudioEnvelope *e){
    /* $1733E-$17380: every note, including note zero, restarts both of the
       channel's current envelopes without changing their macro pointers or
       loop modes.  Leaving an envelope at its completed zero value made later
       notes—and eventually whole presentation-music channels—inaudible. */
    e->base=0u;e->value=0u;e->phase=0u;e->ticks=0u;e->repeats=0u;
    e->flags=(uint8_t)((e->flags|1u)&(uint8_t)~4u);
}

static uint16_t envelope_step(FloodAudioEnvelope *e,uint16_t input){
    int32_t result;
    if(!(e->flags&4u)){
        const uint8_t *part=e->data+e->phase;
        if(e->flags&1u){e->flags&=(uint8_t)~1u;e->value=(uint16_t)(e->value+(int8_t)part[1]);}
        if(++e->ticks==part[2]){
            e->ticks=0;
            if(++e->repeats==part[0]){
                e->repeats=0;e->phase=(uint8_t)(e->phase+3u);
                if(e->phase==12u){
                    if(e->flags&0x80u){e->base=(uint16_t)(e->base+(int8_t)e->loop_adjust);e->phase=3u;e->flags|=1u;}
                    else e->flags|=4u;
                }else e->flags|=1u;
            }else e->flags|=1u;
        }
    }
    result=(int32_t)input+(int16_t)e->value-(int16_t)e->base;
    return result<0?0u:(uint16_t)result;
}

static void sample_start(FloodAudio *a,FloodAudioChannel *c){
    const size_t desc=4u+(size_t)c->sample_id*16u;
    c->sample_playing=false;c->dma_state=0u;
    if(desc+16u>a->instrument_size){c->sample_length=0;return;}
    c->sample_start=4u+be32(a->instruments+desc);
    c->sample_length=(size_t)be16(a->instruments+desc+12u)*2u;
    if(c->sample_start+c->sample_length>a->instrument_size)c->sample_length=0;
    c->sample_phase=0;c->sample_loop=false;
    if(c->sample_length)c->dma_state=1u;
}

static void sample_to_loop(FloodAudio *a,FloodAudioChannel *c){
    const size_t desc=4u+(size_t)c->sample_id*16u;
    if(desc+16u>a->instrument_size){c->sample_length=0;return;}
    c->sample_start=4u+be32(a->instruments+desc+4u);
    c->sample_length=(size_t)be16(a->instruments+desc+14u)*2u;
    if(c->sample_start+c->sample_length>a->instrument_size)c->sample_length=0;
    c->sample_phase=0;c->sample_loop=true;
}

static void start_track(FloodAudio *a,unsigned channel,uint8_t id,uint16_t control){
    FloodAudioChannel *c=&a->channel[channel];const size_t entry=a->track_table+(size_t)id*2u;
    if(entry+2u>a->track_size)return;
    memset(c,0,sizeof(*c));c->track_start=a->song_base+be16(a->tracks+entry);
    c->track_pos=c->track_start;c->loop_track=(control&0x20u)!=0u;
    if(c->track_pos>=a->track_size)return;
    c->sample_id=0;c->volume=63;c->active=true;
}

void flood_audio_trigger(FloodAudio *a,uint8_t id){
    if(!a->loaded||a->music_mode||id>=64u)return;
    do{
        const uint16_t control=route_control[id];
        for(unsigned ch=0;ch<4u;ch++)if(control&(1u<<ch))start_track(a,ch,id,control);
        id=route_next[id];
    }while(id);
}

static bool advance_sequence(FloodAudio *a,FloodAudioChannel *c){
    for(unsigned guard=0;guard<2u;guard++){
        if(c->sequence_pos>=a->track_size)return false;
        const uint8_t id=a->tracks[c->sequence_pos++];
        if(id==0xffu){
            if(!c->loop_sequence)return false;
            c->sequence_pos=c->sequence_start;
            continue;
        }
        const size_t entry=a->track_table+(size_t)id*2u;
        if(entry+2u>a->track_size)return false;
        c->track_pos=a->song_base+be16(a->tracks+entry);
        return c->track_pos<a->track_size;
    }
    return false;
}

static void parse_track(FloodAudio *a,FloodAudioChannel *c){
    for(unsigned guard=0;guard<128u&&c->active;guard++){
        if(c->track_pos>=a->track_size){c->active=false;c->volume=0;return;}
        const uint8_t command=a->tracks[c->track_pos++];
        if(command==0xffu){
            if(a->music_mode&&advance_sequence(a,c))continue;
            /* Original effect-control bit 5 is copied to channel flag $20 at
               $16DF2.  At a track terminator, $172A0-$172EC uses that flag
               to restore the effect's initial track pointer and continue. */
            if(!a->music_mode&&c->loop_track){c->track_pos=c->track_start;continue;}
            c->active=false;c->volume=0;return;
        }
        if(command<0x80u){
            /* $17328 only applies transpose to non-zero note bytes.  $20 in
               the channel is the stable note period; pitch envelopes produce
               a separate output period from that base on every interrupt. */
            const int note=(int)command+(command?(int8_t)c->transpose:0);
            c->base_period=(note>=0&&note<(int)(sizeof(period_table)/sizeof(period_table[0])))?period_table[note]:0u;
            c->period=c->base_period;
            envelope_restart(&c->volume_env);envelope_restart(&c->pitch_env);
            sample_start(a,c);return;
        }
        if(command<0xa0u){
            const unsigned n=command&15u;
            if(n<sizeof(delay_table)){c->delay_reset=delay_table[n];c->delay=c->delay_reset;}
        }else if(command<0xc0u)c->sample_id=(uint8_t)(command&31u);
        else if(command<0xe0u){
            const size_t p=a->volume_macros+(size_t)(command&31u)*13u;
            if(p+13u<=a->track_size)envelope_start(&c->volume_env,a->tracks+p,0u);
        }else{
            const unsigned handler=command&7u;
            if(handler==0u){if(c->track_pos<a->track_size)c->transpose=a->tracks[c->track_pos++];}
            else if(handler==1u||handler==2u){
                if(c->track_pos<a->track_size){
                    const uint8_t n=a->tracks[c->track_pos++];const size_t p=a->pitch_macros+(size_t)n*13u;
                    if(p+13u<=a->track_size)envelope_start(&c->pitch_env,a->tracks+p,handler);
                }
            }
        }
    }
}

void flood_audio_tick(FloodAudio *a){
    if(!a->loaded)return;
    const bool tempo=(--a->speed_counter==0u);
    for(unsigned n=0;n<4u;n++){
        FloodAudioChannel *c=&a->channel[n];if(!c->active)continue;
        /* $170E2-$1712E: a new note spends one interrupt in state 1
           (DMA disabled/programmed), starts in state 2 on the next interrupt,
           then leaves the hardware running with its loop registers installed. */
        if(c->dma_state==1u){c->dma_state=2u;c->sample_playing=true;}
        else if(c->dma_state==2u)c->dma_state=0u;
        if(tempo&&c->delay)c->delay--;
        /* $1727E restores the selected delay before parsing the next note.
           Without this, only the first note has its intended duration and
           the track then races through one note per 50 Hz interrupt. */
        if(!c->delay){c->delay=c->delay_reset;parse_track(a,c);}
        if(!c->active)continue;
        uint16_t volume=envelope_step(&c->volume_env,0u);if(volume>63u)volume=63u;c->volume=(uint8_t)volume;
        c->period=envelope_step(&c->pitch_env,c->base_period);
        c->sample_step=c->period?(((uint64_t)PAL_AUDIO_CLOCK<<32)/(c->period*a->sample_rate)):0u;
    }
    if(tempo)a->speed_counter=a->speed;
}

static bool setup_song(FloodAudio *a,uint32_t rate,char *err,size_t errcap){
    if(a->track_size<2u){snprintf(err,errcap,"short tracker file");return false;}
    a->song_base=be16(a->tracks);
    if(a->song_base+16u>a->track_size){snprintf(err,errcap,"bad song header");return false;}
    const uint8_t *song=a->tracks+a->song_base;
    a->speed=(uint8_t)be16(song);if(!a->speed)a->speed=1u;
    a->volume_macros=a->song_base+be16(song+2);
    a->pitch_macros=a->song_base+be16(song+4);
    a->track_table=a->song_base+16u;
    if(a->volume_macros>=a->track_size||a->pitch_macros>=a->track_size){snprintf(err,errcap,"bad macro table");return false;}
    a->speed_counter=a->speed;a->sample_rate=rate?rate:44100u;a->loaded=true;
    return true;
}

bool flood_audio_load(FloodAudio *a,const char *data_dir,uint32_t rate,char *err,size_t errcap){
    char path[512];memset(a,0,sizeof(*a));
    snprintf(path,sizeof(path),"%s/audio/FLDFX_DU.bin",data_dir);
    if(!read_file(path,a->tracks,FLOOD_FX_TRACK_BYTES)){snprintf(err,errcap,"cannot read %s",path);return false;}
    snprintf(path,sizeof(path),"%s/audio/FLDFX_IN.bin",data_dir);
    if(!read_file(path,a->instruments,FLOOD_FX_INSTRUMENT_BYTES)){snprintf(err,errcap,"cannot read %s",path);return false;}
    a->track_size=FLOOD_FX_TRACK_BYTES;a->instrument_size=FLOOD_FX_INSTRUMENT_BYTES;
    return setup_song(a,rate,err,errcap);
}

bool flood_music_load(FloodAudio *a,const char *data_dir,uint32_t rate,char *err,size_t errcap){
    char path[512];memset(a,0,sizeof(*a));
    snprintf(path,sizeof(path),"%s/audio/TRACK_DA.bin",data_dir);
    if(!read_file(path,a->tracks,FLOOD_MUSIC_TRACK_BYTES)){snprintf(err,errcap,"cannot read %s",path);return false;}
    snprintf(path,sizeof(path),"%s/audio/INSTR_DA.bin",data_dir);
    if(!read_file(path,a->instruments,FLOOD_MUSIC_INSTRUMENT_BYTES)){snprintf(err,errcap,"cannot read %s",path);return false;}
    a->track_size=FLOOD_MUSIC_TRACK_BYTES;a->instrument_size=FLOOD_MUSIC_INSTRUMENT_BYTES;a->music_mode=true;
    if(!setup_song(a,rate,err,errcap))return false;
    const uint8_t *song=a->tracks+a->song_base;
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++){
        FloodAudioChannel *c=&a->channel[n];
        c->sequence_start=a->song_base+be16(song+6u+n*2u);
        c->sequence_pos=c->sequence_start;c->loop_sequence=true;c->volume=63u;c->active=true;
        if(c->sequence_start>=a->track_size||!advance_sequence(a,c)){
            snprintf(err,errcap,"bad music sequence %u",n);memset(a,0,sizeof(*a));return false;
        }
    }
    return true;
}

void flood_audio_mix(FloodAudio *a,int16_t *stereo,size_t frames){
    memset(stereo,0,frames*2u*sizeof(*stereo));if(!a->loaded)return;
    for(size_t frame=0;frame<frames;frame++){
        /* $16C9A starts CIA-B timer A after writing only its high latch byte
           $19.  The reset low latch remains $FF, so the inclusive period is
           $19FF+1=$1A00 E-clock ticks: 709379/6656 = 106.577 Hz on PAL. */
        a->tick_accumulator+=PAL_CIA_E_CLOCK;
        if(a->tick_accumulator>=(uint64_t)a->sample_rate*CIA_AUDIO_PERIOD){
            a->tick_accumulator-=(uint64_t)a->sample_rate*CIA_AUDIO_PERIOD;
            flood_audio_tick(a);
        }
        int left=0,right=0;
        for(unsigned n=0;n<4u;n++){
            FloodAudioChannel *c=&a->channel[n];
            if(c->active&&c->sample_playing&&c->volume&&c->sample_length&&c->sample_step){
                size_t pos=(size_t)(c->sample_phase>>32);
                if(pos>=c->sample_length){
                    const uint64_t excess=c->sample_phase-((uint64_t)c->sample_length<<32);
                    sample_to_loop(a,c);
                    if(c->sample_length)c->sample_phase=excess%((uint64_t)c->sample_length<<32);
                    pos=(size_t)(c->sample_phase>>32);
                }
                if(c->sample_start+pos<a->instrument_size){
                    const int value=(int8_t)a->instruments[c->sample_start+pos]*(int)c->volume*2;
                    if(n==0u||n==3u)left+=value;else right+=value;
                }
                c->sample_phase+=c->sample_step;
            }
        }
        if(left>32767)left=32767;
        if(left<-32768)left=-32768;
        if(right>32767)right=32767;
        if(right<-32768)right=-32768;
        stereo[frame*2u]=(int16_t)left;stereo[frame*2u+1u]=(int16_t)right;
    }
}

void flood_audio_stop(FloodAudio *a){
    a->loaded=false;
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++){
        a->channel[n].active=false;a->channel[n].volume=0u;
    }
}

bool flood_audio_is_playing(const FloodAudio *a){
    if(!a||!a->loaded)return false;
    for(unsigned n=0;n<FLOOD_AUDIO_CHANNELS;n++)
        if(a->channel[n].active)return true;
    return false;
}
