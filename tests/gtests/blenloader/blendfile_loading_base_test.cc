/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 by Blender Foundation.
 */
#include "blendfile_loading_base_test.h"

extern "C" {
#include "BKE_blender.h"
#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_main.h"
#include "BKE_modifier.h"
#include "BKE_node.h"
#include "BKE_scene.h"

#include "BLI_threads.h"
#include "BLI_path_util.h"

#include "BLO_readfile.h"

#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph.h"

#include "DNA_genfile.h" /* for DNA_sdna_current_init() */
#include "DNA_windowmanager_types.h"

#include "IMB_imbuf.h"

#include "MEM_guardedalloc.h"

#include "RNA_define.h"

#include "WM_api.h"
}

DEFINE_string(test_assets_dir, "", "lib/tests directory from SVN containing the test assets.");

BlendfileLoadingBaseTest::~BlendfileLoadingBaseTest()
{
}

void BlendfileLoadingBaseTest::SetUpTestCase()
{
  testing::Test::SetUpTestCase();

  /* Minimal code to make loading a blendfile and constructing a depsgraph not crash, copied from
   * main() in creator.c. */
  BLI_threadapi_init();

  DNA_sdna_current_init();
  BKE_blender_globals_init();
  IMB_init();
  BKE_images_init();
  BKE_modifier_init();
  DEG_register_node_types();
  RNA_init();
  init_nodesystem();

  G.background = true;
  G.factory_startup = true;

  /* Allocate a dummy window manager. The real window manager will try and load Python scripts from
   * the release directory, which it won't be able ot find. */
  G.main->wm.first = MEM_callocN(sizeof(wmWindowManager), __func__);
}

void BlendfileLoadingBaseTest::TearDown()
{
  depsgraph_free();
  blendfile_free();

  testing::Test::TearDown();
}

bool BlendfileLoadingBaseTest::blendfile_load(const char *filepath)
{
  if (FLAGS_test_assets_dir.empty()) {
    ADD_FAILURE()
        << "Pass the flag --test-assets-dir and point to the lib/tests directory from SVN.";
    return false;
  }

  char abspath[FILENAME_MAX];
  BLI_path_join(abspath, sizeof(abspath), FLAGS_test_assets_dir.c_str(), filepath, NULL);

  bfile = BLO_read_from_file(abspath, BLO_READ_SKIP_NONE, NULL /* reports */);
  if (bfile == nullptr) {
    ADD_FAILURE() << "Unable to load file '" << filepath << "' from test assets dir '"
                  << FLAGS_test_assets_dir << "'";
    return false;
  }
  return true;
}

void BlendfileLoadingBaseTest::blendfile_free()
{
  if (bfile == nullptr) {
    return;
  }
  BLO_blendfiledata_free(bfile);
  bfile = nullptr;
}

void BlendfileLoadingBaseTest::depsgraph_create(eEvaluationMode depsgraph_evaluation_mode)
{
  depsgraph = DEG_graph_new(
      bfile->main, bfile->curscene, bfile->cur_view_layer, depsgraph_evaluation_mode);
  DEG_graph_build_from_view_layer(depsgraph, bfile->main, bfile->curscene, bfile->cur_view_layer);
  BKE_scene_graph_update_tagged(depsgraph, bfile->main);
}

void BlendfileLoadingBaseTest::depsgraph_free()
{
  if (depsgraph == nullptr) {
    return;
  }
  DEG_graph_free(depsgraph);
  depsgraph = nullptr;
}
