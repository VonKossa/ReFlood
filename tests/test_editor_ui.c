#define main editor_entry_point
#include "../src/editor_main.c"
#undef main
#include <assert.h>

int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: test_editor_ui EXTRACTED_DATA_DIR\n");return 2;}
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);
    assert(SDL_Init(SDL_INIT_VIDEO)==0);
    Editor e;memset(&e,0,sizeof(e));e.zoom=6;e.data=argv[1];e.output="custom";
    editor_level_new(&e.level,1);e.dirty=true;
    e.window=SDL_CreateWindow("editor test",0,0,WIDTH,HEIGHT,0);
    assert(e.window);
    e.renderer=SDL_CreateRenderer(e.window,-1,SDL_RENDERER_SOFTWARE);
    assert(e.renderer&&load_graphics(&e));
    char help[112];
    for(unsigned tile=0;tile<256u;tile++){
        editor_tile_description(tile,e.attributes[tile],e.attributes_valid,help,sizeof(help));
        assert(help[0]);
    }
    editor_tile_description(22,0,false,help,sizeof(help));
    assert(!strcmp(help,"Player start position"));
    editor_tile_description(23,0,false,help,sizeof(help));
    assert(!strcmp(help,"Camera bounds"));
    editor_tile_description(255,0,false,help,sizeof(help));
    assert(strstr(help,"Trigger tile"));
    editor_tile_description(156,0,true,help,sizeof(help));
    assert(strstr(help,"166"));
    editor_tile_description(166,0,true,help,sizeof(help));
    assert(strstr(help,"156"));
    uint8_t bank_b_attributes[256];
    char bank_b_path[1024];
    snprintf(bank_b_path,sizeof(bank_b_path),"%s/blocks/BLOCKB_attrs.bin",e.data);
    assert(file_exact(bank_b_path,bank_b_attributes,sizeof(bank_b_attributes)));
    assert(e.attributes_valid);
    editor_tile_description(71,e.attributes[71],true,help,sizeof(help));
    assert(!strstr(help,"fatal"));
    editor_tile_description(71,bank_b_attributes[71],true,help,sizeof(help));
    assert(strstr(help,"fatal"));
    editor_tile_description(40,e.attributes[40],true,help,sizeof(help));
    char bank_a_terrain[112];snprintf(bank_a_terrain,sizeof(bank_a_terrain),"%s",help);
    editor_tile_description(40,bank_b_attributes[40],true,help,sizeof(help));
    assert(strcmp(bank_a_terrain,help));
    assert(key(&e,SDLK_t,KMOD_NONE));
    assert(e.dialog==DIALOG_TILES&&e.tile_help_offset==0);
    assert(key(&e,SDLK_END,KMOD_NONE));
    assert(e.tile_help_offset==256u-TILE_HELP_ROWS);
    assert(key(&e,SDLK_DOWN,KMOD_NONE));
    assert(e.tile_help_offset==256u-TILE_HELP_ROWS);
    assert(key(&e,SDLK_HOME,KMOD_NONE));
    assert(e.tile_help_offset==0);
    assert(key(&e,SDLK_PAGEDOWN,KMOD_NONE));
    assert(e.tile_help_offset==TILE_HELP_ROWS);
    assert(key(&e,SDLK_PAGEUP,KMOD_NONE));
    assert(e.tile_help_offset==0);
    render(&e);
    assert(key(&e,SDLK_t,KMOD_NONE)&&e.dialog==DIALOG_NONE);
    assert(e.dirty);
    assert(key(&e,SDLK_q,KMOD_CTRL));
    assert(e.dialog==DIALOG_QUIT);
    assert(key(&e,SDLK_n,KMOD_NONE));
    assert(e.dialog==DIALOG_NONE);
    assert(key(&e,SDLK_l,KMOD_CTRL));
    assert(e.dialog==DIALOG_LOAD_WARNING);
    assert(key(&e,SDLK_y,KMOD_NONE));
    assert(e.dialog==DIALOG_BROWSER);
    bool found=false;
    for(unsigned i=0;i<e.browser.count;i++)
        if(!strcmp(e.browser.entries[i].name,"level_05_header.bin")){
            e.browser.selected=i;found=true;break;
        }
    assert(found);
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(e.dialog==DIALOG_NONE&&e.level.level==5&&!e.dirty);
    for(unsigned i=0;i<8;i++)assert(key(&e,SDLK_MINUS,KMOD_NONE));
    assert(e.zoom==0&&tile_size(&e)==4u&&e.scroll_x==0&&e.scroll_y==0);
    assert(key(&e,SDLK_LEFT,KMOD_NONE));
    assert(key(&e,SDLK_RIGHT,KMOD_NONE));
    assert(e.scroll_x==0);
    paint(&e,PANEL_X+15*PALETTE_STEP+12,PALETTE_TOP+15*PALETTE_STEP+12,false);
    assert(e.selected==255u);
    paint(&e,MAP_LEFT+127*4+1,MAP_TOP+99*4+1,false);
    assert(e.level.map[99u*EDITOR_W+127u]==255u);
    for(unsigned i=0;i<20;i++)assert(key(&e,SDLK_PLUS,KMOD_NONE));
    assert(e.zoom==ZOOM_STEPS-1u&&tile_size(&e)==64u);
    assert(key(&e,SDLK_g,KMOD_CTRL)&&e.grid);
    assert(key(&e,SDLK_g,KMOD_CTRL)&&!e.grid);
    e.zoom=0;e.scroll_x=e.scroll_y=0;
    SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
    SDL_Rect map_clip={MAP_LEFT,MAP_TOP,MAP_WIDTH,MAP_BOTTOM-MAP_TOP};
    SDL_RenderSetClipRect(e.renderer,&map_clip);
    for(int col=0;col<12;col++)
        draw_tile(&e,6,MAP_LEFT+col*4,MAP_TOP,4);
    draw_grid(&e,4);
    SDL_RenderSetClipRect(e.renderer,NULL);
    for(int line=10;line<=11;line++){
        Uint32 vertical=0,horizontal=0;
        SDL_Rect sample_v={MAP_LEFT+line*4,MAP_TOP+2,1,1};
        SDL_Rect sample_h={MAP_LEFT+2,MAP_TOP+line*4,1,1};
        assert(SDL_RenderReadPixels(e.renderer,&sample_v,SDL_PIXELFORMAT_ARGB8888,&vertical,4)==0);
        assert(SDL_RenderReadPixels(e.renderer,&sample_h,SDL_PIXELFORMAT_ARGB8888,&horizontal,4)==0);
        assert(vertical==0xff2addebu&&horizontal==0xff2addebu);
    }
    const int scaled_sizes[][2]={{1920,1080},{1366,768}};
    for(unsigned display=0;display<2u;display++){
        SDL_SetWindowSize(e.window,scaled_sizes[display][0],scaled_sizes[display][1]);
        SDL_RenderSetLogicalSize(e.renderer,WIDTH,HEIGHT);
        SDL_Rect viewport;
        float sx,sy;
        SDL_RenderGetViewport(e.renderer,&viewport);
        SDL_RenderGetScale(e.renderer,&sx,&sy);
        SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
        draw_fullscreen_grid(&e,4);
        for(int line=10;line<=11;line++){
            Uint32 vertical=0,horizontal=0;
            SDL_Rect sample_v={screen_pixel(MAP_LEFT+line*4,viewport.x,sx),
                screen_pixel(MAP_TOP+2,viewport.y,sy),1,1};
            SDL_Rect sample_h={screen_pixel(MAP_LEFT+2,viewport.x,sx),
                screen_pixel(MAP_TOP+line*4,viewport.y,sy),1,1};
            assert(SDL_RenderReadPixels(e.renderer,&sample_v,SDL_PIXELFORMAT_ARGB8888,&vertical,4)==0);
            assert(SDL_RenderReadPixels(e.renderer,&sample_h,SDL_PIXELFORMAT_ARGB8888,&horizontal,4)==0);
            assert(vertical==0xff2addebu&&horizontal==0xff2addebu);
        }
        SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
        label(&e,100,100,"B: PALETTE BANK");
        const int first=screen_pixel(100,viewport.x,sx);
        const int glyph_y=screen_pixel(100,viewport.y,sy);
        int longest=0;
        for(int row=glyph_y-2;row<=glyph_y+2;row++){
            int run=0;
            for(int glyph_x=first-2;glyph_x<first+12;glyph_x++){
                Uint32 ink=0;
                SDL_Rect sample={glyph_x,row,1,1};
                assert(SDL_RenderReadPixels(e.renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&ink,4)==0);
                if(ink==0xfff0e8d2u){
                    run++;
                    if(run>longest)longest=run;
                }else run=0;
            }
        }
        assert(longest>=(int)(6.0f*sx));
        e.fullscreen=true;e.label_count=0;
        SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
        label(&e,100,100,"B: PALETTE BANK");
        draw_fullscreen_labels(&e);
        const int physical_x=screen_pixel(100,viewport.x,sx);
        const int physical_y=screen_pixel(100,viewport.y,sy);
        unsigned font_scale=(unsigned)(sx<sy?sx:sy);
        if(!font_scale)font_scale=1u;
        for(int glyph_x=physical_x;glyph_x<physical_x+(int)(6u*font_scale);glyph_x++){
            Uint32 ink=0;
            SDL_Rect sample={glyph_x,physical_y,1,1};
            assert(SDL_RenderReadPixels(e.renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&ink,4)==0);
            assert(ink==0xfff0e8d2u);
        }
        e.fullscreen=false;
    }
    SDL_SetWindowSize(e.window,WIDTH,HEIGHT);
    SDL_RenderSetLogicalSize(e.renderer,WIDTH,HEIGHT);
    SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
    label(&e,10,10,":");
    Uint32 pixel=0;
    SDL_Rect sample={13,12,1,1};
    assert(SDL_RenderReadPixels(e.renderer,&sample,SDL_PIXELFORMAT_ARGB8888,&pixel,4)==0);
    assert(pixel==0xfff0e8d2u);
    assert(key(&e,SDLK_f,KMOD_CTRL));
    assert(e.fullscreen);
    SDL_Rect fullscreen_viewport;
    float fullscreen_sx,fullscreen_sy;
    SDL_RenderGetViewport(e.renderer,&fullscreen_viewport);
    SDL_RenderGetScale(e.renderer,&fullscreen_sx,&fullscreen_sy);
    SDL_SetRenderDrawColor(e.renderer,0,0,0,255);SDL_RenderClear(e.renderer);
    draw_fullscreen_grid(&e,4);
    for(int line=10;line<=11;line++){
        Uint32 vertical=0;
        SDL_Rect sample_v={screen_pixel(MAP_LEFT+line*4,fullscreen_viewport.x,fullscreen_sx),
            screen_pixel(MAP_TOP+2,fullscreen_viewport.y,fullscreen_sy),1,1};
        assert(SDL_RenderReadPixels(e.renderer,&sample_v,SDL_PIXELFORMAT_ARGB8888,&vertical,4)==0);
        assert(vertical==0xff2addebu);
    }
    assert(key(&e,SDLK_f,KMOD_CTRL));
    assert(!e.fullscreen);
    const char *save_parts[]={"header","tilemap","trigger_payload","triggers"};
    for(unsigned i=0;i<4u;i++){
        char path[256];
        snprintf(path,sizeof(path),"build-check/editor-ui-output/levels/My_Cave_%s.bin",save_parts[i]);
        remove(path);
    }
    e.output="build-check/editor-ui-output";
    assert(key(&e,SDLK_s,KMOD_CTRL));
    assert(e.dialog==DIALOG_SAVE&&e.save_name_focus);
    assert(!strcmp(e.browser.directory,"build-check/editor-ui-output/levels"));
    assert(key(&e,SDLK_TAB,KMOD_NONE));
    assert(!e.save_name_focus);
    assert(key(&e,SDLK_BACKSPACE,KMOD_NONE));
    assert(!strcmp(e.browser.directory,"build-check/editor-ui-output"));
    bool levels_found=false;
    for(unsigned i=0;i<e.browser.count;i++)
        if(!strcmp(e.browser.entries[i].name,"levels")){
            e.browser.selected=i;levels_found=true;break;
        }
    assert(levels_found);
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(!strcmp(e.browser.directory,"build-check/editor-ui-output/levels"));
    assert(key(&e,SDLK_TAB,KMOD_NONE));
    assert(key(&e,SDLK_a,KMOD_CTRL));
    save_text(&e,"My_Cave");
    assert(!strcmp(e.save_name,"My_Cave"));
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(e.dialog==DIALOG_NONE&&!e.dirty);
    assert(editor_level_named_exists(e.save_directory,"My_Cave"));
    assert(key(&e,SDLK_s,KMOD_CTRL));
    assert(e.dialog==DIALOG_SAVE);
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(e.dialog==DIALOG_OVERWRITE);
    assert(key(&e,SDLK_n,KMOD_NONE));
    assert(e.dialog==DIALOG_SAVE);
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(e.dialog==DIALOG_OVERWRITE);
    assert(key(&e,SDLK_y,KMOD_NONE));
    assert(e.dialog==DIALOG_NONE);
    assert(key(&e,SDLK_l,KMOD_CTRL));
    assert(e.dialog==DIALOG_BROWSER);
    bool named_found=false;
    for(unsigned i=0;i<e.browser.count;i++)
        if(!strcmp(e.browser.entries[i].name,"My_Cave_header.bin")){
            e.browser.selected=i;named_found=true;break;
        }
    assert(named_found);
    assert(key(&e,SDLK_RETURN,KMOD_NONE));
    assert(e.dialog==DIALOG_NONE&&!strcmp(e.save_name,"My_Cave"));
    assert(key(&e,SDLK_q,KMOD_CTRL));
    assert(!key(&e,SDLK_y,KMOD_NONE));
    SDL_DestroyTexture(e.font_texture);SDL_DestroyTexture(e.tiles);
    SDL_DestroyRenderer(e.renderer);SDL_DestroyWindow(e.window);
    SDL_Quit();puts("editor shortcuts, confirmation, and header load passed");return 0;
}
