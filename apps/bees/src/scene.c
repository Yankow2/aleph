/* scene.c 
   bees
   aleph

   scene management module
   includes operator network, DSP patch
*/

// std
#include <string.h>
// asf
#include "delay.h"

// aleph-common
#include "module_common.h"

//avr32
#include "app.h"
#include "bfin.h"
#include "font.h"
#include "print_funcs.h"
#include "simple_string.h"

/// bees
#include "app_bees.h"
#include "files.h"
//#include "flash.h"
#include "flash_bees.h"
#include "global.h"
#include "memory.h"
#include "pages.h"
#include "net_protected.h"
#include "preset.h"
#include "render.h"
#include "scene.h"
#include "types.h"

//-----------------------------
// ---- extern data

// RAM buffer for scene data
sceneData_t* sceneData;

//----------------------------------------------
//----- extern functions

void scene_init(void) {
  u32 i;
  sceneData = (sceneData_t*)alloc_mem( sizeof(sceneData_t) );
  for(i=0; i<SCENE_NAME_LEN; i++) {
    (sceneData->desc.sceneName)[i] = ' ';
  }
  for(i=0; i<MODULE_NAME_LEN; i++) {
    (sceneData->desc.moduleName)[i] = ' ';
  }
  strcpy(sceneData->desc.sceneName, ""); 
}

void scene_deinit(void) {
}

// fill global RAM buffer with current state of system
void scene_write_buf(void) {
  u8* dst = (u8*)(sceneData->pickle);
  char test[SCENE_NAME_LEN] = "                ";
  u32 bytes = 0;
  u8* newDst = NULL;
    int i;

  ///// print paramameters
  //  u32 i;

  print_dbg("\r\n writing scene data... ");

  /*  for(i=0; i<net->numParams; i++) {
      print_dbg("\r\n param ");
      print_dbg_ulong(i);
      print_dbg(" : ");
      print_dbg(net->params[i].desc.label);
      print_dbg(" ; val ");
      print_dbg_hex((u32)net->params[i].data.value.asInt);
      }
  */

  // write name
  for(i=0; i<SCENE_NAME_LEN; i++) {
    *dst = sceneData->desc.sceneName[i];
    dst++;
    bytes++;
  }

  // write bees version
  *dst = sceneData->desc.beesVersion.min;
  dst++; bytes++;
  *dst = sceneData->desc.beesVersion.maj;
  dst++; bytes++;
  dst = pickle_16(sceneData->desc.beesVersion.rev, dst);
  bytes += 2;

  print_dbg("\r\n scene_write buf; module name: ");
  print_dbg(sceneData->desc.moduleName);

  // write module name
  for(i=0; i<MODULE_NAME_LEN; i++) {
    *dst = (sceneData->desc.moduleName)[i];
    dst++;
    bytes++;
  }

  

  //// TEST
for(i=0; i<MODULE_NAME_LEN; i++) {
  test[i] = *(dst - MODULE_NAME_LEN + i);
 } 
  print_dbg("; test buffer after write: ");
  print_dbg(test);
  ////

  // write module version
  *dst = sceneData->desc.moduleVersion.min;
  dst++;
  *dst = sceneData->desc.moduleVersion.maj;
  dst++;
  dst = pickle_16(sceneData->desc.moduleVersion.rev, dst);
  bytes += 4;
  
  // pickle network
  newDst = net_pickle(dst);
  bytes += (newDst - dst);
  print_dbg("\r\n pickled network, bytes written: 0x");
  print_dbg_hex(bytes);
  dst = newDst;

  // pickle presets
  newDst = presets_pickle(dst);
  bytes += (newDst - dst);
  print_dbg("\r\n pickled presets, bytes written: 0x");
  print_dbg_hex(bytes);
  dst = newDst;

#if RELEASEBUILD==1
#else
  if(bytes > SCENE_PICKLE_SIZE - 0x800) {
    print_dbg(" !!!!!!!! warning: serialized scene data approaching allocated bounds !!!!! ");
  }
  if(bytes > SCENE_PICKLE_SIZE) {
    print_dbg(" !!!!!!!! error: serialized scene data exceeded allocated bounds !!!!! ");
  }
#endif
}

