﻿// ********************************************************************************************************
// <copyright file="MapWindow.Legend.cs" company="MapWindow.org">
// Copyright (c) MapWindow.org. All rights reserved.
// </copyright>
// The contents of this file are subject to the Mozilla Public License Version 1.1 (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at 
// http:// Www.mozilla.org/MPL/ 
// Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF 
// ANY KIND, either express or implied. See the License for the specificlanguage governing rights and 
// limitations under the License. 
// 
// The Initial Developer of this version of the Original Code is Sergei Leschinski
// 
// Contributor(s): (Open source contributors should list themselves and their modifications here). 
// Change Log: 
// Date            Changed By      Notes
// ********************************************************************************************************

using MapWindow.Legend.Classes;
using MapWindow.Legend.Controls.Legend;

namespace MapWindow.Legend.Forms
{
    using System.Windows.Forms;
    using MapWinGIS;
    using MapWindow.Legend;

    partial class frmSymbologyMain
    {
        /// <summary>
        /// Initializes the state of dynamic visibility controls
        /// </summary>
        private void InitVisibilityTab()
        {
            scaleLayer.Locked = true;

            Layer layer = m_layer;
            scaleLayer.MaximumScale = layer.MaxVisibleScale;
            scaleLayer.MinimimScale = layer.MinVisibleScale;
            scaleLayer.UseDynamicVisibility = layer.UseDynamicVisibility;

            MapWinGIS.Map map = m_legend.Map;
            scaleLayer.CurrentScale = map.CurrentScale;
            
            ShpfileType type = Globals.ShapefileType2D(m_shapefile.ShapefileType);
            uint color = (type == ShpfileType.SHP_POLYLINE)? m_shapefile.DefaultDrawingOptions.LineColor : m_shapefile.DefaultDrawingOptions.FillColor;
            scaleLayer.FillColor = Colors.UintToColor(color);

            scaleLayer.Locked = false;
        }

        /// <summary>
        /// Handles the changes in the dynamic visibility state of the layer
        /// </summary>
        private void scaleLayer_StateChanged()
        {
            if (_noEvents)
                return;

            Layer lyr = m_layer;
            lyr.MaxVisibleScale = scaleLayer.MaximumScale;
            lyr.MinVisibleScale = scaleLayer.MinimimScale;
            lyr.UseDynamicVisibility = scaleLayer.UseDynamicVisibility;
            RedrawMap();
            Application.DoEvents();
        }
    }
}
