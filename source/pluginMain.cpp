// ---------------------------------------------------------------------
//
//  pluginMain.cpp
//  SHAPESTools
//
//  Created by ingo on 3/28/14.
//  Copyright (c) 2020 Ingo Clemens. All rights reserved.
//
// ---------------------------------------------------------------------

#include <string>

static const std::string kVERSION = "3.0.1";

#include <maya/MFnPlugin.h>

#include "SHAPESBrush.h"

// ---------------------------------------------------------------------
// initialization
// ---------------------------------------------------------------------

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Ingo Clemens", kVERSION.c_str(), "Any");

    status = plugin.registerContextCommand("SHAPESBrushContext",
                                           SHAPESBrushContextCmd::creator,
                                           "SHAPESBrushCmd",
                                           SHAPESBrushTool::creator);
    if (status != MStatus::kSuccess)
        status.perror("Register SHAPESBrushContext failed.");

    return status;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Ingo Clemens", kVERSION.c_str(), "Any");

    MGlobal::executeCommand("source SHAPESBrushToolCtx; SHAPESBrushToolCtxDelete;");

    status = plugin.deregisterContextCommand("SHAPESBrushContext",
                                             "SHAPESBrushCmd");
    if (status != MStatus::kSuccess)
        status.perror("Deregister SHAPESBrushContext failed.");

    return status;
}

// ---------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2020 Ingo Clemens, brave rabbit
// SHAPESBrush is under the terms of the MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Author: Ingo Clemens    www.braverabbit.com
// ---------------------------------------------------------------------