// set current state of system from global RAM buffer
void scene_read_buf(void) {
  /// pointer to serial blob
  const u8* src = (u8*)&(sceneData->pickle);
  int i;
  //// TEST
  volatile char moduleName[32];
  ModuleVersion moduleVersion;
  ////
   app_pause();

  // store current mod name in scene desc
   //  memcpy(modName, sceneData->desc.moduleName, MODULE_NAME_LEN);
   
   // read scene name
  for(i=0; i<SCENE_NAME_LEN; i++) {
    sceneData->desc.sceneName[i] = *src;
    src++;
  }

   // read bees version
  sceneData->desc.beesVersion.min = *src;
  src++;
  sceneData->desc.beesVersion.maj = *src;
  src++;
  src = unpickle_16(src, &(sceneData->desc.beesVersion.rev));

 // read module name
  for(i=0; i<SCENE_NAME_LEN; i++) {
    sceneData->desc.moduleName[i] = *src;
    src++;
  }

  print_dbg("\r\n unpickled module name: ");
  print_dbg(sceneData->desc.moduleName);

   // read module version
  sceneData->desc.moduleVersion.min = *src;
  src++;
  sceneData->desc.moduleVersion.maj = *src;
  src++;
  src = unpickle_16(src, &(sceneData->desc.moduleVersion.rev));

  print_dbg("\r\n unpickled module version: ");
  print_dbg_ulong(sceneData->desc.moduleVersion.maj);
  print_dbg(".");
  print_dbg_ulong(sceneData->desc.moduleVersion.min);
  print_dbg(".");
  print_dbg_ulong(sceneData->desc.moduleVersion.rev);


  ///// load the DSP now!
  print_dbg("\r\n loading module from card, filename: ");
  print_dbg(sceneData->desc.moduleName);
  files_load_dsp_name(sceneData->desc.moduleName);

  print_dbg("\r\n waiting for DSP init...");
  bfin_wait_ready();

#if RELEASEBUILD==1
#else
  
  // query module name / version
  //// FIXME: currently nothing happens with this.
  print_dbg("\r\n querying module name...");
  bfin_get_module_name(moduleName);
  print_dbg("\r\n querying module version...");
  bfin_get_module_version(&moduleVersion);

  print_dbg("\r\n received module name: ");
  print_dbg((char*)moduleName);

  print_dbg("\r\n received module version: ");
  print_dbg_ulong(moduleVersion.maj);
  print_dbg(".");
  print_dbg_ulong(moduleVersion.min);
  print_dbg(".");
  print_dbg_ulong(moduleVersion.rev);

#endif

  app_pause();

  print_dbg("\r\n clearing operator list...");
  net_clear_user_ops();

  print_dbg("\r\n reporting DSP parameters...");
  net_report_params();

  /// FIXME:
  /// check the module version and warn if different!
  // there could also be a check here for mismatched parameter list.

  // unpickle network 
  print_dbg("\r\n unpickling network for scene recall...");
  src = net_unpickle(src);
    
  // unpickle presets
  print_dbg("\r\n unpickling presets for scene recall...");
  src = presets_unpickle(src);
  
  print_dbg("\r\n copied stored network and presets to RAM ");

  /* for(i=0; i<net->numParams; i++) { */
  /*   print_dbg("\r\n param "); */
  /*   print_dbg_ulong(i); */
  /*   print_dbg(" : "); */
  /*   print_dbg(net->params[i].desc.label); */
  /*   print_dbg(" ; val "); */
  /*   print_dbg_hex((u32)net->params[i].data.value); */
  /* } */

  bfin_wait_ready();
  // update bfin parameters
  net_send_params();
  print_dbg("\r\n sent new parameter values");

  delay_ms(5);

  // enable audio processing
  bfin_enable();
  
  app_resume();
}

// write current state as default
void scene_write_default(void) {
#if 0
  s8 neq = 0;
  s8 modName[MODULE_NAME_LEN];
#endif

  app_pause();
  render_boot("writing scene to flash");

  print_dbg("\r\n writing scene to flash... ");
  print_dbg("module name: ");
  print_dbg(sceneData->desc.moduleName);

  flash_write_scene();

# if 0 // not storing .ldr in flash for the moment!
  // write default LDR if changed 
  neq = strncmp((const char*)modName, (const char*)sceneData->desc.moduleName, MODULE_NAME_LEN);
  if(neq) {
    render_boot("writing DSP to flash");
    print_dbg("\r\n writing default LDR from scene descriptor");
    files_store_default_dsp_name(sceneData->desc.moduleName);
  } 
#endif    
  delay_ms(20);
  print_dbg("\r\n finished writing default scene");
  app_resume();
}

// load from default
void scene_read_default(void) {
  app_pause();
  print_dbg("\r\n reading default scene from flash... ");
  flash_read_scene();
  
  print_dbg("\r\n finished reading ");  
  app_resume();
}

// set scene name
void scene_set_name(const char* name) {
  strncpy(sceneData->desc.sceneName, name, SCENE_NAME_LEN);
}

// set module name
void scene_set_module_name(const char* name) {
  strncpy(sceneData->desc.moduleName, name, MODULE_NAME_LEN);
}

// query module name and version
void scene_query_module(void) {
  volatile char * moduleName = sceneData->desc.moduleName;
  ModuleVersion * moduleVersion = &(sceneData->desc.moduleVersion);

  /// sets module name/version in scene data to reported name/version

  print_dbg("\r\n querying module name...");
  bfin_get_module_name(moduleName);
  print_dbg("\r\n querying module version...");
  bfin_get_module_version(moduleVersion);

  strcat((char*)moduleName, ".ldr");

  print_dbg("\r\n received module name: ");
  print_dbg((char*)moduleName);


  print_dbg("\r\n received module version: ");
  print_dbg_ulong(moduleVersion->maj);
  print_dbg(".");
  print_dbg_ulong(moduleVersion->min);
  print_dbg(".");
  print_dbg_ulong(moduleVersion->rev);

}
