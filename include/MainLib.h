//
// Created by Julian on 9/25/2024.
//

#ifndef HOUDINIBEAMNGLANDSCAPE_MAINLIB_H
#define HOUDINIBEAMNGLANDSCAPE_MAINLIB_H

#include <SOP/SOP_Node.h>

namespace HDK_BeamNGLandscapeExporter
{

class SOP_BeamNGLandscapeExporter : public SOP_Node
{
public:
     SOP_BeamNGLandscapeExporter(OP_Network* net, const char*, OP_Operator* entry);
    ~SOP_BeamNGLandscapeExporter() override;

    static OP_Node*     myConstructor(OP_Network* net, const char* name, OP_Operator* entry);
    static PRM_Template g_myTemplateList[];

protected:
    OP_ERROR cookMySop(OP_Context& context) override;
};
} // namespace HDK_BeamNGLandscapeExporter

#endif // HOUDINIBEAMNGLANDSCAPE_MAINLIB_H
