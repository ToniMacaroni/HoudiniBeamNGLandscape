#include "MainLib.h"

#include <GA/GA_Handle.h>
#include <GU/GU_Detail.h>
#include <OP/OP_AutoLockInputs.h>
#include <OP/OP_Operator.h>
#include <OP/OP_OperatorTable.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>

#include <GEO/GEO_IOTranslator.h>
#include <UT/UT_Vector3.h>

#include <CH/CH_Manager.h>
#include <GEO/GEO_Detail.h>
#include <GEO/GEO_PrimPoly.h>
#include <GU/GU_PrimVolume.h>

using namespace HDK_BeamNGLandscapeExporter;

void
newSopOperator(OP_OperatorTable* table)
{
    table->addOperator(new OP_Operator(
        "beamng_landscape_exporter",
        "BeamNG Landscape Exporter",
        SOP_BeamNGLandscapeExporter::myConstructor,
        SOP_BeamNGLandscapeExporter::g_myTemplateList,
        1,
        1
    ));
}

static fpreal
remap(fpreal value, fpreal low1, fpreal high1, fpreal low2, fpreal high2)
{
    return low2 + (value - low1) * (high2 - low2) / (high1 - low1);
}

static uint16_t
fit(fpreal val)
{
    val = remap(val, 0, 200, 0, 65535);
    if (val < 0) val = 0;
    if (val > 65535) val = 65535;
    return static_cast<uint16_t>(std::round(val));
}

static void
getRange(GU_PrimVolume* volume, fpreal& min, fpreal& max)
{
    min = std::numeric_limits<fpreal>::max();
    max = std::numeric_limits<fpreal>::min();
    int resX, resY, resZ;
    volume->getRes(resX, resY, resZ);

    for (int y = 0; y < resY; ++y)
    {
        for (int x = 0; x < resX; ++x)
        {
            fpreal value = volume->getValueAtIndex(x, y, 0);
            if (value < min) min = value;
            if (value > max) max = value;
        }
    }
}

static int
exportGeoData(void* data, int index, fpreal t, const PRM_Template* tplate)
{
    auto* sop = static_cast<SOP_BeamNGLandscapeExporter*>(data);

    UT_String filePath;
    sop->evalString(filePath, "file", 0, t);

    bool shouldRemap = sop->evalInt("remap", 0, t);

    fpreal inRangeMin = sop->evalFloat("inRange", 0, t);
    fpreal inRangeMax = sop->evalFloat("inRange", 1, t);

    if (filePath == "") return 0;

    std::string filename = filePath.toStdString();
    filename = filename.substr(filename.find_last_of('/') + 1);
    filename = filename.substr(0, filename.find_last_of('.'));

    OP_Context      context(0);
    UT_Debug        dbg("HoudiniBeamNGLandscapeExporter");
    GU_DetailHandle gdHandle = sop->getCookedGeoHandle(context);

    if (!gdHandle.isValid()) return 0;

    const GU_Detail* gdp = gdHandle.gdp();

    const GA_PrimitiveGroup* group = nullptr;
    const GA_Primitive*      prim = nullptr;
    const GEO_PrimVolume*    height_volume = nullptr;

    GA_ROHandleS name_attrib(gdp->findStringTuple(GA_ATTRIB_PRIMITIVE, "name"));

    if (!name_attrib.isValid())
    {
        printf("No name attribute found");
        return 1;
    }

    GA_FOR_ALL_OPT_GROUP_PRIMITIVES(gdp, group, prim)
    {
        if (prim->getTypeId() == GEO_PRIMVOLUME)
        {
            if (name_attrib.get(prim->getMapOffset()) == "height") height_volume = (GEO_PrimVolume*)prim;
        }
    }

    std::ofstream file(filePath, std::ios::out | std::ios::binary);

    char     version = 9;
    uint32_t size = 2048;

    file.write(&version, sizeof(version));
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    dbg.output("Writing heightmap with in range %f - %f", inRangeMin, inRangeMax);

    for (int y = 0; y < 2048; ++y)
    {
        for (int x = 0; x < 2048; ++x)
        {
            fpreal val = height_volume->getValueAtIndex(x, y, 0);
            if(shouldRemap)
            {
                val = remap(val, inRangeMin, inRangeMax, 0, 65535);
            }
            if (val < 0) val = 0;
            if (val > 65535) val = 65535;

            auto heightMapValue = static_cast<uint16_t>(std::round(val));
            file.write(reinterpret_cast<const char*>(&heightMapValue), sizeof(heightMapValue));
        }
    }

    uint8_t layerMapValue = 1;
    for (int i = 0; i < 2048 * 2048; ++i)
    {
        file.write(reinterpret_cast<const char*>(&layerMapValue), sizeof(layerMapValue));
    }

    // uint8_t layerTextureMapValue = 1;
    // for (int i = 0; i < 2048 * 2048; ++i)
    // {
    //     file.write(reinterpret_cast<const char*>(&layerTextureMapValue), sizeof(layerTextureMapValue));
    // }

    uint32_t layerTextureMapValue = 8;
    file.write(reinterpret_cast<const char*>(&layerTextureMapValue), sizeof(layerTextureMapValue));

    auto writeString = [&file](const std::string& str)
    {
        uint8_t length = str.size();
        file.write(reinterpret_cast<const char*>(&length), sizeof(length));
        file.write(str.c_str(), length);
    };

    writeString("Grass");
    writeString("dirt");
    writeString("Grass2");
    writeString("BeachSand");
    writeString("dirt_grass");
    writeString("ROCK");
    writeString("Mud");
    writeString("Asphalt");

    return 0;
}

static PRM_Name sop_names[] = {
    PRM_Name("file", "File"),
    PRM_Name("remap", "Remap"),
    PRM_Name("inRange", "In Range"),
    PRM_Name("export", "Export"),
};

static PRM_Default shouldRemapDefault(1);
static PRM_Default inRangeDefault[] = {
    PRM_Default(0),
    PRM_Default(200)
};

PRM_Template SOP_BeamNGLandscapeExporter::g_myTemplateList[] = {
    PRM_Template(PRM_FILE, 1, &sop_names[0], 0),
    PRM_Template(PRM_TOGGLE, 1, &sop_names[1], &shouldRemapDefault),
    PRM_Template(PRM_FLT, 2, &sop_names[2], inRangeDefault),
    PRM_Template(PRM_CALLBACK, 1, &sop_names[3], 0, 0, 0, exportGeoData),
    PRM_Template()
};

OP_Node*
SOP_BeamNGLandscapeExporter::myConstructor(OP_Network* net, const char* name, OP_Operator* entry)
{
    return new SOP_BeamNGLandscapeExporter(net, name, entry);
}

SOP_BeamNGLandscapeExporter::
SOP_BeamNGLandscapeExporter(OP_Network* net, const char* name, OP_Operator* entry) : SOP_Node(net, name, entry)
{
    mySopFlags.setManagesDataIDs(true);
}

SOP_BeamNGLandscapeExporter::~
SOP_BeamNGLandscapeExporter() = default;

OP_ERROR
SOP_BeamNGLandscapeExporter::cookMySop(OP_Context& context)
{
    fpreal t = context.getTime();

    OP_AutoLockInputs inputs(this);
    if (inputs.lock(context) >= UT_ERROR_ABORT) return error();

    duplicateSource(0, context);

    return error();
}
