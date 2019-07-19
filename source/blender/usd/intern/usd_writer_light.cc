#include "usd_writer_light.h"
#include "usd_hierarchy_iterator.h"

#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/sphereLight.h>

extern "C" {
#include "BLI_assert.h"
#include "BLI_utildefines.h"

#include "DNA_light_types.h"
#include "DNA_object_types.h"
}

USDLightWriter::USDLightWriter(const USDExporterContext &ctx) : USDAbstractWriter(ctx)
{
}

bool USDLightWriter::is_supported(const Object *object) const
{
  Light *light = static_cast<Light *>(object->data);
  return ELEM(light->type, LA_AREA, LA_LOCAL, LA_SUN);
}

void USDLightWriter::do_write(HierarchyContext &context)
{
  pxr::UsdTimeCode timecode = get_export_time_code();

  Light *light = static_cast<Light *>(context.object->data);
  pxr::UsdLuxLight usd_light;

  switch (light->type) {
    case LA_AREA:
      switch (light->area_shape) {
        case LA_AREA_DISK:
        case LA_AREA_ELLIPSE: { /* An ellipse light will deteriorate into a disk light. */
          pxr::UsdLuxDiskLight disk_light = pxr::UsdLuxDiskLight::Define(stage, usd_path_);
          disk_light.CreateRadiusAttr().Set(light->area_size, timecode);
          usd_light = disk_light;
          break;
        }
        case LA_AREA_RECT: {
          pxr::UsdLuxRectLight rect_light = pxr::UsdLuxRectLight::Define(stage, usd_path_);
          rect_light.CreateWidthAttr().Set(light->area_size, timecode);
          rect_light.CreateHeightAttr().Set(light->area_sizey, timecode);
          usd_light = rect_light;
          break;
        }
        case LA_AREA_SQUARE: {
          pxr::UsdLuxRectLight rect_light = pxr::UsdLuxRectLight::Define(stage, usd_path_);
          rect_light.CreateWidthAttr().Set(light->area_size, timecode);
          rect_light.CreateHeightAttr().Set(light->area_size, timecode);
          usd_light = rect_light;
          break;
        }
      }
      break;
    case LA_LOCAL: {
      pxr::UsdLuxSphereLight sphere_light = pxr::UsdLuxSphereLight::Define(stage, usd_path_);
      sphere_light.CreateRadiusAttr().Set(light->area_size, timecode);
      usd_light = sphere_light;
      break;
    }
    case LA_SUN:
      usd_light = pxr::UsdLuxDistantLight::Define(stage, usd_path_);
      break;
    default:
      BLI_assert(!"is_supported() returned true for unsupported light type");
  }

  /* Scale factor to get to somewhat-similar illumination. Since the Hydra viewport had similar
   * over-exposure as Blender Internal with the same values, this code applies the reverse of the
   * versioning code in light_emission_unify(). */
  float usd_intensity;
  if (light->type == LA_SUN) {
    /* Untested, as the Hydra GL viewport doesn't support distant lights. */
    usd_intensity = light->energy;
  }
  else {
    usd_intensity = light->energy / 100.f;
  }
  usd_light.CreateIntensityAttr().Set(usd_intensity, timecode);

  usd_light.CreateColorAttr().Set(pxr::GfVec3f(light->r, light->g, light->b), timecode);
  usd_light.CreateSpecularAttr().Set(light->spec_fac, timecode);
}
